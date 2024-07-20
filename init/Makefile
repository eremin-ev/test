init: init.S
	as -o init.o init.S
	ld -o init init.o

.PHONY: clean
clean:
	rm -vf init *.o
