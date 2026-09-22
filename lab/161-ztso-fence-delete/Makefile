all: run

# Three runs; the compiler output is deterministic, so the logs must be
# byte-identical and the checksum stable. Python's hash seed does not
# enter any output.
run:
	@for i in 1 2 3; do python3 check.py > run$$i.txt || exit 1; done
	@cat run1.txt run2.txt run3.txt | sha256sum

clean:
	rm -f tso_base.s tso_ztso.s tso_base.o tso_ztso.o run1.txt run2.txt run3.txt
