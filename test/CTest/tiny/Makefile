CC = clang

lib_x86_64 = ../lib-x86_64
lib_arm = ../lib-arm

all:
	${CC} -I../include -L${lib_x86_64} -w -Wsystem-headers -target x86_64-linux-gnu -o x86_64.out *.c
	${CC} -I../include -L${lib_arm} -w -Wsystem-headers -target arm-eabi-gcc -o arm.out  *.c
