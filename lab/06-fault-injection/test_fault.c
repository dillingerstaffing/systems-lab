/* Fault-injection test over the lab/01 SPSC ring buffer.
 *
 * Three threads move framed data through the queue:
 *   producer - pushes N_FRAMES frames; each frame is 16 payload words
 *              followed by one CRC32 word covering the payload
 *              (CRC32 from lab/04, vendored here as crc32.c).
 *   consumer - pops each frame, recomputes the CRC32 over the popped
 *              payload, and compares it with the popped CRC word.
 *   injector - (fault-on mode only) flips one bit in a payload word of
 *              selected frames while the frame sits in the queue,
 *              written by the producer but not yet read by the consumer.
 *
 * The injector rendezvouses with the consumer through a gate: the
 * consumer parks before reading an injected frame, the injector flips
 * one bit in one of that frame's slots, then releases the consumer.
 * This emulates breakpoint-style fault injection (hold the reader,
 * corrupt the in-flight word, resume). Every injection is therefore
 * guaranteed to land in a slot the producer has written and the
 * consumer has not yet read, so the injected count and the detected
 * count must match exactly. The gate's release/acquire atomics give a
 * happens-before edge from the injector's flip to the consumer's read,
 * so the flip is not a data race. Without the gate the timing would
 * be nondeterministic and a flip could land just after the consumer
 * read the word, which would only weaken the experiment.
 *
 * Why the check must catch it: CRC32 detects every single-bit error in
 * a message shorter than 2^32 - 1 bits (the generator polynomial does
 * not divide x^k for small k, so a one-bit error pattern always leaves
 * a nonzero remainder). Each payload is 512 bits, far below the bound.
 *
 * Modes (argv[1]):
 *   off - no injector thread. All N_FRAMES frames must transfer with
 *         valid CRCs and intact payloads. Exit 0 on success, 1 on any
 *         failure.
 *   on  - the injector flips one bit per injected frame (every 32nd
 *         frame starting at frame 7). Every injected frame must fail
 *         its CRC check, each detection must attribute to the exact
 *         slot and bit the injector flipped, and no other frame may
 *         fail. The detections are printed and the program exits 1:
 *         that nonzero exit IS the test result, the loud signal that
 *         the integrity check caught the fault. Exit 2 means the
 *         harness itself disagreed (a missed injection, a false
 *         positive, or a count mismatch), which must never happen.
 */

#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "spsc.h"
#include "crc32.h"

#define N_FRAMES 200000u
#define CAPACITY 4096u
#define PAYLOAD_WORDS 16u
#define WORDS_PER_FRAME (PAYLOAD_WORDS + 1u) /* 16 payload + 1 CRC32 */
#define INJECT_EVERY 32u
#define INJECT_OFFSET 7u
#define INJECT_MAX (N_FRAMES / INJECT_EVERY + 1u)

static inline bool is_injected(uint32_t f)
{
    return f % INJECT_EVERY == INJECT_OFFSET;
}

/* Ring-buffer slot counter of the first word of frame f. */
static inline size_t frame_slot(uint32_t f)
{
    return (size_t)f * WORDS_PER_FRAME;
}

/* Deterministic payload: word 0 carries the frame sequence number so
 * loss or reorder is visible; the rest is a fixed mix of f and j. */
static inline uint32_t payload_word(uint32_t f, uint32_t j)
{
    if (j == 0)
        return f;
    return (f * UINT32_C(2654435761)) ^ (j * UINT32_C(0x9E3779B1)) ^
           UINT32_C(0x85EBCA6B);
}

/* Deterministic fault target inside frame f: which payload word and
 * which bit to flip. */
static inline void fault_target(uint32_t f, uint32_t *word, uint32_t *bit)
{
    *word = (f ^ (f >> 5)) % PAYLOAD_WORDS;
    *bit = (f * 31u + 7u) % 32u;
}

/* Shared injector/consumer gate plus the injector's log. The log is
 * written by the injector thread and read by main after pthread_join,
 * which orders the two. */
typedef struct {
    spsc_t *q;
    atomic_size_t target; /* frame the consumer is parked on */
    atomic_bool armed;    /* injector sets true once the flip landed */
    uint32_t inj_frame[INJECT_MAX];
    size_t inj_slot[INJECT_MAX];
    uint32_t inj_word[INJECT_MAX];
    uint32_t inj_bit[INJECT_MAX];
    uint32_t inj_count;
} fi_t;

/* Consumer's result block. */
typedef struct {
    uint32_t frames_ok;  /* non-injected frames, CRC and payload verified */
    uint32_t detected;   /* injected frames whose CRC check failed */
    uint32_t det_frame[INJECT_MAX];
    uint32_t det_word[INJECT_MAX]; /* payload word that differed */
    uint32_t det_xor[INJECT_MAX];  /* popped ^ expected for that word */
    uint32_t det_crc_popped[INJECT_MAX];
    uint32_t det_crc_recomp[INJECT_MAX];
    uint32_t errors; /* CRC failures on non-injected frames, or payload
                      * mismatches on frames that passed their CRC */
} cons_res_t;

typedef struct {
    fi_t *st;
    bool inject_on;
    cons_res_t *res; /* consumer only */
} worker_arg_t;

static void *producer(void *arg)
{
    fi_t *st = ((worker_arg_t *)arg)->st;
    spsc_t *q = st->q;
    uint32_t payload[PAYLOAD_WORDS];

    for (uint32_t f = 0; f < N_FRAMES; f++) {
        for (uint32_t j = 0; j < PAYLOAD_WORDS; j++)
            payload[j] = payload_word(f, j);
        uint32_t crc = crc32_table(payload, sizeof payload);
        for (uint32_t j = 0; j < PAYLOAD_WORDS; j++)
            while (!spsc_push(q, payload[j])) {
                /* full: spin until the consumer makes room */
            }
        while (!spsc_push(q, crc)) {
            /* full: spin until the consumer makes room */
        }
    }
    return NULL;
}

static void *injector(void *arg)
{
    fi_t *st = ((worker_arg_t *)arg)->st;
    spsc_t *q = st->q;
    uint32_t n = 0;

    for (uint32_t f = 0; f < N_FRAMES; f++) {
        if (!is_injected(f))
            continue;
        /* Wait until the whole frame is in the queue. The acquire load
         * pairs with the producer's release store of tail, so the slot
         * writes below are visible to us. */
        size_t end = frame_slot(f) + WORDS_PER_FRAME;
        while (atomic_load_explicit(&q->tail, memory_order_acquire) < end) {
            /* frame not fully written yet */
        }
        /* Wait until the consumer is parked on this frame: head is then
         * exactly frame_slot(f), so no word of the frame has been read. */
        while (atomic_load_explicit(&st->target, memory_order_acquire) != f) {
            /* consumer not parked yet */
        }
        /* The consumer re-arms the gate after each injection. */
        while (atomic_load_explicit(&st->armed, memory_order_acquire)) {
            /* previous release not yet consumed */
        }

        uint32_t w, b;
        fault_target(f, &w, &b);
        size_t slot = frame_slot(f) + w;
        /* Genuine bit flip in the live ring-buffer slot. Ordered before
         * the consumer's read by the release store of `armed` below. */
        q->buf[slot & q->mask] ^= (uint32_t)1u << b;

        st->inj_frame[n] = f;
        st->inj_slot[n] = slot;
        st->inj_word[n] = w;
        st->inj_bit[n] = b;
        n++;

        atomic_store_explicit(&st->armed, true, memory_order_release);
    }
    st->inj_count = n;
    return NULL;
}

static void *consumer(void *arg)
{
    worker_arg_t *a = arg;
    fi_t *st = a->st;
    spsc_t *q = st->q;
    cons_res_t *r = a->res;
    uint32_t payload[PAYLOAD_WORDS];

    for (uint32_t f = 0; f < N_FRAMES; f++) {
        bool injected = a->inject_on && is_injected(f);
        if (injected) {
            /* Park before reading frame f; the injector flips one of
             * its slots, then releases us. */
            atomic_store_explicit(&st->target, (size_t)f,
                                  memory_order_release);
            while (!atomic_load_explicit(&st->armed, memory_order_acquire)) {
                /* waiting for the injector's flip */
            }
            atomic_store_explicit(&st->armed, false, memory_order_release);
        }

        for (uint32_t j = 0; j < PAYLOAD_WORDS; j++)
            while (!spsc_pop(q, &payload[j])) {
                /* empty: spin until the producer delivers */
            }
        uint32_t crc_word;
        while (!spsc_pop(q, &crc_word)) {
            /* empty: spin until the producer delivers */
        }

        uint32_t check = crc32_table(payload, sizeof payload);
        if (check != crc_word) {
            if (!injected) {
                r->errors++; /* CRC failed with no fault injected */
                continue;
            }
            /* Attribute the detection: find the word that differs and
             * confirm it differs by exactly the injected bit. */
            uint32_t dw = PAYLOAD_WORDS, dx = 0;
            for (uint32_t j = 0; j < PAYLOAD_WORDS; j++) {
                uint32_t want = payload_word(f, j);
                if (payload[j] != want) {
                    dw = j;
                    dx = payload[j] ^ want;
                    break;
                }
            }
            uint32_t i = r->detected++;
            r->det_frame[i] = f;
            r->det_word[i] = dw;
            r->det_xor[i] = dx;
            r->det_crc_popped[i] = crc_word;
            r->det_crc_recomp[i] = check;
            continue;
        }
        if (injected) {
            r->errors++; /* fault injected but CRC passed: missed */
            continue;
        }
        /* Clean frame: verify the full payload, not just the CRC. */
        for (uint32_t j = 0; j < PAYLOAD_WORDS; j++) {
            if (payload[j] != payload_word(f, j)) {
                r->errors++;
                break;
            }
        }
        r->frames_ok++;
    }
    return NULL;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    bool inject_on;
    if (argc == 2 && strcmp(argv[1], "on") == 0)
        inject_on = true;
    else if (argc == 2 && strcmp(argv[1], "off") == 0)
        inject_on = false;
    else {
        fprintf(stderr, "usage: %s on|off\n", argv[0]);
        return 2;
    }

    /* Build the CRC table single-threaded so the worker threads never
     * race on its lazy initialization. */
    {
        uint32_t z = 0;
        (void)crc32_table(&z, sizeof z);
    }

    /* 4096 slots x 4 bytes = 16 KiB, comfortably inside L1. */
    static uint32_t storage[CAPACITY];
    spsc_t q;
    if (!spsc_init(&q, storage, CAPACITY)) {
        printf("FAIL spsc_init rejected capacity %u\n", CAPACITY);
        return 1;
    }

    static fi_t st;
    memset(&st, 0, sizeof st);
    st.q = &q;
    atomic_init(&st.target, (size_t)-1);
    atomic_init(&st.armed, false);

    cons_res_t *res = calloc(1, sizeof *res);
    if (!res) {
        printf("FAIL calloc failed\n");
        return 1;
    }

    worker_arg_t parg = {&st, inject_on, NULL};
    worker_arg_t carg = {&st, inject_on, res};
    worker_arg_t iarg = {&st, inject_on, NULL};
    pthread_t prod, cons, inj;

    printf("[fault] mode=%s frames=%u capacity=%u words/frame=%u\n",
           inject_on ? "on" : "off", N_FRAMES, CAPACITY, WORDS_PER_FRAME);

    double t0 = now_s();
    if (pthread_create(&prod, NULL, producer, &parg) != 0 ||
        pthread_create(&cons, NULL, consumer, &carg) != 0 ||
        (inject_on && pthread_create(&inj, NULL, injector, &iarg) != 0)) {
        printf("FAIL pthread_create failed\n");
        return 1;
    }
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    if (inject_on)
        pthread_join(inj, NULL);
    double dt = now_s() - t0;

    uint64_t words = (uint64_t)N_FRAMES * WORDS_PER_FRAME;
    printf("[fault] transferred %" PRIu32 " frames (%" PRIu64
           " words) in %.3f s\n",
           N_FRAMES, words, dt);
    printf("[fault] throughput: %.1f Mwords/s (push+pop counted)\n",
           (2.0 * words / dt) / 1e6);

    if (!inject_on) {
        printf("[fault] frames verified clean: %" PRIu32 " / %" PRIu32 "\n",
               res->frames_ok, N_FRAMES);
        printf("[fault] errors: %" PRIu32 "\n", res->errors);
        bool clean = (res->frames_ok == N_FRAMES && res->errors == 0);
        free(res);
        if (clean) {
            printf("ALL FRAMES CLEAN\n");
            return 0;
        }
        printf("FAIL clean transfer did not verify\n");
        return 1;
    }

    /* Fault-on: every injected frame must have been detected, each
     * detection attributed to the exact flipped slot and bit, and no
     * non-injected frame may have failed. */
    uint32_t expect_inj = (N_FRAMES - 1u - INJECT_OFFSET) / INJECT_EVERY + 1u;
    printf("[fault] injected: %" PRIu32 " (expected %" PRIu32
           "), detected: %" PRIu32 ", clean frames ok: %" PRIu32
           ", errors: %" PRIu32 "\n",
           st.inj_count, expect_inj, res->detected, res->frames_ok,
           res->errors);

    bool mismatch = false;
    if (st.inj_count != expect_inj) {
        printf("FAIL injector ran %" PRIu32 " times, want %" PRIu32 "\n",
               st.inj_count, expect_inj);
        mismatch = true;
    }
    if (res->detected != st.inj_count) {
        printf("FAIL detected %" PRIu32 ", injected %" PRIu32 "\n",
               res->detected, st.inj_count);
        mismatch = true;
    }
    if (res->errors != 0) {
        printf("FAIL %" PRIu32
               " errors (false positive or missed injection)\n",
               res->errors);
        mismatch = true;
    }
    for (uint32_t i = 0; i < res->detected && !mismatch; i++) {
        if (res->det_frame[i] != st.inj_frame[i] ||
            res->det_word[i] != st.inj_word[i] ||
            res->det_xor[i] != ((uint32_t)1u << st.inj_bit[i])) {
            printf("FAIL detection %u not attributed to its injection: "
                   "frame %" PRIu32 " (injected %" PRIu32 "), word %" PRIu32
                   " (injected %" PRIu32 "), xor 0x%08" PRIx32 "\n",
                   i, res->det_frame[i], st.inj_frame[i], res->det_word[i],
                   st.inj_word[i], res->det_xor[i]);
            mismatch = true;
        }
    }

    /* Show a few real detection records. */
    uint32_t show = res->detected < 3 ? res->detected : 3;
    for (uint32_t i = 0; i < show; i++) {
        printf("[fault] detected #%u: frame %" PRIu32 ", slot %zu, word "
               "%" PRIu32 ", bit %" PRIu32 " flipped, popped CRC "
               "0x%08" PRIx32 " vs recomputed 0x%08" PRIx32 "\n",
               i, res->det_frame[i], st.inj_slot[i], res->det_word[i],
               st.inj_bit[i], res->det_crc_popped[i],
               res->det_crc_recomp[i]);
    }

    free(res);
    if (mismatch) {
        printf("FAIL harness discrepancy (see above)\n");
        return 2;
    }
    printf("FAULT DETECTED: every injected corruption caught by the "
           "integrity check\n");
    return 1; /* nonzero: the loud signal that corruption was detected */
}
