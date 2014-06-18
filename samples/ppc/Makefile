CC = powerpc64-unknown-linux-gnu-gcc-4.8.2
CC_Path = /opt/x-tools/powerpc64-unknown-linux-gnu/bin

FNAME = fib.c

all: fib

fib:
	$(CC_Path)/$(CC) -O0 $(FNAME) -o fib_O0_gcc_ppc64
	$(CC_Path)/$(CC) -O1 $(FNAME) -o fib_O1_gcc_ppc64
	$(CC_Path)/$(CC) -O2 $(FNAME) -o fib_O2_gcc_ppc64

clean:
	rm -f *.o *_ppc*
	
