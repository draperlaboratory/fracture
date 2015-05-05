CC=`xcrun --sdk iphoneos --find clang`
SYSROOT=`xcrun --sdk iphoneos --show-sdk-path`

all: fibmacho simplemacho

fibmacho: fib.c
	$(CC) fib.c -isysroot $(SYSROOT) -arch armv7 -o fibmacho

simplemacho: simple.s
	$(CC) simple.s -isysroot $(SYSROOT) -arch armv7 -o simplemacho

clean:
	rm -f fibmacho simplemacho