# lab/144-pte-permission-check

Sv39 leaf PTE permission check from the raw flag bits: pte_perm_ok decides
whether a read, write, or execute access faults, from the PTE's R, W, X, U
bits and the current privilege mode (S or U), at baseline mstatus (MXR=0,
SUM=0), following the access rules in §4.3.2 (steps 3 and 5) and §4.1.1 of
the RISC-V Privileged Architecture v20211203. MXR and SUM are explicitly
out of scope.

Verified by differential test against a structurally independent
spec-table oracle (no shared code with the implementation): all 256 raw
flag bytes x 3 access types x 2 modes, plus directed edge rows for the
reserved W-without-R combination and S/U-mode U-bit mismatches,
0 mismatches. See PROOF.md for the full verification record.

Build and verify:

    make            # builds the differential tests (-O0, -O2, ASan+UBSan)
    make run        # runs the full differential test on all three builds
    make bench      # throughput bench (200000 x 1536 checks, best of 5 reps)
    make clean      # removes built binaries (never committed)
    ./gen_proof.sh  # re-runs everything and regenerates PROOF.md
