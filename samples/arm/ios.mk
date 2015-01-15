CC=`xcrun --sdk iphoneos --find clang`
SYSROOT=`xcrun --sdk iphoneos --show-sdk-path`

all: fibmacho

fibmacho: fib.c
	$(CC) fib.c -isysroot $(SYSROOT) -arch armv7 -o fibmacho

clean:
	rm -f fibmacho