CC = arm-linux-gnueabi-gcc-4.4
LLC = ecc -target arm-ellcc-linux
LLC_Path = /opt/x-tools/ellcc/bin

FNAME = fib

all: fib

fib:
	$(CC) -O0 $(FNAME).c -o fib_O0_gcc_arm
	$(CC) -O1 $(FNAME).c -o fib_O1_gcc_arm
	$(CC) -O2 $(FNAME).c -o fib_O2_gcc_arm
	$(LLC_Path)/$(LLC) -O0 $(FNAME).c -o $(FNAME)_O0_clang_arm
	$(LLC_Path)/$(LLC) -O1 $(FNAME).c -o $(FNAME)_O1_clang_arm
	$(LLC_Path)/$(LLC) -O2 $(FNAME).c -o $(FNAME)_O2_clang_arm
	
clean:
	rm -f *.o *_arm*

