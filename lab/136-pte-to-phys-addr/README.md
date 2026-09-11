# lab/136-pte-to-phys-addr

Sv39 physical-address formation from a leaf PTE's page number:
`phys_addr = ((ppn & 0xFFFFFFFFFFF) << 12) | (offset & 0xFFF)`,
built from the shift/OR identities on the 44-bit PPN (PTE bits
53:10) and the 12-bit page offset.

Verified by differential test against a hand-written per-bit
oracle that walks destination bits 0..55 and sets each bit
individually from single-bit source extractions (structurally
independent of the whole-field shift/OR implementation, no shared
helper code): directed 2^k / 2^k +/- 1 edges over the 56-bit
physical space, all-ones/all-zeros rows, mask-contract rows
confirming bits above the 44/12-bit inputs are never read, three
exhaustive 24-bit PPN lanes (50,331,648 cases), and 10M
fixed-seed splitmix64 (ppn, offset) pairs, 0 mismatches over
60,331,824 checks. FNV-1a checksum of the outputs is identical
across -O2, -O0, and ASan+UBSan builds. See PROOF.md for the full
verification record.

Build and verify:

    make            # builds test_pte_phys
    make run        # runs the full differential test
    make sanitize   # full test under ASan+UBSan
    make opt0       # full test at -O0 (checksum must match)
    make bench      # throughput bench (10M conversions, best of 5 reps)
    make clean      # removes built binaries (never committed)
