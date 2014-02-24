INSTALLATION INSTRUCTIONS


200763

Unpacking and installation
--------------------------

1.  Unpacking the archive (if you have not done so already).

    On Unix and Linux (in a terminal window):

        cd your-install-dir
        gunzip fracture.tar.gz
        tar xvf fracture.tar

    This creates the subdirectory some-package containing the files.


2.  Configuring the package.

    The configure script is called "configure" on unix/Linux. It should be run
    from a command line after cd'ing to the package directory.

    The configure script will prompt you in some cases for further
    information. Answer these questions and carefully read the license text
    before accepting the license conditions. The package cannot be used if
    you do not accept the license conditions.

    Example configure with llvm source and object path configured:

      configure --enable-debug-symbols --with-llvmsrc=/opt/llvm-trunk \
        --with-llvmobj=/opt/llvm-trunk

    NOTE: On Ubuntu and other systems we have had to add the following flag:

      --disable-optimized

    

3.  Building the component and examples (when required).

        make


Usage Example
-------------

$ fracture -mattr=v6 ./samples/arm/fib_armel
MCDirector: Using Triple: arm-unknown-unknown
MCDirector: Using CPU: generic
MCDirector: Using Features: +v6
Disassembler: Setting Section .text
fracture> sym .text
SYMBOL TABLE FOR SECTION .text at 0x0000831c
0000831c l    d         00000000 0000831c .text
00008358                00000000 00008358 $a
00008358 l     F        00000000 00008358 call_gmon_start
0000837c                00000000 0000837c $d
0000831c                00000000 0000831c $a
0000834c                00000000 0000834c $d
00008384                00000000 00008384 $a
00008384 l     F        00000000 00008384 __do_global_dtors_aux
0000839c                00000000 0000839c $d
000083a0                00000000 000083a0 $a
000083a0 l     F        00000000 000083a0 frame_dummy
000083cc                00000000 000083cc $d
000083d4                00000000 000083d4 $a
000084c0                00000000 000084c0 $d
000084cc                00000000 000084cc $a
00008708                00000000 00008708 $a
00008770                00000000 00008770 $d
00008708 g     F        00000004 00008708 __libc_csu_fini
0000831c g     F        00000000 0000831c _start
0000852c g     F        00000148 0000852c fastfib
00008674 g     F        00000094 00008674 fastfib_v2
000084cc g     F        00000060 000084cc fib
0000870c g     F        00000070 0000870c __libc_csu_init
000083d4 g     F        000000f8 000083d4 main
fracture> dis 0x852c
Address: 34092
NumInstrs: 0
34092
<fastfib>:
0000852C:   04 B0 2D E5                         str     r11, [sp, #-4]!
00008530:   00 B0 8D E2                         add     r11, sp, #0x0
00008534:   24 D0 4D E2                         sub     sp, sp, #0x24
00008538:   20 00 0B E5                         str     r0, [r11, #-32]
0000853C:   18 30 4B E2                         sub     r3, r11, #0x18
00008540:   0C 30 0B E5                         str     r3, [r11, #-12]
00008544:   00 30 A0 E3                         mov     r3, #0x0
00008548:   08 30 0B E5                         str     r3, [r11, #-8]
0000854C:   35 00 00 EA                         b       #0xd4
00008550:   08 30 1B E5                         ldr     r3, [r11, #-8]
00008554:   01 00 53 E3                         cmp     r3, #0x1
00008558:   03 00 00 8A                         bhi     #0xc
0000855C:   0C 30 1B E5                         ldr     r3, [r11, #-12]
00008560:   08 20 1B E5                         ldr     r2, [r11, #-8]
00008564:   00 20 83 E5                         str     r2, [r3]
00008568:   21 00 00 EA                         b       #0x84
0000856C:   18 30 4B E2                         sub     r3, r11, #0x18
00008570:   0C 20 1B E5                         ldr     r2, [r11, #-12]
00008574:   03 00 52 E1                         cmp     r2, r3
00008578:   09 00 00 1A                         bne     #0x24
0000857C:   18 30 4B E2                         sub     r3, r11, #0x18
00008580:   04 30 83 E2                         add     r3, r3, #0x4
00008584:   00 20 93 E5                         ldr     r2, [r3]
00008588:   18 30 4B E2                         sub     r3, r11, #0x18
0000858C:   08 30 83 E2                         add     r3, r3, #0x8
00008590:   00 30 93 E5                         ldr     r3, [r3]
00008594:   03 20 82 E0                         add     r2, r2, r3
00008598:   0C 30 1B E5                         ldr     r3, [r11, #-12]
0000859C:   00 20 83 E5                         str     r2, [r3]
000085A0:   13 00 00 EA                         b       #0x4c
000085A4:   18 30 4B E2                         sub     r3, r11, #0x18
000085A8:   04 30 83 E2                         add     r3, r3, #0x4
000085AC:   0C 20 1B E5                         ldr     r2, [r11, #-12]
000085B0:   03 00 52 E1                         cmp     r2, r3
000085B4:   07 00 00 1A                         bne     #0x1c
000085B8:   18 20 1B E5                         ldr     r2, [r11, #-24]
000085BC:   18 30 4B E2                         sub     r3, r11, #0x18
000085C0:   08 30 83 E2                         add     r3, r3, #0x8
000085C4:   00 30 93 E5                         ldr     r3, [r3]
000085C8:   03 20 82 E0                         add     r2, r2, r3
000085CC:   0C 30 1B E5                         ldr     r3, [r11, #-12]
000085D0:   00 20 83 E5                         str     r2, [r3]
000085D4:   06 00 00 EA                         b       #0x18
000085D8:   18 20 1B E5                         ldr     r2, [r11, #-24]
000085DC:   18 30 4B E2                         sub     r3, r11, #0x18
000085E0:   04 30 83 E2                         add     r3, r3, #0x4
000085E4:   00 30 93 E5                         ldr     r3, [r3]
000085E8:   03 20 82 E0                         add     r2, r2, r3
000085EC:   0C 30 1B E5                         ldr     r3, [r11, #-12]
000085F0:   00 20 83 E5                         str     r2, [r3]
000085F4:   0C 30 1B E5                         ldr     r3, [r11, #-12]
000085F8:   04 30 83 E2                         add     r3, r3, #0x4
000085FC:   0C 30 0B E5                         str     r3, [r11, #-12]
00008600:   18 30 4B E2                         sub     r3, r11, #0x18
00008604:   08 30 83 E2                         add     r3, r3, #0x8
00008608:   0C 20 1B E5                         ldr     r2, [r11, #-12]
0000860C:   03 00 52 E1                         cmp     r2, r3
00008610:   01 00 00 9A                         bls     #0x4
00008614:   18 30 4B E2                         sub     r3, r11, #0x18
00008618:   0C 30 0B E5                         str     r3, [r11, #-12]
0000861C:   08 30 1B E5                         ldr     r3, [r11, #-8]
00008620:   01 30 83 E2                         add     r3, r3, #0x1
00008624:   08 30 0B E5                         str     r3, [r11, #-8]
00008628:   08 20 1B E5                         ldr     r2, [r11, #-8]
0000862C:   20 30 1B E5                         ldr     r3, [r11, #-32]
00008630:   03 00 52 E1                         cmp     r2, r3
00008634:   C5 FF FF 9A                         bls     #-0xec
00008638:   18 30 4B E2                         sub     r3, r11, #0x18
0000863C:   0C 20 1B E5                         ldr     r2, [r11, #-12]
00008640:   03 00 52 E1                         cmp     r2, r3
00008644:   03 00 00 1A                         bne     #0xc
00008648:   0C 30 1B E5                         ldr     r3, [r11, #-12]
0000864C:   08 30 83 E2                         add     r3, r3, #0x8
00008650:   00 30 93 E5                         ldr     r3, [r3]
00008654:   02 00 00 EA                         b       #0x8
00008658:   0C 30 1B E5                         ldr     r3, [r11, #-12]
0000865C:   04 30 43 E2                         sub     r3, r3, #0x4
00008660:   00 30 93 E5                         ldr     r3, [r3]
00008664:   03 00 A0 E1                         mov     r0, r3
00008668:   00 D0 8B E2                         add     sp, r11, #0x0
0000866C:   00 08 BD E8                         ldm     sp!, {r11}
00008670:   1E FF 2F E1                         bx      lr
fracture> quit



Uninstalling
------------

    The following command will remove any fils that have been
    automatically placed outside the package directory itself during
    installation and building

        make distclean [NOTE: this needs to be added]

Enjoy! :)

- The Draper Embedded Security Team.
