# lab/147-pte-global-bit

Sv39 PTE G-bit (global mapping) rule: a leaf PTE with G set marks its
translation global, so the cached translation applies across address
spaces without consulting the ASID; a leaf with G clear is ASID-scoped.
This module builds a from-scratch Sv39 page-table walker (validity,
leaf detection, the reserved W-without-R combination, superpage
misalignment) plus a small ASID-tagged translation cache, both from the
PTE bit identities, and verifies them against an independently written
spec table over G set/clear x ASID matching/mismatching combinations.

Three satp values (ASID 1, ASID 2, ASID 9) share one global leaf and
translate the same virtual page to the same physical page; the
non-global contrast mappings stay per-ASID. See PROOF.md for the full
verification record.
