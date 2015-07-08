CC = clang
CSMITH_INCLUDE = ../../include
C_SOURCES = $(wildcard *.c)

all: arm
	echo done

arm: $(C_SOURCES:.c=.arm-out)
	echo building arm binaries
	mkdir -p armbins
	mv -f *.arm-out armbins

x86: $(C_SOURCES:.c=.x86-out)
	echo building x86 binaries
	mkdir -p x86bins
	mv -f *.x86-out x86bins

%.arm-out : %.c
	$(CC) -I$(CSMITH_INCLUDE) -w -Wsystem-headers -target arm-linux-eabi -o $@  $<

%.x86-out : %.c
	$(CC) -I$(CSMITH_INCLUDE) -w -Wsystem-headers -target x86_64-linux-gnu -o $@ $<

