# lab/164-srai-division

`x >> 3` is not `x / 8` for negative `x`: the shift rounds toward
negative infinity, C division truncates toward zero. Firsthand gcc
13.2.0 listings plus a QEMU 8.2.2 run of both paths on thirteen
inputs. See PROOF.md.
