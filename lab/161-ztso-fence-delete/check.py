#!/usr/bin/env python3
"""Verify the Ztso fence-deletion table firsthand.

Compiles tso.c twice (xpack riscv-none-elf-gcc 15.2.0, -O2):
  base: -march=rv64gc          (weak RVWMO atomics codegen)
  ztso: -march=rv64gc_ztso     (Total Store Ordering codegen)

For each of the four atomic functions, extracts the fence instructions
from the function body and asserts the expected sequence. Then asserts
the binary signature: the ztso object's Tag_RISCV_arch gains _ztso1p0,
the base object's does not.

10 checks per run. Exits nonzero on any mismatch.
"""
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
GCC = Path.home() / "workspace/toolchains/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-gcc"
READELF = Path.home() / "workspace/toolchains/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-readelf"
CFLAGS = ["-O2", "-mabi=lp64d", "-Wall", "-Wextra", "-Werror"]

# function -> (base fences, ztso fences), in program order
EXPECTED = {
    "st_seq_cst": (["fence\trw,w", "fence\trw,rw"], ["fence\trw,rw"]),
    "ld_seq_cst": (["fence\trw,rw", "fence\tr,rw"], ["fence\trw,rw"]),
    "st_release": (["fence\trw,w"], []),
    "ld_acquire": (["fence\tr,rw"], []),
}

checks = 0
mismatches = 0


def check(name, cond, detail=""):
    global checks, mismatches
    checks += 1
    if cond:
        print(f"ok   {name}")
    else:
        mismatches += 1
        print(f"FAIL {name} {detail}")


def fences_of(asm_text, func):
    """Fence instructions inside one function's body, in order."""
    body = re.search(rf"^{func}:$(.*?)^\t\.size\t{func},",
                     asm_text, re.M | re.S)
    assert body, f"function {func} not found"
    return re.findall(r"^\tfence\t\w+,\w+$", body.group(1), re.M)


def main():
    for tag, march in (("base", "rv64gc"), ("ztso", "rv64gc_ztso")):
        s_path = HERE / f"tso_{tag}.s"
        o_path = HERE / f"tso_{tag}.o"
        r = subprocess.run([str(GCC), *CFLAGS, f"-march={march}",
                            "-S", "-o", str(s_path), str(HERE / "tso.c")],
                           capture_output=True, text=True)
        check(f"{tag}: compiles clean", r.returncode == 0, r.stderr.strip())
        asm = s_path.read_text()
        for func, (want_base, want_ztso) in EXPECTED.items():
            want = want_ztso if tag == "ztso" else want_base
            got = [" ".join(f.split()) for f in fences_of(asm, func)]
            want_n = [" ".join(w.split()) for w in want]
            check(f"{tag}: {func} fences == {want_n}", got == want_n,
                  f"got {got}")

    # Binary signature: the arch string, not the code.
    for tag, march in (("base", "rv64gc"), ("ztso", "rv64gc_ztso")):
        o_path = HERE / f"tso_{tag}.o"
        subprocess.run([str(GCC), *CFLAGS, f"-march={march}",
                        "-c", "-o", str(o_path), str(HERE / "tso.c")],
                       check=True, capture_output=True)
        r = subprocess.run([str(READELF), "-A", str(o_path)],
                           capture_output=True, text=True)
        has_ztso = "_ztso1p0" in r.stdout
        check(f"{tag}: Tag_RISCV_arch {'has' if tag == 'ztso' else 'lacks'} _ztso1p0",
              has_ztso == (tag == "ztso"))

    print(f"checks={checks} mismatches={mismatches}")
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
