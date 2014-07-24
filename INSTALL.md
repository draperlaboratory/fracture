INSTALLATION INSTRUCTIONS
=========================

This version of the Fracture library works with LLVM r202033. You will have to
compile this version of LLVM before you try to use Fracture. This
guide will walk you through the compilation and installation of both
tools and show usage statements to verify that the fracture library is
compiled correctly.

The library is known to compile on various linux versions (Redhat,
Mageia, Ubuntu, Debian). It should work on Mac OSX and FreeBSD. It
does not compile on Windows.

Step 1: Installing LLVM
=======================

Fracture relies on specific LLVM internals, and so it is best to use
it with a specific revision of the LLVM development tree. Currently
that revision is:

     202033

You will also need clang to compile the Fracture library. To aid you,
we have forked these repositories at the appropriate revisions:

* https://github.com/draperlaboratory/clang
* https://github.com/draperlaboratory/llvm

As Fracture is a prototype library, we only use it with debugging
enabled. You must compile LLVM and Clang with the same settings. A
sample of commands to do that is as follows:

    git clone https://github.com/draperlaboratory/llvm
    cd llvm/tools
    git clone https://github.com/draperlaboratory/clang
    cd ..
    ./configure --enable-debug-symbols --prefix=/usr/local --build=<your info>
    make -j16
    sudo make install

Note: The --build option is important, it should match your `gcc -v`
output:

    $ gcc -v
    Using built-in specs.
    COLLECT_GCC=gcc
    COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-mageia-linux-gnu/4.8.2/lto-wrapper
    Target: x86_64-mageia-linux-gnu
    Configured with: ../configure --prefix=/usr --libexecdir=/usr/lib --with-slibdir=/lib

In this example, the `--build` variable is:

    ./configure --enable-debug-symbols --prefix=/usr/local --build=x86_64-mageia-linux-gnu

Install libeditline
-------------------

Fracture needs an editline or readline library in addition to the
libraries required by LLVM. You will need the "dev" version, in
Mageia, the install line looks like:

    sudo urpmi lib64edit-devel

You can also install from source. Either is known to work.

Step 2: Compiling Fracture
==========================

Sample commands to download and compile fracture are:

    git clone https://github.com/draperlaboratory/fracture.git fracture
    cd fracture
    ./autoconf/AutoRegen.sh
    ./configure --enable-debug-symbols --with-llvmsrc=/opt/llvm --with-llvmobj=/opt/llvm
    make -j16

The directory `/opt/llvm` is the directory you compiled llvm in step
1. We assume you were in `/opt` when you compiled.

NOTE: On Ubuntu, Mageia, and other systems we have had to add the following flag:
    --disable-optimized

Note that the compile generates a large amount of debug output from the
fracture-tblgen tool. There may also be warnings in the output.

Linux
-----

The most common problem is out of date c++ std libraries. LLVM needs a
relatively new version of these libs, and the solution is to install a new
g++ (4.7 or newer).

FreeBSD
-------

Tested on RELEASE-9.2 with the latest ports (the first FreeBSD to support the
newer libraries):

    cd /usr/ports/devel/libc++
    sudo make install clean
    ...
    cd llvm
    ./configure --enable-debug-symbols --prefix=/usr/local --build=amd64-undermydesk-freebsd --enable-libcpp
    ... (gmake, gmake install, etc)
    cd fracture
    export CXXFLAGS=CXXFLAGS='-std=c++11 -stdlib=libc++ -nostdinc++ -I/usr/local/include/c++/v1'
    ./autoconf/AutoRegen.sh
    ./configure --enable-debug-symbols --with-llvmsrc=/llvmdir --with-llvmobj=/llvm
    gmake -j16

Mac OSX
-------

Tested on Version 10.9.2 with XCode and fink (for unix packages). First, install
libcxx from the LLVM site, then:

    cd llvm
    export CXXFLAGS="-std=c++11 -stdlib=libc++ -nostdinc++ -I[libcxxdir]/include"
    ./configure --enable-debug-symbols --enable-shared --prefix=/sw --build=x86_64-apple-darwin13.0.0 CXX=g++ CC=gcc -enable-libcpp
    ... (make, make install)
    cd fracture
    ./autoconf/AutoRegen.sh
    ./configure --enable-debug-symbols --build=x86_64-apple-darwing13.0.0 --with-llvmsrc=llvm --with-llvmobj=llvm
    make -j16

Other Systems
-------------

We expect fracture to compile on other systems that compile LLVM and clang, but
we have not tested it. If you try it and encounter problems, please let us know
(fracture@draper.com). If you succeed in compiling it, also let us know so we
can post the details in this install document.

Step 3: Usage Examples
======================

If fracture compiles, you should be able to run it with the following commands.
Note that the only working architecture right now is ARM, and you have to
specify machine attribute flags to make the system detect the instructions in
the sample below.

```
$ ./Debug+Asserts/bin/fracture-cl -mattr=v6 ./samples/arm/fib_arm
MCDirector: Using Triple: arm-unknown-unknown
MCDirector: Using CPU: generic
MCDirector: Using Features: +v6
Disassembler: Setting Section .text
Debug+Asserts/bin/fracture-cl> sect
Sections:
Idx Name          Size      Address          Type
  1               00000000 0000000000000000
  2 .interp       00000013 0000000000008134 DATA
  3 .note.ABI-tag 00000020 0000000000008148
  4 .note.gnu.build-id 00000024 0000000000008168
  5 .hash         00000028 000000000000818c
  6 .gnu.hash     0000002c 00000000000081b4
  7 .dynsym       00000050 00000000000081e0
  8 .dynstr       00000043 0000000000008230
  9 .gnu.version  0000000a 0000000000008274
 10 .gnu.version_r 00000020 0000000000008280
 11 .rel.dyn      00000008 00000000000082a0
 12 .rel.plt      00000020 00000000000082a8
 13 .init         00000010 00000000000082c8 TEXT DATA
 14 .plt          00000044 00000000000082d8 TEXT DATA
 15 .text         00000460 000000000000831c TEXT DATA
 16 .fini         0000000c 000000000000877c TEXT DATA
 17 .rodata       00000034 0000000000008788 DATA
 18 .ARM.exidx    00000008 00000000000087bc
 19 .eh_frame     00000004 00000000000087c4 DATA
 20 .init_array   00000004 00000000000107c8
 21 .fini_array   00000004 00000000000107cc
 22 .jcr          00000004 00000000000107d0 DATA
 23 .dynamic      000000f0 00000000000107d4
 24 .got          00000020 00000000000108c4 DATA
 25 .data         00000008 00000000000108e4 DATA
 26 .bss          00000004 00000000000108ec BSS
 27 .ARM.attributes 0000002e 0000000000000000
 28 .comment      0000001c 0000000000000000
 29 .shstrtab     0000010a 0000000000000000
 30 .symtab       00000650 0000000000000000
 31 .strtab       0000024e 0000000000000000
Debug+Asserts/bin/fracture-cl> sym .text
SYMBOL TABLE FOR SECTION .text at 0x0000831c
00000000                00000000 00000000
00008358                00000000 00008358 $a
00008358 l     F        00000000 00008358 call_gmon_start
0000837c                00000000 0000837c $d
0000834c                00000000 0000834c $d
000082d0                00000000 000082d0 $a
00008780                00000000 00008780 $a
000108e8                00000000 000108e8 $d
00008384                00000000 00008384 $a
00008384 l     F        00000000 00008384 __do_global_dtors_aux
0000839c                00000000 0000839c $d
000083a0                00000000 000083a0 $a
000083a0 l     F        00000000 000083a0 frame_dummy
000083cc                00000000 000083cc $d
0000878c                00000000 0000878c $d
000083d4                00000000 000083d4 $a
000084c0                00000000 000084c0 $d
000084cc                00000000 000084cc $a
00008708                00000000 00008708 $a
00008770                00000000 00008770 $d
000107cc                00000000 000107cc __init_array_end
000082e8                00000000 000082e8 $d
000082ec                00000000 000082ec $a
00000000 l     F        00000000 000082ec abort@@GLIBC_2.4
00008708 l     F        00000004 00008708 __libc_csu_fini
00000000 l     F        00000000 000082f8 __libc_start_main@@GLIBC_2.4
00000000                00000000 00000000 __gmon_start__
00000000                00000000 00000000 _Jv_RegisterClasses
0000852c l     F        00000148 0000852c fastfib
00008674 l     F        00000094 00008674 fastfib_v2
000084cc l     F        00000060 000084cc fib
000108ec                00000000 000108ec __bss_start__
000087c4                00000000 000087c4 __exidx_end
000108e8 l              00000000 000108e8 __dso_handle
000108f0                00000000 000108f0 __end__
0000870c l     F        00000070 0000870c __libc_csu_init
000108f0                00000000 000108f0 __bss_end__
00000000 l     F        00000000 00008310 printf@@GLIBC_2.4
000108ec                00000000 000108ec __bss_start
000108f0                00000000 000108f0 _bss_end__
000108f0                00000000 000108f0 _end
000108ec                00000000 000108ec _edata
000087bc                00000000 000087bc __exidx_start
000083d4 l     F        000000f8 000083d4 main
Debug+Asserts/bin/fracture-cl> dis 0x83d4
Address: 33748
NumInstrs: 0
<main>:
000083D4:   10 48 2D E9                         push    {r4, r11, lr}
000083D8:   08 B0 8D E2                         add     r11, sp, #0x8
000083DC:   0C D0 4D E2                         sub     sp, sp, #0xc
000083E0:   00 30 A0 E3                         mov     r3, #0x0
000083E4:   10 30 0B E5                         str     r3, [r11, #-16]
000083E8:   0A 00 00 EA                         b       #0x28
000083EC:   CC 40 9F E5                         ldr     r4, [pc, #204]
000083F0:   10 00 1B E5                         ldr     r0, [r11, #-16]
000083F4:   34 00 00 EB                         bl      #0xd0
000083F8:   00 30 A0 E1                         mov     r3, r0
000083FC:   04 00 A0 E1                         mov     r0, r4
00008400:   10 10 1B E5                         ldr     r1, [r11, #-16]
00008404:   03 20 A0 E1                         mov     r2, r3
00008408:   C0 FF FF EB                         bl      #-0x100
0000840C:   10 30 1B E5                         ldr     r3, [r11, #-16]
00008410:   01 30 83 E2                         add     r3, r3, #0x1
00008414:   10 30 0B E5                         str     r3, [r11, #-16]
00008418:   10 30 1B E5                         ldr     r3, [r11, #-16]
0000841C:   22 00 53 E3                         cmp     r3, #0x22
00008420:   F1 FF FF 9A                         bls     #-0x3c
00008424:   00 30 A0 E3                         mov     r3, #0x0
00008428:   10 30 0B E5                         str     r3, [r11, #-16]
0000842C:   0A 00 00 EA                         b       #0x28
00008430:   8C 40 9F E5                         ldr     r4, [pc, #140]
00008434:   10 00 1B E5                         ldr     r0, [r11, #-16]
00008438:   3B 00 00 EB                         bl      #0xec
0000843C:   00 30 A0 E1                         mov     r3, r0
00008440:   04 00 A0 E1                         mov     r0, r4
00008444:   10 10 1B E5                         ldr     r1, [r11, #-16]
00008448:   03 20 A0 E1                         mov     r2, r3
0000844C:   AF FF FF EB                         bl      #-0x144
00008450:   10 30 1B E5                         ldr     r3, [r11, #-16]
00008454:   01 30 83 E2                         add     r3, r3, #0x1
00008458:   10 30 0B E5                         str     r3, [r11, #-16]
0000845C:   10 30 1B E5                         ldr     r3, [r11, #-16]
00008460:   22 00 53 E3                         cmp     r3, #0x22
00008464:   F1 FF FF 9A                         bls     #-0x3c
00008468:   00 30 A0 E3                         mov     r3, #0x0
0000846C:   10 30 0B E5                         str     r3, [r11, #-16]
00008470:   0A 00 00 EA                         b       #0x28
00008474:   4C 40 9F E5                         ldr     r4, [pc, #76]
00008478:   10 00 1B E5                         ldr     r0, [r11, #-16]
0000847C:   2A 00 00 EB                         bl      #0xa8
00008480:   00 30 A0 E1                         mov     r3, r0
00008484:   04 00 A0 E1                         mov     r0, r4
00008488:   10 10 1B E5                         ldr     r1, [r11, #-16]
0000848C:   03 20 A0 E1                         mov     r2, r3
00008490:   9E FF FF EB                         bl      #-0x188
00008494:   10 30 1B E5                         ldr     r3, [r11, #-16]
00008498:   01 30 83 E2                         add     r3, r3, #0x1
0000849C:   10 30 0B E5                         str     r3, [r11, #-16]
000084A0:   10 30 1B E5                         ldr     r3, [r11, #-16]
000084A4:   22 00 53 E3                         cmp     r3, #0x22
000084A8:   F1 FF FF 9A                         bls     #-0x3c
000084AC:   00 30 A0 E3                         mov     r3, #0x0
000084B0:   03 00 A0 E1                         mov     r0, r3
000084B4:   08 D0 4B E2                         sub     sp, r11, #0x8
000084B8:   10 48 BD E8                         pop     {r4, r11, lr}
000084BC:   1E FF 2F E1                         bx      lr
Debug+Asserts/bin/fracture-cl> dec 0x83d4
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
Null values on CopyToReg, skipping!
>  %CPSR = alloca i32
=>  %CPSR5 = load i32* %CPSR, !dbg !116
==>  store i32 %CPSR5, i32* @CPSR, !dbg !116
=>  %CPSR4 = load i32* %CPSR, !dbg !106
=>  %CPSR3 = load i32* %CPSR, !dbg !72
=>  %CPSR2 = load i32* %CPSR, !dbg !38
=>  store i32 %CPSR1, i32* %CPSR
>  %R2 = alloca i32
=>  %R2_2 = load i32* %R2, !dbg !116
==>  store i32 %R2_2, i32* @R2, !dbg !116
=>  store i32 %R3_15, i32* %R2, !dbg !92
=>  store i32 %R3_9, i32* %R2, !dbg !58
=>  store i32 %R3_3, i32* %R2, !dbg !24
=>  store i32 %R2_1, i32* %R2
>  %R1 = alloca i32
=>  %R1_2 = load i32* %R1, !dbg !116
==>  store i32 %R1_2, i32* @R1, !dbg !116
=>  store i32 %R11_58, i32* %R1, !dbg !90
=>  store i32 %R11_36, i32* %R1, !dbg !56
=>  store i32 %R11_14, i32* %R1, !dbg !22
=>  store i32 %R1_1, i32* %R1
>  %R0 = alloca i32
=>  %R0_5 = load i32* %R0, !dbg !116
==>  store i32 %R0_5, i32* @R0, !dbg !116
=>  store i32 %R3_20, i32* %R0, !dbg !110
=>  store i32 %R4_5, i32* %R0, !dbg !88
=>  %R0_4 = load i32* %R0, !dbg !86
==>  store i32 %R0_4, i32* %R3, !dbg !86
=>  store i32 %R11_54, i32* %R0, !dbg !82
=>  store i32 %R4_4, i32* %R0, !dbg !54
=>  %R0_3 = load i32* %R0, !dbg !52
==>  store i32 %R0_3, i32* %R3, !dbg !52
=>  store i32 %R11_32, i32* %R0, !dbg !48
=>  store i32 %R4_3, i32* %R0, !dbg !20
=>  %R0_2 = load i32* %R0, !dbg !18
==>  store i32 %R0_2, i32* %R3, !dbg !18
=>  store i32 %R11_10, i32* %R0, !dbg !14
=>  store i32 %R0_1, i32* %R0
>  %PC = alloca i32
=>  %PC14 = load i32* %PC, !dbg !116
==>  store i32 %PC14, i32* @PC, !dbg !116
=>  %PC10 = load i32* %PC, !dbg !80
==>  %PC11 = add i32 %PC10, 76, !dbg !80
===>  %PC12 = inttoptr i32 %PC11 to i32*, !dbg !80
====>  %PC13 = load i32* %PC12, !dbg !80
=====>  store i32 %PC13, i32* %R4, !dbg !80
=>  %PC6 = load i32* %PC, !dbg !46
==>  %PC7 = add i32 %PC6, 140, !dbg !46
===>  %PC8 = inttoptr i32 %PC7 to i32*, !dbg !46
====>  %PC9 = load i32* %PC8, !dbg !46
=====>  store i32 %PC9, i32* %R4, !dbg !46
=>  %PC2 = load i32* %PC, !dbg !12
==>  %PC3 = add i32 %PC2, 204, !dbg !12
===>  %PC4 = inttoptr i32 %PC3 to i32*, !dbg !12
====>  %PC5 = load i32* %PC4, !dbg !12
=====>  store i32 %PC5, i32* %R4, !dbg !12
=>  store i32 %PC1, i32* %PC
>  %R3 = alloca i32
=>  %R3_21 = load i32* %R3, !dbg !116
==>  store i32 %R3_21, i32* @R3, !dbg !116
=>  %R3_20 = load i32* %R3, !dbg !110
==>  store i32 %R3_20, i32* %R0, !dbg !110
=>  store i32 0, i32* %R3, !dbg !108
=>  %R3_19 = load i32* %R3, !dbg !104
==>  %2 = icmp ule i32 %R3_19, 34, !dbg !106
===>  br i1 %2, label %"main+160", label %"main+216", !dbg !106
=>  store i32 %R11_69, i32* %R3, !dbg !102
=>  %R3_18 = load i32* %R3, !dbg !100
==>  store i32 %R3_18, i32* %R11_65, !dbg !100
=>  store i32 %R3_17, i32* %R3, !dbg !98
=>  %R3_16 = load i32* %R3, !dbg !98
==>  %R3_17 = add i32 %R3_16, 1, !dbg !98
===>  store i32 %R3_17, i32* %R3, !dbg !98
=>  store i32 %R11_62, i32* %R3, !dbg !96
=>  %R3_15 = load i32* %R3, !dbg !92
==>  store i32 %R3_15, i32* %R2, !dbg !92
=>  store i32 %R0_4, i32* %R3, !dbg !86
=>  %R3_14 = load i32* %R3, !dbg !76
==>  store i32 %R3_14, i32* %R11_50, !dbg !76
=>  store i32 0, i32* %R3, !dbg !74
=>  %R3_13 = load i32* %R3, !dbg !70
==>  %1 = icmp ule i32 %R3_13, 34, !dbg !72
===>  br i1 %1, label %"main+92", label %"main+148", !dbg !72
=>  store i32 %R11_47, i32* %R3, !dbg !68
=>  %R3_12 = load i32* %R3, !dbg !66
==>  store i32 %R3_12, i32* %R11_43, !dbg !66
=>  store i32 %R3_11, i32* %R3, !dbg !64
=>  %R3_10 = load i32* %R3, !dbg !64
==>  %R3_11 = add i32 %R3_10, 1, !dbg !64
===>  store i32 %R3_11, i32* %R3, !dbg !64
=>  store i32 %R11_40, i32* %R3, !dbg !62
=>  %R3_9 = load i32* %R3, !dbg !58
==>  store i32 %R3_9, i32* %R2, !dbg !58
=>  store i32 %R0_3, i32* %R3, !dbg !52
=>  %R3_8 = load i32* %R3, !dbg !42
==>  store i32 %R3_8, i32* %R11_28, !dbg !42
=>  store i32 0, i32* %R3, !dbg !40
=>  %R3_7 = load i32* %R3, !dbg !36
==>  %0 = icmp ule i32 %R3_7, 34, !dbg !38
===>  br i1 %0, label %"main+24", label %"main+80", !dbg !38
=>  store i32 %R11_25, i32* %R3, !dbg !34
=>  %R3_6 = load i32* %R3, !dbg !32
==>  store i32 %R3_6, i32* %R11_21, !dbg !32
=>  store i32 %R3_5, i32* %R3, !dbg !30
=>  %R3_4 = load i32* %R3, !dbg !30
==>  %R3_5 = add i32 %R3_4, 1, !dbg !30
===>  store i32 %R3_5, i32* %R3, !dbg !30
=>  store i32 %R11_18, i32* %R3, !dbg !28
=>  %R3_3 = load i32* %R3, !dbg !24
==>  store i32 %R3_3, i32* %R2, !dbg !24
=>  store i32 %R0_2, i32* %R3, !dbg !18
=>  %R3_2 = load i32* %R3, !dbg !8
==>  store i32 %R3_2, i32* %R11_6, !dbg !8
=>  store i32 0, i32* %R3, !dbg !6
=>  store i32 %R3_1, i32* %R3
>  %LR = alloca i32
=>  %LR3 = load i32* %LR, !dbg !116
==>  store i32 %LR3, i32* @LR, !dbg !116
=>  store i32 %SP22, i32* %LR, !dbg !114
=>  %LR2 = load i32* %LR, !dbg !0
==>  store i32 %LR2, i32* %SP7, !dbg !0
=>  store i32 %LR1, i32* %LR
>  %R11 = alloca i32
=>  %R11_71 = load i32* %R11, !dbg !116
==>  store i32 %R11_71, i32* @R11, !dbg !116
=>  store i32 %SP19, i32* %R11, !dbg !114
=>  %R11_70 = load i32* %R11, !dbg !112
==>  %SP12 = sub i32 %R11_70, 8, !dbg !112
===>  store i32 %SP12, i32* %SP, !dbg !112
=>  %R11_66 = load i32* %R11, !dbg !102
==>  %R11_67 = add i32 %R11_66, -16, !dbg !102
===>  %R11_68 = inttoptr i32 %R11_67 to i32*, !dbg !102
====>  %R11_69 = load i32* %R11_68, !dbg !102
=====>  store i32 %R11_69, i32* %R3, !dbg !102
=>  %R11_63 = load i32* %R11, !dbg !100
==>  %R11_64 = add i32 %R11_63, -16, !dbg !100
===>  %R11_65 = inttoptr i32 %R11_64 to i32*, !dbg !100
====>  store i32 %R3_18, i32* %R11_65, !dbg !100
=>  %R11_59 = load i32* %R11, !dbg !96
==>  %R11_60 = add i32 %R11_59, -16, !dbg !96
===>  %R11_61 = inttoptr i32 %R11_60 to i32*, !dbg !96
====>  %R11_62 = load i32* %R11_61, !dbg !96
=====>  store i32 %R11_62, i32* %R3, !dbg !96
=>  %R11_55 = load i32* %R11, !dbg !90
==>  %R11_56 = add i32 %R11_55, -16, !dbg !90
===>  %R11_57 = inttoptr i32 %R11_56 to i32*, !dbg !90
====>  %R11_58 = load i32* %R11_57, !dbg !90
=====>  store i32 %R11_58, i32* %R1, !dbg !90
=>  %R11_51 = load i32* %R11, !dbg !82
==>  %R11_52 = add i32 %R11_51, -16, !dbg !82
===>  %R11_53 = inttoptr i32 %R11_52 to i32*, !dbg !82
====>  %R11_54 = load i32* %R11_53, !dbg !82
=====>  store i32 %R11_54, i32* %R0, !dbg !82
=>  %R11_48 = load i32* %R11, !dbg !76
==>  %R11_49 = add i32 %R11_48, -16, !dbg !76
===>  %R11_50 = inttoptr i32 %R11_49 to i32*, !dbg !76
====>  store i32 %R3_14, i32* %R11_50, !dbg !76
=>  %R11_44 = load i32* %R11, !dbg !68
==>  %R11_45 = add i32 %R11_44, -16, !dbg !68
===>  %R11_46 = inttoptr i32 %R11_45 to i32*, !dbg !68
====>  %R11_47 = load i32* %R11_46, !dbg !68
=====>  store i32 %R11_47, i32* %R3, !dbg !68
=>  %R11_41 = load i32* %R11, !dbg !66
==>  %R11_42 = add i32 %R11_41, -16, !dbg !66
===>  %R11_43 = inttoptr i32 %R11_42 to i32*, !dbg !66
====>  store i32 %R3_12, i32* %R11_43, !dbg !66
=>  %R11_37 = load i32* %R11, !dbg !62
==>  %R11_38 = add i32 %R11_37, -16, !dbg !62
===>  %R11_39 = inttoptr i32 %R11_38 to i32*, !dbg !62
====>  %R11_40 = load i32* %R11_39, !dbg !62
=====>  store i32 %R11_40, i32* %R3, !dbg !62
=>  %R11_33 = load i32* %R11, !dbg !56
==>  %R11_34 = add i32 %R11_33, -16, !dbg !56
===>  %R11_35 = inttoptr i32 %R11_34 to i32*, !dbg !56
====>  %R11_36 = load i32* %R11_35, !dbg !56
=====>  store i32 %R11_36, i32* %R1, !dbg !56
=>  %R11_29 = load i32* %R11, !dbg !48
==>  %R11_30 = add i32 %R11_29, -16, !dbg !48
===>  %R11_31 = inttoptr i32 %R11_30 to i32*, !dbg !48
====>  %R11_32 = load i32* %R11_31, !dbg !48
=====>  store i32 %R11_32, i32* %R0, !dbg !48
=>  %R11_26 = load i32* %R11, !dbg !42
==>  %R11_27 = add i32 %R11_26, -16, !dbg !42
===>  %R11_28 = inttoptr i32 %R11_27 to i32*, !dbg !42
====>  store i32 %R3_8, i32* %R11_28, !dbg !42
=>  %R11_22 = load i32* %R11, !dbg !34
==>  %R11_23 = add i32 %R11_22, -16, !dbg !34
===>  %R11_24 = inttoptr i32 %R11_23 to i32*, !dbg !34
====>  %R11_25 = load i32* %R11_24, !dbg !34
=====>  store i32 %R11_25, i32* %R3, !dbg !34
=>  %R11_19 = load i32* %R11, !dbg !32
==>  %R11_20 = add i32 %R11_19, -16, !dbg !32
===>  %R11_21 = inttoptr i32 %R11_20 to i32*, !dbg !32
====>  store i32 %R3_6, i32* %R11_21, !dbg !32
=>  %R11_15 = load i32* %R11, !dbg !28
==>  %R11_16 = add i32 %R11_15, -16, !dbg !28
===>  %R11_17 = inttoptr i32 %R11_16 to i32*, !dbg !28
====>  %R11_18 = load i32* %R11_17, !dbg !28
=====>  store i32 %R11_18, i32* %R3, !dbg !28
=>  %R11_11 = load i32* %R11, !dbg !22
==>  %R11_12 = add i32 %R11_11, -16, !dbg !22
===>  %R11_13 = inttoptr i32 %R11_12 to i32*, !dbg !22
====>  %R11_14 = load i32* %R11_13, !dbg !22
=====>  store i32 %R11_14, i32* %R1, !dbg !22
=>  %R11_7 = load i32* %R11, !dbg !14
==>  %R11_8 = add i32 %R11_7, -16, !dbg !14
===>  %R11_9 = inttoptr i32 %R11_8 to i32*, !dbg !14
====>  %R11_10 = load i32* %R11_9, !dbg !14
=====>  store i32 %R11_10, i32* %R0, !dbg !14
=>  %R11_4 = load i32* %R11, !dbg !8
==>  %R11_5 = add i32 %R11_4, -16, !dbg !8
===>  %R11_6 = inttoptr i32 %R11_5 to i32*, !dbg !8
====>  store i32 %R3_2, i32* %R11_6, !dbg !8
=>  store i32 %R11_3, i32* %R11, !dbg !2
=>  %R11_2 = load i32* %R11, !dbg !0
==>  store i32 %R11_2, i32* %SP5, !dbg !0
=>  store i32 %R11_1, i32* %R11
>  %R4 = alloca i32
=>  %R4_6 = load i32* %R4, !dbg !116
==>  store i32 %R4_6, i32* @R4, !dbg !116
=>  store i32 %SP16, i32* %R4, !dbg !114
=>  %R4_5 = load i32* %R4, !dbg !88
==>  store i32 %R4_5, i32* %R0, !dbg !88
=>  store i32 %PC13, i32* %R4, !dbg !80
=>  %R4_4 = load i32* %R4, !dbg !54
==>  store i32 %R4_4, i32* %R0, !dbg !54
=>  store i32 %PC9, i32* %R4, !dbg !46
=>  %R4_3 = load i32* %R4, !dbg !20
==>  store i32 %R4_3, i32* %R0, !dbg !20
=>  store i32 %PC5, i32* %R4, !dbg !12
=>  %R4_2 = load i32* %R4, !dbg !0
==>  store i32 %R4_2, i32* %SP3, !dbg !0
=>  store i32 %R4_1, i32* %R4
>  %SP = alloca i32
=>  %SP23 = load i32* %SP, !dbg !116
==>  store i32 %SP23, i32* @SP, !dbg !116
=>  store i32 %SP20, i32* %SP, !dbg !114
=>  %SP13 = load i32* %SP, !dbg !114
==>  %SP14 = add i32 %SP13, 4, !dbg !114
===>  %SP17 = add i32 %SP14, 4, !dbg !114
====>  %SP20 = add i32 %SP17, 4, !dbg !114
=====>  store i32 %SP20, i32* %SP, !dbg !114
=====>  %SP21 = inttoptr i32 %SP20 to i32*, !dbg !114
======>  %SP22 = load i32* %SP21, !dbg !114
=======>  store i32 %SP22, i32* %LR, !dbg !114
====>  %SP18 = inttoptr i32 %SP17 to i32*, !dbg !114
=====>  %SP19 = load i32* %SP18, !dbg !114
======>  store i32 %SP19, i32* %R11, !dbg !114
===>  %SP15 = inttoptr i32 %SP14 to i32*, !dbg !114
====>  %SP16 = load i32* %SP15, !dbg !114
=====>  store i32 %SP16, i32* %R4, !dbg !114
=>  store i32 %SP12, i32* %SP, !dbg !112
=>  store i32 %SP11, i32* %SP, !dbg !4
=>  %SP10 = load i32* %SP, !dbg !4
==>  %SP11 = sub i32 %SP10, 12, !dbg !4
===>  store i32 %SP11, i32* %SP, !dbg !4
=>  %SP9 = load i32* %SP, !dbg !2
==>  %R11_3 = add i32 %SP9, 8, !dbg !2
===>  store i32 %R11_3, i32* %R11, !dbg !2
=>  store i32 %SP8, i32* %SP, !dbg !0
=>  %SP2 = load i32* %SP, !dbg !0
==>  %SP4 = sub i32 %SP2, 4, !dbg !0
===>  %SP6 = sub i32 %SP4, 4, !dbg !0
====>  %SP8 = sub i32 %SP6, 4, !dbg !0
=====>  store i32 %SP8, i32* %SP, !dbg !0
====>  %SP7 = inttoptr i32 %SP6 to i32*, !dbg !0
=====>  store i32 %LR2, i32* %SP7, !dbg !0
===>  %SP5 = inttoptr i32 %SP4 to i32*, !dbg !0
====>  store i32 %R11_2, i32* %SP5, !dbg !0
==>  %SP3 = inttoptr i32 %SP2 to i32*, !dbg !0
===>  store i32 %R4_2, i32* %SP3, !dbg !0
=>  store i32 %SP1, i32* %SP
Num Loads and Stores: 27

define void @main() {
entry:
  %CPSR = alloca i32
  %CPSR1 = load i32* @CPSR
  store i32 %CPSR1, i32* %CPSR
  %R2 = alloca i32
  %R2_1 = load i32* @R2
  store i32 %R2_1, i32* %R2
  %R1 = alloca i32
  %R1_1 = load i32* @R1
  store i32 %R1_1, i32* %R1
  %R0 = alloca i32
  %R0_1 = load i32* @R0
  store i32 %R0_1, i32* %R0
  %PC = alloca i32
  %PC1 = load i32* @PC
  store i32 %PC1, i32* %PC
  %R3 = alloca i32
  %R3_1 = load i32* @R3
  store i32 %R3_1, i32* %R3
  %LR = alloca i32
  %LR1 = load i32* @LR
  store i32 %LR1, i32* %LR
  %R11 = alloca i32
  %R11_1 = load i32* @R11
  store i32 %R11_1, i32* %R11
  %R4 = alloca i32
  %R4_1 = load i32* @R4
  store i32 %R4_1, i32* %R4
  %SP = alloca i32
  %SP1 = load i32* @SP
  store i32 %SP1, i32* %SP
  br label %"main+0"

"main+0":                                         ; preds = %entry
  %SP2 = load i32* %SP, !dbg !0
  %R4_2 = load i32* %R4, !dbg !0
  %R11_2 = load i32* %R11, !dbg !0
  %LR2 = load i32* %LR, !dbg !0
  %SP3 = inttoptr i32 %SP2 to i32*, !dbg !0
  store i32 %R4_2, i32* %SP3, !dbg !0
  %SP4 = sub i32 %SP2, 4, !dbg !0
  %SP5 = inttoptr i32 %SP4 to i32*, !dbg !0
  store i32 %R11_2, i32* %SP5, !dbg !0
  %SP6 = sub i32 %SP4, 4, !dbg !0
  %SP7 = inttoptr i32 %SP6 to i32*, !dbg !0
  store i32 %LR2, i32* %SP7, !dbg !0
  %SP8 = sub i32 %SP6, 4, !dbg !0
  store i32 %SP8, i32* %SP, !dbg !0
  %SP9 = load i32* %SP, !dbg !2
  %R11_3 = add i32 %SP9, 8, !dbg !2
  store i32 %R11_3, i32* %R11, !dbg !2
  %SP10 = load i32* %SP, !dbg !4
  %SP11 = sub i32 %SP10, 12, !dbg !4
  store i32 %SP11, i32* %SP, !dbg !4
  store i32 0, i32* %R3, !dbg !6
  %R3_2 = load i32* %R3, !dbg !8
  %R11_4 = load i32* %R11, !dbg !8
  %R11_5 = add i32 %R11_4, -16, !dbg !8
  %R11_6 = inttoptr i32 %R11_5 to i32*, !dbg !8
  store i32 %R3_2, i32* %R11_6, !dbg !8
  br label %"main+68", !dbg !10

"main+24":                                        ; preds = %"main+68"
  %PC2 = load i32* %PC, !dbg !12
  %PC3 = add i32 %PC2, 204, !dbg !12
  %PC4 = inttoptr i32 %PC3 to i32*, !dbg !12
  %PC5 = load i32* %PC4, !dbg !12
  store i32 %PC5, i32* %R4, !dbg !12
  %R11_7 = load i32* %R11, !dbg !14
  %R11_8 = add i32 %R11_7, -16, !dbg !14
  %R11_9 = inttoptr i32 %R11_8 to i32*, !dbg !14
  %R11_10 = load i32* %R11_9, !dbg !14
  store i32 %R11_10, i32* %R0, !dbg !14
  call void @fib(), !dbg !16
  %R0_2 = load i32* %R0, !dbg !18
  store i32 %R0_2, i32* %R3, !dbg !18
  %R4_3 = load i32* %R4, !dbg !20
  store i32 %R4_3, i32* %R0, !dbg !20
  %R11_11 = load i32* %R11, !dbg !22
  %R11_12 = add i32 %R11_11, -16, !dbg !22
  %R11_13 = inttoptr i32 %R11_12 to i32*, !dbg !22
  %R11_14 = load i32* %R11_13, !dbg !22
  store i32 %R11_14, i32* %R1, !dbg !22
  %R3_3 = load i32* %R3, !dbg !24
  store i32 %R3_3, i32* %R2, !dbg !24
  call void @func_8310(), !dbg !26
  %R11_15 = load i32* %R11, !dbg !28
  %R11_16 = add i32 %R11_15, -16, !dbg !28
  %R11_17 = inttoptr i32 %R11_16 to i32*, !dbg !28
  %R11_18 = load i32* %R11_17, !dbg !28
  store i32 %R11_18, i32* %R3, !dbg !28
  %R3_4 = load i32* %R3, !dbg !30
  %R3_5 = add i32 %R3_4, 1, !dbg !30
  store i32 %R3_5, i32* %R3, !dbg !30
  %R3_6 = load i32* %R3, !dbg !32
  %R11_19 = load i32* %R11, !dbg !32
  %R11_20 = add i32 %R11_19, -16, !dbg !32
  %R11_21 = inttoptr i32 %R11_20 to i32*, !dbg !32
  store i32 %R3_6, i32* %R11_21, !dbg !32
  br label %"main+68", !dbg !32

"main+68":                                        ; preds = %"main+24", %"main+0"
  %R11_22 = load i32* %R11, !dbg !34
  %R11_23 = add i32 %R11_22, -16, !dbg !34
  %R11_24 = inttoptr i32 %R11_23 to i32*, !dbg !34
  %R11_25 = load i32* %R11_24, !dbg !34
  store i32 %R11_25, i32* %R3, !dbg !34
  %R3_7 = load i32* %R3, !dbg !36
  %CPSR2 = load i32* %CPSR, !dbg !38
  %0 = icmp ule i32 %R3_7, 34, !dbg !38
  br i1 %0, label %"main+24", label %"main+80", !dbg !38

"main+80":                                        ; preds = %"main+68"
  store i32 0, i32* %R3, !dbg !40
  %R3_8 = load i32* %R3, !dbg !42
  %R11_26 = load i32* %R11, !dbg !42
  %R11_27 = add i32 %R11_26, -16, !dbg !42
  %R11_28 = inttoptr i32 %R11_27 to i32*, !dbg !42
  store i32 %R3_8, i32* %R11_28, !dbg !42
  br label %"main+136", !dbg !44

"main+92":                                        ; preds = %"main+136"
  %PC6 = load i32* %PC, !dbg !46
  %PC7 = add i32 %PC6, 140, !dbg !46
  %PC8 = inttoptr i32 %PC7 to i32*, !dbg !46
  %PC9 = load i32* %PC8, !dbg !46
  store i32 %PC9, i32* %R4, !dbg !46
  %R11_29 = load i32* %R11, !dbg !48
  %R11_30 = add i32 %R11_29, -16, !dbg !48
  %R11_31 = inttoptr i32 %R11_30 to i32*, !dbg !48
  %R11_32 = load i32* %R11_31, !dbg !48
  store i32 %R11_32, i32* %R0, !dbg !48
  call void @fastfib(), !dbg !50
  %R0_3 = load i32* %R0, !dbg !52
  store i32 %R0_3, i32* %R3, !dbg !52
  %R4_4 = load i32* %R4, !dbg !54
  store i32 %R4_4, i32* %R0, !dbg !54
  %R11_33 = load i32* %R11, !dbg !56
  %R11_34 = add i32 %R11_33, -16, !dbg !56
  %R11_35 = inttoptr i32 %R11_34 to i32*, !dbg !56
  %R11_36 = load i32* %R11_35, !dbg !56
  store i32 %R11_36, i32* %R1, !dbg !56
  %R3_9 = load i32* %R3, !dbg !58
  store i32 %R3_9, i32* %R2, !dbg !58
  call void @func_8310(), !dbg !60
  %R11_37 = load i32* %R11, !dbg !62
  %R11_38 = add i32 %R11_37, -16, !dbg !62
  %R11_39 = inttoptr i32 %R11_38 to i32*, !dbg !62
  %R11_40 = load i32* %R11_39, !dbg !62
  store i32 %R11_40, i32* %R3, !dbg !62
  %R3_10 = load i32* %R3, !dbg !64
  %R3_11 = add i32 %R3_10, 1, !dbg !64
  store i32 %R3_11, i32* %R3, !dbg !64
  %R3_12 = load i32* %R3, !dbg !66
  %R11_41 = load i32* %R11, !dbg !66
  %R11_42 = add i32 %R11_41, -16, !dbg !66
  %R11_43 = inttoptr i32 %R11_42 to i32*, !dbg !66
  store i32 %R3_12, i32* %R11_43, !dbg !66
  br label %"main+136", !dbg !66

"main+136":                                       ; preds = %"main+92", %"main+80"
  %R11_44 = load i32* %R11, !dbg !68
  %R11_45 = add i32 %R11_44, -16, !dbg !68
  %R11_46 = inttoptr i32 %R11_45 to i32*, !dbg !68
  %R11_47 = load i32* %R11_46, !dbg !68
  store i32 %R11_47, i32* %R3, !dbg !68
  %R3_13 = load i32* %R3, !dbg !70
  %CPSR3 = load i32* %CPSR, !dbg !72
  %1 = icmp ule i32 %R3_13, 34, !dbg !72
  br i1 %1, label %"main+92", label %"main+148", !dbg !72

"main+148":                                       ; preds = %"main+136"
  store i32 0, i32* %R3, !dbg !74
  %R3_14 = load i32* %R3, !dbg !76
  %R11_48 = load i32* %R11, !dbg !76
  %R11_49 = add i32 %R11_48, -16, !dbg !76
  %R11_50 = inttoptr i32 %R11_49 to i32*, !dbg !76
  store i32 %R3_14, i32* %R11_50, !dbg !76
  br label %"main+204", !dbg !78

"main+160":                                       ; preds = %"main+204"
  %PC10 = load i32* %PC, !dbg !80
  %PC11 = add i32 %PC10, 76, !dbg !80
  %PC12 = inttoptr i32 %PC11 to i32*, !dbg !80
  %PC13 = load i32* %PC12, !dbg !80
  store i32 %PC13, i32* %R4, !dbg !80
  %R11_51 = load i32* %R11, !dbg !82
  %R11_52 = add i32 %R11_51, -16, !dbg !82
  %R11_53 = inttoptr i32 %R11_52 to i32*, !dbg !82
  %R11_54 = load i32* %R11_53, !dbg !82
  store i32 %R11_54, i32* %R0, !dbg !82
  call void @fastfib(), !dbg !84
  %R0_4 = load i32* %R0, !dbg !86
  store i32 %R0_4, i32* %R3, !dbg !86
  %R4_5 = load i32* %R4, !dbg !88
  store i32 %R4_5, i32* %R0, !dbg !88
  %R11_55 = load i32* %R11, !dbg !90
  %R11_56 = add i32 %R11_55, -16, !dbg !90
  %R11_57 = inttoptr i32 %R11_56 to i32*, !dbg !90
  %R11_58 = load i32* %R11_57, !dbg !90
  store i32 %R11_58, i32* %R1, !dbg !90
  %R3_15 = load i32* %R3, !dbg !92
  store i32 %R3_15, i32* %R2, !dbg !92
  call void @func_8310(), !dbg !94
  %R11_59 = load i32* %R11, !dbg !96
  %R11_60 = add i32 %R11_59, -16, !dbg !96
  %R11_61 = inttoptr i32 %R11_60 to i32*, !dbg !96
  %R11_62 = load i32* %R11_61, !dbg !96
  store i32 %R11_62, i32* %R3, !dbg !96
  %R3_16 = load i32* %R3, !dbg !98
  %R3_17 = add i32 %R3_16, 1, !dbg !98
  store i32 %R3_17, i32* %R3, !dbg !98
  %R3_18 = load i32* %R3, !dbg !100
  %R11_63 = load i32* %R11, !dbg !100
  %R11_64 = add i32 %R11_63, -16, !dbg !100
  %R11_65 = inttoptr i32 %R11_64 to i32*, !dbg !100
  store i32 %R3_18, i32* %R11_65, !dbg !100
  br label %"main+204", !dbg !100

"main+204":                                       ; preds = %"main+160", %"main+148"
  %R11_66 = load i32* %R11, !dbg !102
  %R11_67 = add i32 %R11_66, -16, !dbg !102
  %R11_68 = inttoptr i32 %R11_67 to i32*, !dbg !102
  %R11_69 = load i32* %R11_68, !dbg !102
  store i32 %R11_69, i32* %R3, !dbg !102
  %R3_19 = load i32* %R3, !dbg !104
  %CPSR4 = load i32* %CPSR, !dbg !106
  %2 = icmp ule i32 %R3_19, 34, !dbg !106
  br i1 %2, label %"main+160", label %"main+216", !dbg !106

"main+216":                                       ; preds = %"main+204"
  store i32 0, i32* %R3, !dbg !108
  %R3_20 = load i32* %R3, !dbg !110
  store i32 %R3_20, i32* %R0, !dbg !110
  %R11_70 = load i32* %R11, !dbg !112
  %SP12 = sub i32 %R11_70, 8, !dbg !112
  store i32 %SP12, i32* %SP, !dbg !112
  %SP13 = load i32* %SP, !dbg !114
  %SP14 = add i32 %SP13, 4, !dbg !114
  %SP15 = inttoptr i32 %SP14 to i32*, !dbg !114
  %SP16 = load i32* %SP15, !dbg !114
  store i32 %SP16, i32* %R4, !dbg !114
  %SP17 = add i32 %SP14, 4, !dbg !114
  %SP18 = inttoptr i32 %SP17 to i32*, !dbg !114
  %SP19 = load i32* %SP18, !dbg !114
  store i32 %SP19, i32* %R11, !dbg !114
  %SP20 = add i32 %SP17, 4, !dbg !114
  %SP21 = inttoptr i32 %SP20 to i32*, !dbg !114
  %SP22 = load i32* %SP21, !dbg !114
  store i32 %SP22, i32* %LR, !dbg !114
  store i32 %SP20, i32* %SP, !dbg !114
  %CPSR5 = load i32* %CPSR, !dbg !116
  store i32 %CPSR5, i32* @CPSR, !dbg !116
  %LR3 = load i32* %LR, !dbg !116
  store i32 %LR3, i32* @LR, !dbg !116
  %PC14 = load i32* %PC, !dbg !116
  store i32 %PC14, i32* @PC, !dbg !116
  %SP23 = load i32* %SP, !dbg !116
  store i32 %SP23, i32* @SP, !dbg !116
  %R0_5 = load i32* %R0, !dbg !116
  store i32 %R0_5, i32* @R0, !dbg !116
  %R1_2 = load i32* %R1, !dbg !116
  store i32 %R1_2, i32* @R1, !dbg !116
  %R2_2 = load i32* %R2, !dbg !116
  store i32 %R2_2, i32* @R2, !dbg !116
  %R3_21 = load i32* %R3, !dbg !116
  store i32 %R3_21, i32* @R3, !dbg !116
  %R4_6 = load i32* %R4, !dbg !116
  store i32 %R4_6, i32* @R4, !dbg !116
  %R11_71 = load i32* %R11, !dbg !116
  store i32 %R11_71, i32* @R11, !dbg !116
  ret void, !dbg !116
}
Debug+Asserts/bin/fracture-cl> quit
```
