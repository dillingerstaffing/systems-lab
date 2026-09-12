# lab/140-mulhi-unsigned

`mulhi_u64(a, b)`: the high 64 bits of the 128-bit unsigned product of
two `uint64_t` values, computed from the 32-bit splitting identity.

## The idea

Write a = ah*2^32 + al and b = bh*2^32 + bl. Then

    a*b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl,

so the high word is the ah*bh partial product plus the carry
accounting from the middle terms:

    high = p3 + (m1 << 32) + (mid >> 32) + c,

where mid = p1 + p2 (computed wrapped, m1 = 1 exactly when the add
wrapped) and c = 1 exactly when (mid << 32) + p0 wrapped. Each of the
four partial products is 32 bits x 32 bits, so it fits in 64 bits:
the 128-bit product is never formed and no 128-bit type is used.

The unsigned construction complements lab/94 (the signed case), which
needed extra sign-correction subtracts; the unsigned high word needs
only the splitting identity plus carry accounting.

## Files

- `mulhi_unsigned.h` - the implementation
- `test_mulhi_unsigned.c` - differential test against an exact
  `unsigned __int128` reference (the wide type is confined to the
  test's oracle)
- `PROOF.md` - construction, genuine build log, genuine run output,
  disassembly check, and the machine-readable proof header
- `Makefile` - builds `-O0`, `-O2`, and ASan+UBSan variants

## Build

```
make run
```
