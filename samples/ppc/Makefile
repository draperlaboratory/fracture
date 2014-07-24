CC = powerpc-unknown-linux-gnu-gcc-4.8.2
CC_Path = /opt/x-tools/powerpc-unknown-linux-gnu/bin
CC64 = powerpc64-unknown-linux-gnu-gcc-4.8.2
CC_Path64 = /opt/x-tools/powerpc64-unknown-linux-gnu/bin
LLC_Path = /opt/x-tools/ellcc/bin
LLC = ecc -target ppc-ellcc-linux
LLC_64 = ecc -target ppc64-ellcc-linux

FNAME = fib

all: fib

fib:
	$(CC_Path)/$(CC) -O0 $(FNAME).c -o $(FNAME)_O0_gcc_ppc32
	$(CC_Path)/$(CC) -O1 $(FNAME).c -o $(FNAME)_O1_gcc_ppc32
	$(CC_Path)/$(CC) -O2 $(FNAME).c -o $(FNAME)_O2_gcc_ppc32
	$(CC_Path64)/$(CC64) -O0 $(FNAME).c -o $(FNAME)_O0_gcc_ppc64
	$(CC_Path64)/$(CC64) -O1 $(FNAME).c -o $(FNAME)_O1_gcc_ppc64
	$(CC_Path64)/$(CC64) -O2 $(FNAME).c -o $(FNAME)_O2_gcc_ppc64
	$(LLC_Path)/$(LLC) -O0 $(FNAME).c -o $(FNAME)_O0_clang_ppc	
	$(LLC_Path)/$(LLC) -O1 $(FNAME).c -o $(FNAME)_O1_clang_ppc
	$(LLC_Path)/$(LLC) -O2 $(FNAME).c -o $(FNAME)_O2_clang_ppc
	#$(LLC_Path)/$(LLC_64) -O0 $(FNAME).c -o $(FNAME)_O0_clang_ppc64
	#$(LLC_Path)/$(LLC_64) -O1 $(FNAME).c -o $(FNAME)_O1_clang_ppc64
	#$(LLC_Path)/$(LLC_64) -O2 $(FNAME).c -o $(FNAME)_O2_clang_ppc64
clean:
	rm -f *.o *_ppc*
	
