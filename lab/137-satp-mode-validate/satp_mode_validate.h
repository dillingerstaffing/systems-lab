#ifndef SATP_MODE_VALIDATE_H
#define SATP_MODE_VALIDATE_H

/*
 * RV64 satp.MODE legality check.
 *
 * The satp CSR's MODE field (bits 63:60 when SXLEN=64) selects the
 * address-translation scheme. The privileged architecture names the
 * valid encodings in Table 114 of section 4.1.11: 0 is Bare (no
 * translation or protection), 8 is Sv39, 9 is Sv48, 10 is Sv57.
 * Values 1-7 and 12-13 are reserved for standard use, 11 is reserved
 * for a future page-based 64-bit scheme (Sv64, not yet defined), and
 * 14-15 are designated for custom use.
 *
 * satp_mode_legal_rv64(mode):
 *   returns 1 when `mode` names a scheme the spec defines
 *              (0, 8, 9, or 10),
 *   returns 0 otherwise, including any value above 15, which the
 *              4-bit field cannot hold.
 *
 * This is the legality of the encoding in the spec, not a claim about
 * a particular hart: the spec states that implementations are not
 * required to support all MODE settings, and that a write with an
 * unsupported MODE has no effect.
 */
int satp_mode_legal_rv64(unsigned mode);

#endif /* SATP_MODE_VALIDATE_H */
