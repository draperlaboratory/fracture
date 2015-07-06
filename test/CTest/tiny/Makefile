CC = clang

lib_x86_64 = ../lib-x86_64
lib_arm = ../lib-arm
C_SOURCES = $(wildcard *.c)

all: arm x86
	echo help trapped in Makefile!

arm: $(C_SOURCES:.c=.arm-out)
	echo building arm binaries

x86: $(C_SOURCES:.c=.x86-out)
	echo building x86 binaries

%.arm-out : %.c
	$(CC) -I../include -w -Wsystem-headers -target arm-linux-eabi -o $@  $<

%.x86-out : %.c
	$(CC) -I../include -w -Wsystem-headers -target x86_64-linux-gnu -o $@ $<
