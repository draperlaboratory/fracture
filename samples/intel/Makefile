LLC = ecc -target i386-ellcc-linux
LLC_64 = ecc -target x86_64-ellcc-linux
LLC_Path = /opt/x-tools/ellcc/bin


FNAME = fib

all: fib

fib:
#	$(CC) -O0 $(FNAME).c -o fib_O0_gcc_x86
#	$(CC) -O1 $(FNAME).c -o fib_O1_gcc_x86
#	$(CC) -O2 $(FNAME).c -o fib_O2_gcc_x86
	$(LLC_Path)/$(LLC) -O0 $(FNAME).c -o $(FNAME)_O0_clang_x86
	$(LLC_Path)/$(LLC) -O1 $(FNAME).c -o $(FNAME)_O1_clang_x86
	$(LLC_Path)/$(LLC) -O2 $(FNAME).c -o $(FNAME)_O2_clang_x86
	$(LLC_Path)/$(LLC) -O0 $(FNAME).c -o $(FNAME)_O2_clang_64
	$(LLC_Path)/$(LLC) -O1 $(FNAME).c -o $(FNAME)_O2_clang_64
	$(LLC_Path)/$(LLC) -O2 $(FNAME).c -o $(FNAME)_O2_clang_64
	
clean:
	rm -f *.o *_x86*
	rm -f *.o *_64*

