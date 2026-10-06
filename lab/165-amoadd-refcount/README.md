# lab/165-amoadd-refcount

`amoadd.w` returns the value a word held BEFORE the add, which turns a
reference-count drop into one instruction: if the returned old value is
1, this drop took the count to zero and the caller is the last holder.
Firsthand gcc 13.2.0 listings plus a QEMU 8.2.2 run of three drops and
one acquire. See PROOF.md.
