CC  = clang
GCC = gcc
OPT = -m32

FNAME = fib

all: x32

x32:
	$(GCC) $(OPT) -O0 $(FNAME).c -o fib_O0_gcc_elf_x86
	$(GCC) $(OPT) -O1 $(FNAME).c -o fib_O1_gcc_elf_x86
	$(GCC) $(OPT) -O2 $(FNAME).c -o fib_O2_gcc_elf_x86
	$(CC)  $(OPT) -O0 $(FNAME).c -o fib_O0_llvm_elf_x86
	$(CC)  $(OPT) -O1 $(FNAME).c -o fib_O1_llvm_elf_x86
	$(CC)  $(OPT) -O2 $(FNAME).c -o fib_O2_llvm_elf_x86

x64:
	$(GCC) -O0 $(FNAME).c -o fib_O0_gcc_elf_x64
	$(GCC) -O1 $(FNAME).c -o fib_O1_gcc_elf_x64
	$(GCC) -O2 $(FNAME).c -o fib_O2_gcc_elf_x64
	$(CC)  -O0 $(FNAME).c -o fib_O0_llvm_elf_x64
	$(CC)  -O1 $(FNAME).c -o fib_O1_llvm_elf_x64
	$(CC)  -O2 $(FNAME).c -o fib_O2_llvm_elf_x64	
	
clean:
	rm -f *.o *elf_x86*
	rm -f *.o *elf_x64*

