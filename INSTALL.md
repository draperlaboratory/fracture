# INSTALLATION INSTRUCTIONS

This version of the Fracture library works with LLVM r202033. You will have to
compile this version of LLVM before you try to use Fracture. This
guide will walk you through the compilation and installation of both
tools and show usage statements to verify that the fracture library is
compiled correctly.

The library is known to compile on various linux versions (Redhat,
Mageia, Ubuntu, Debian). It should work on Mac OSX and FreeBSD. It
does not compile on Windows.

## Prerequisites

### Linux

#### *libeditline*

Fracture needs an editline or readline library in addition to the
libraries required by LLVM. You will need the "dev" version, in
Mageia, the install line looks like:

```Shell
sudo urpmi lib64edit-devel
```

You can also install from source. Either is known to work.

### Mac OSX

#### *XCode*

You need to have XCode and its command-line-tools installed:

```Shell
xcode-select --install
```

Also accept the License Agreement which is presented when opening XCode.

#### *Homebrew*

Homebrew is a package manager like `apt-get` for OSX and is needed to install additional packages. Instructions on how to install Homebrew see [brew.sh](http://brew.sh).

#### *autoconf* and *automake*

Both are needed to automatically configure the source-code of LLVM, Clang and fracture and can be installed via `Homebrew`. 

```Shell
brew install autoconf automake
```

## Step 1: Installing LLVM

Fracture relies on specific LLVM internals, and so it is best to use
it with a specific revision of the LLVM development tree. Currently,
Fracture works with LLVM 3.6.

You will also need clang to compile the Fracture library. To aid you,
we have forked these repositories at the appropriate revisions:

* https://github.com/llvm-mirror/clang
* https://github.com/llvm-mirror/llvm

As Fracture is a prototype library, we only use it with debugging
enabled. You must compile LLVM and Clang with the same settings. A
sample of commands to do that is as follows:

```Shell
cd $HOME
git clone https://github.com/llvm-mirror/llvm
cd llvm
git checkout release_36
cd tools
git clone https://github.com/llvm-mirror/clang
cd clang
git checkout release_36
cd $HOME/llvm
./configure --enable-debug-symbols --prefix=/usr/local --build=<your info>
make -j16
sudo make install
```

**Note** The `--build` option is important, it should match your `gcc -v`
output:

```Shell
$ gcc -v
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-mageia-linux-gnu/4.8.2/lto-wrapper
Target: x86_64-mageia-linux-gnu
Configured with: ../configure --prefix=/usr --libexecdir=/usr/lib --with-slibdir=/lib
```

In this example, the `--build` variable is:

```Shell
./configure --enable-debug-symbols --prefix=/usr/local --build=x86_64-mageia-linux-gnu
```

**Note** Building LLVM with the `make -j16` command uses ~10GB of memory. On less robust machines it is recommended to just use `make` instead. 

## Step 2: Compiling Fracture

The directory `$HOME/llvm` is the directory you compiled **LLVM** in step 1. We assume you were in your home directory (`/home/yourusername/` in Linux) when you compiled.

Note that the compile generates a large amount of debug output from the
`fracture-tblgen` tool. There may also be warnings in the output.

### Linux

The most common problem is out of date c++ std libraries. **LLVM** needs a
relatively new version of these libs, and the solution is to install a new
g++ (4.7 or newer). After that the compilation of fracture is straight forward:

```Shell
cd $HOME
git clone https://github.com/draperlaboratory/fracture.git fracture
cd fracture
./autoconf/AutoRegen.sh
./configure --enable-debug-symbols --with-llvmsrc=$HOME/llvm --with-llvmobj=$HOME/llvm
make -j16
```

**Note** On Ubuntu, Mageia, and other systems we have had to add the following flag:

    --disable-optimized

### FreeBSD

Tested on RELEASE-9.2 with the latest ports (the first FreeBSD to support the
newer libraries):

```Shell
cd /usr/ports/devel/libc++
sudo make install clean
...
cd $HOME/llvm
./configure --enable-debug-symbols --prefix=/usr/local --build=amd64-undermydesk-freebsd --enable-libcpp
... (gmake, gmake install, etc)
cd $HOME/fracture
export CXXFLAGS=CXXFLAGS='-std=c++11 -stdlib=libc++ -nostdinc++ -I/usr/local/include/c++/v1'
./autoconf/AutoRegen.sh
./configure --enable-debug-symbols --with-llvmsrc=$HOME/llvmdir --with-llvmobj=$HOME/llvm
gmake -j16
```

### Mac OSX

Tested on OSX 10.9.4 with XCode and Homebrew:

```Shell
cd $HOME/fracture
export CXXFLAGS="-std=c++11 -stdlib=libc++ -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1"
./autoconf/AutoRegen.sh
./configure --enable-debug-symbols --with-llvmsrc=$HOME/llvm/ --with-llvmobj=$HOME/llvm/
make -j16
```

### Other Systems

We expect fracture to compile on other systems that compile **LLVM** and **Clang**, but we have not tested it. If you try it and encounter problems, please let us know (fracture@draper.com). If you succeed in compiling it, also let us know so we can post the details in this install document.

## Step 3: Usage Examples

If fracture compiles, you should be able to run it with the following commands.
Note that the only working architecture right now is **ARM**, and you have to
specify machine attribute flags to make the system detect the instructions in
the sample below.

### Call `fracture-cl` with given ARM-binary

```Shell
$ ./Debug+Asserts/bin/fracture-cl -mattr=v6 ./samples/arm/fib_armel
MCDirector: Using Triple: arm-unknown-unknown
MCDirector: Using CPU: generic
MCDirector: Using Features: +v6
Triple: arm-unknown-unknown
CPU: generic
```

### Show Sections

    Debug+Asserts/bin/fracture-cl> sect
```
Sections:
Idx Name               Size      Address          Type
  1                    00000000 0000000000000000 
  2 .interp            00000013 0000000000008134 DATA 
  3 .note.ABI-tag      00000020 0000000000008148 
  4 .note.gnu.build-id 00000024 0000000000008168 
  5 .hash              00000028 000000000000818c 
  6 .gnu.hash          0000002c 00000000000081b4 
  7 .dynsym            00000050 00000000000081e0 
  8 .dynstr            00000043 0000000000008230 
  9 .gnu.version       0000000a 0000000000008274 
 10 .gnu.version_r     00000020 0000000000008280 
 11 .rel.dyn           00000008 00000000000082a0 
 12 .rel.plt           00000020 00000000000082a8 
 13 .init              00000010 00000000000082c8 TEXT DATA 
 14 .plt               00000044 00000000000082d8 TEXT DATA 
 15 .text              00000460 000000000000831c TEXT DATA 
 16 .fini              0000000c 000000000000877c TEXT DATA 
 17 .rodata            00000034 0000000000008788 DATA 
 18 .ARM.exidx         00000008 00000000000087bc 
 19 .eh_frame          00000004 00000000000087c4 DATA 
 20 .init_array        00000004 00000000000107c8 
 21 .fini_array        00000004 00000000000107cc 
 22 .jcr               00000004 00000000000107d0 DATA 
 23 .dynamic           000000f0 00000000000107d4 
 24 .got               00000020 00000000000108c4 DATA 
 25 .data              00000008 00000000000108e4 DATA 
 26 .bss               00000004 00000000000108ec BSS
 27 .ARM.attributes    0000002e 0000000000000000 
 28 .comment           0000001c 0000000000000000 
 29 .shstrtab          0000010a 0000000000000000 
 30 .symtab            00000650 0000000000000000 
 31 .strtab            0000024e 0000000000000000
```


### Show Symbols

    Debug+Asserts/bin/fracture-cl> sym .text
```
SYMBOL TABLE FOR SECTION .text at 0x0000831c
0000831c l     F 	00000000 00000000 _start
00008358 l     F 	00000000 00000000 call_gmon_start
00008384 l     F 	00000000 00000000 __do_global_dtors_aux
000083a0 l     F 	00000000 00000000 frame_dummy
000083d4 l     F 	00000000 00000000 main
000084cc l     F 	00000000 00000000 fib
0000852c l     F 	00000000 00000000 fastfib
00008674 l     F 	00000000 00000000 fastfib_v2
00008708 l     F 	00000000 00000000 __libc_csu_fini
0000870c l     F 	00000000 00000000 __libc_csu_init
```

### Show Disassembly

    Debug+Asserts/bin/fracture-cl> dis 0x83d4
```Assembly
Address: 33748
NumInstrs: 0
<main>:
000083D4:   10 48 2D E9                 	push	{r4, r11, lr}
000083D8:   08 B0 8D E2                 	add	r11, sp, #8
000083DC:   0C D0 4D E2                 	sub	sp, sp, #12
000083E0:   00 30 A0 E3                 	mov	r3, #0
000083E4:   10 30 0B E5                 	str	r3, [r11, #-0x10]
000083E8:   0A 00 00 EA                 	b	#0x28
000083EC:   CC 40 9F E5                 	ldr	r4, [pc, #0xcc]
000083F0:   10 00 1B E5                 	ldr	r0, [r11, #-0x10]
000083F4:   34 00 00 EB                 	bl	#0xd0 @ fib
000083F8:   00 30 A0 E1                 	mov	r3, r0
000083FC:   04 00 A0 E1                 	mov	r0, r4
00008400:   10 10 1B E5                 	ldr	r1, [r11, #-0x10]
00008404:   03 20 A0 E1                 	mov	r2, r3
00008408:   C0 FF FF EB                 	bl	#-0x100 @ printf
0000840C:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
00008410:   01 30 83 E2                 	add	r3, r3, #1
00008414:   10 30 0B E5                 	str	r3, [r11, #-0x10]
00008418:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
0000841C:   22 00 53 E3                 	cmp	r3, #34
00008420:   F1 FF FF 9A                 	bls	#-0x3c
00008424:   00 30 A0 E3                 	mov	r3, #0
00008428:   10 30 0B E5                 	str	r3, [r11, #-0x10]
0000842C:   0A 00 00 EA                 	b	#0x28
00008430:   8C 40 9F E5                 	ldr	r4, [pc, #0x8c]
00008434:   10 00 1B E5                 	ldr	r0, [r11, #-0x10]
00008438:   3B 00 00 EB                 	bl	#0xec @ fastfib
0000843C:   00 30 A0 E1                 	mov	r3, r0
00008440:   04 00 A0 E1                 	mov	r0, r4
00008444:   10 10 1B E5                 	ldr	r1, [r11, #-0x10]
00008448:   03 20 A0 E1                 	mov	r2, r3
0000844C:   AF FF FF EB                 	bl	#-0x144 @ printf
00008450:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
00008454:   01 30 83 E2                 	add	r3, r3, #1
00008458:   10 30 0B E5                 	str	r3, [r11, #-0x10]
0000845C:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
00008460:   22 00 53 E3                 	cmp	r3, #34
00008464:   F1 FF FF 9A                 	bls	#-0x3c
00008468:   00 30 A0 E3                 	mov	r3, #0
0000846C:   10 30 0B E5                 	str	r3, [r11, #-0x10]
00008470:   0A 00 00 EA                 	b	#0x28
00008474:   4C 40 9F E5                 	ldr	r4, [pc, #0x4c]
00008478:   10 00 1B E5                 	ldr	r0, [r11, #-0x10]
0000847C:   2A 00 00 EB                 	bl	#0xa8 @ fastfib
00008480:   00 30 A0 E1                 	mov	r3, r0
00008484:   04 00 A0 E1                 	mov	r0, r4
00008488:   10 10 1B E5                 	ldr	r1, [r11, #-0x10]
0000848C:   03 20 A0 E1                 	mov	r2, r3
00008490:   9E FF FF EB                 	bl	#-0x188 @ printf
00008494:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
00008498:   01 30 83 E2                 	add	r3, r3, #1
0000849C:   10 30 0B E5                 	str	r3, [r11, #-0x10]
000084A0:   10 30 1B E5                 	ldr	r3, [r11, #-0x10]
000084A4:   22 00 53 E3                 	cmp	r3, #34
000084A8:   F1 FF FF 9A                 	bls	#-0x3c
000084AC:   00 30 A0 E3                 	mov	r3, #0
000084B0:   03 00 A0 E1                 	mov	r0, r3
000084B4:   08 D0 4B E2                 	sub	sp, r11, #8
000084B8:   10 48 BD E8                 	pop	{r4, r11, lr}
000084BC:   1E FF 2F E1                 	bx	lr
```

### Decompile to LLVM-IR

    Debug+Asserts/bin/fracture-cl> dec 0x83d4
```LLVM
define void @main() {
entry:
  br label %"main+0"

"main+0":                                         ; preds = %entry
  %SP1 = load i32* @SP, !dbg !0
  %R4_1 = load i32* @R4, !dbg !0
  %R11_1 = load i32* @R11, !dbg !0
  %LR1 = load i32* @LR, !dbg !0
  %SP2 = inttoptr i32 %SP1 to i32*, !dbg !0
  store i32 %R4_1, i32* %SP2, !dbg !0
  %SP3 = sub i32 %SP1, 4, !dbg !0
  %SP4 = inttoptr i32 %SP3 to i32*, !dbg !0
  store i32 %R11_1, i32* %SP4, !dbg !0
  %SP5 = sub i32 %SP3, 4, !dbg !0
  %SP6 = inttoptr i32 %SP5 to i32*, !dbg !0
  store i32 %LR1, i32* %SP6, !dbg !0
  %SP7 = sub i32 %SP5, 4, !dbg !0
  store i32 %SP7, i32* @SP, !dbg !0
  %SP8 = load i32* @SP, !dbg !2
  %R11_2 = add i32 %SP8, 8, !dbg !2
  store i32 %R11_2, i32* @R11, !dbg !2
  %SP9 = load i32* @SP, !dbg !4
  %SP10 = sub i32 %SP9, 12, !dbg !4
  store i32 %SP10, i32* @SP, !dbg !4
  store i32 0, i32* @R3, !dbg !6
  %R3_1 = load i32* @R3, !dbg !8
  %R11_3 = load i32* @R11, !dbg !8
  %R11_4 = add i32 %R11_3, -16, !dbg !8
  %R11_5 = inttoptr i32 %R11_4 to i32*, !dbg !8
  store i32 %R3_1, i32* %R11_5, !dbg !8
  br label %"main+68", !dbg !10

"main+24":                                        ; preds = %"main+68"
  %PC1 = load i32* @PC, !dbg !12
  %PC2 = add i32 %PC1, 204, !dbg !12
  %PC3 = inttoptr i32 %PC2 to i32*, !dbg !12
  %PC4 = load i32* %PC3, !dbg !12
  store i32 %PC4, i32* @R4, !dbg !12
  %R11_6 = load i32* @R11, !dbg !14
  %R11_7 = add i32 %R11_6, -16, !dbg !14
  %R11_8 = inttoptr i32 %R11_7 to i32*, !dbg !14
  %R11_9 = load i32* %R11_8, !dbg !14
  store i32 %R11_9, i32* @R0, !dbg !14
  call void @fib(), !dbg !16
  %R0_1 = load i32* @R0, !dbg !18
  store i32 %R0_1, i32* @R3, !dbg !18
  %R4_2 = load i32* @R4, !dbg !20
  store i32 %R4_2, i32* @R0, !dbg !20
  %R11_10 = load i32* @R11, !dbg !22
  %R11_11 = add i32 %R11_10, -16, !dbg !22
  %R11_12 = inttoptr i32 %R11_11 to i32*, !dbg !22
  %R11_13 = load i32* %R11_12, !dbg !22
  store i32 %R11_13, i32* @R1, !dbg !22
  %R3_2 = load i32* @R3, !dbg !24
  store i32 %R3_2, i32* @R2, !dbg !24
  call void @printf(), !dbg !26
  %R11_14 = load i32* @R11, !dbg !28
  %R11_15 = add i32 %R11_14, -16, !dbg !28
  %R11_16 = inttoptr i32 %R11_15 to i32*, !dbg !28
  %R11_17 = load i32* %R11_16, !dbg !28
  store i32 %R11_17, i32* @R3, !dbg !28
  %R3_3 = load i32* @R3, !dbg !30
  %R3_4 = add i32 %R3_3, 1, !dbg !30
  store i32 %R3_4, i32* @R3, !dbg !30
  %R3_5 = load i32* @R3, !dbg !32
  %R11_18 = load i32* @R11, !dbg !32
  %R11_19 = add i32 %R11_18, -16, !dbg !32
  %R11_20 = inttoptr i32 %R11_19 to i32*, !dbg !32
  store i32 %R3_5, i32* %R11_20, !dbg !32
  br label %"main+68", !dbg !32

"main+68":                                        ; preds = %"main+24", %"main+0"
  %R11_21 = load i32* @R11, !dbg !34
  %R11_22 = add i32 %R11_21, -16, !dbg !34
  %R11_23 = inttoptr i32 %R11_22 to i32*, !dbg !34
  %R11_24 = load i32* %R11_23, !dbg !34
  store i32 %R11_24, i32* @R3, !dbg !34
  %R3_6 = load i32* @R3, !dbg !36
  %CPSR1 = sub i32 %R3_6, 34, !dbg !36
  store i32 %CPSR1, i32* @CPSR, !dbg !36
  %CPSR2 = load i32* @CPSR, !dbg !38
  %0 = icmp ule i32 %R3_6, 34, !dbg !38
  br i1 %0, label %"main+24", label %"main+80", !dbg !38

"main+80":                                        ; preds = %"main+68"
  store i32 0, i32* @R3, !dbg !40
  %R3_7 = load i32* @R3, !dbg !42
  %R11_25 = load i32* @R11, !dbg !42
  %R11_26 = add i32 %R11_25, -16, !dbg !42
  %R11_27 = inttoptr i32 %R11_26 to i32*, !dbg !42
  store i32 %R3_7, i32* %R11_27, !dbg !42
  br label %"main+136", !dbg !44

"main+92":                                        ; preds = %"main+136"
  %PC5 = load i32* @PC, !dbg !46
  %PC6 = add i32 %PC5, 140, !dbg !46
  %PC7 = inttoptr i32 %PC6 to i32*, !dbg !46
  %PC8 = load i32* %PC7, !dbg !46
  store i32 %PC8, i32* @R4, !dbg !46
  %R11_28 = load i32* @R11, !dbg !48
  %R11_29 = add i32 %R11_28, -16, !dbg !48
  %R11_30 = inttoptr i32 %R11_29 to i32*, !dbg !48
  %R11_31 = load i32* %R11_30, !dbg !48
  store i32 %R11_31, i32* @R0, !dbg !48
  call void @fastfib(), !dbg !50
  %R0_2 = load i32* @R0, !dbg !52
  store i32 %R0_2, i32* @R3, !dbg !52
  %R4_3 = load i32* @R4, !dbg !54
  store i32 %R4_3, i32* @R0, !dbg !54
  %R11_32 = load i32* @R11, !dbg !56
  %R11_33 = add i32 %R11_32, -16, !dbg !56
  %R11_34 = inttoptr i32 %R11_33 to i32*, !dbg !56
  %R11_35 = load i32* %R11_34, !dbg !56
  store i32 %R11_35, i32* @R1, !dbg !56
  %R3_8 = load i32* @R3, !dbg !58
  store i32 %R3_8, i32* @R2, !dbg !58
  call void @printf(), !dbg !60
  %R11_36 = load i32* @R11, !dbg !62
  %R11_37 = add i32 %R11_36, -16, !dbg !62
  %R11_38 = inttoptr i32 %R11_37 to i32*, !dbg !62
  %R11_39 = load i32* %R11_38, !dbg !62
  store i32 %R11_39, i32* @R3, !dbg !62
  %R3_9 = load i32* @R3, !dbg !64
  %R3_10 = add i32 %R3_9, 1, !dbg !64
  store i32 %R3_10, i32* @R3, !dbg !64
  %R3_11 = load i32* @R3, !dbg !66
  %R11_40 = load i32* @R11, !dbg !66
  %R11_41 = add i32 %R11_40, -16, !dbg !66
  %R11_42 = inttoptr i32 %R11_41 to i32*, !dbg !66
  store i32 %R3_11, i32* %R11_42, !dbg !66
  br label %"main+136", !dbg !66

"main+136":                                       ; preds = %"main+92", %"main+80"
  %R11_43 = load i32* @R11, !dbg !68
  %R11_44 = add i32 %R11_43, -16, !dbg !68
  %R11_45 = inttoptr i32 %R11_44 to i32*, !dbg !68
  %R11_46 = load i32* %R11_45, !dbg !68
  store i32 %R11_46, i32* @R3, !dbg !68
  %R3_12 = load i32* @R3, !dbg !70
  %CPSR3 = sub i32 %R3_12, 34, !dbg !70
  store i32 %CPSR3, i32* @CPSR, !dbg !70
  %CPSR4 = load i32* @CPSR, !dbg !72
  %1 = icmp ule i32 %R3_12, 34, !dbg !72
  br i1 %1, label %"main+92", label %"main+148", !dbg !72

"main+148":                                       ; preds = %"main+136"
  store i32 0, i32* @R3, !dbg !74
  %R3_13 = load i32* @R3, !dbg !76
  %R11_47 = load i32* @R11, !dbg !76
  %R11_48 = add i32 %R11_47, -16, !dbg !76
  %R11_49 = inttoptr i32 %R11_48 to i32*, !dbg !76
  store i32 %R3_13, i32* %R11_49, !dbg !76
  br label %"main+204", !dbg !78

"main+160":                                       ; preds = %"main+204"
  %PC9 = load i32* @PC, !dbg !80
  %PC10 = add i32 %PC9, 76, !dbg !80
  %PC11 = inttoptr i32 %PC10 to i32*, !dbg !80
  %PC12 = load i32* %PC11, !dbg !80
  store i32 %PC12, i32* @R4, !dbg !80
  %R11_50 = load i32* @R11, !dbg !82
  %R11_51 = add i32 %R11_50, -16, !dbg !82
  %R11_52 = inttoptr i32 %R11_51 to i32*, !dbg !82
  %R11_53 = load i32* %R11_52, !dbg !82
  store i32 %R11_53, i32* @R0, !dbg !82
  call void @fastfib(), !dbg !84
  %R0_3 = load i32* @R0, !dbg !86
  store i32 %R0_3, i32* @R3, !dbg !86
  %R4_4 = load i32* @R4, !dbg !88
  store i32 %R4_4, i32* @R0, !dbg !88
  %R11_54 = load i32* @R11, !dbg !90
  %R11_55 = add i32 %R11_54, -16, !dbg !90
  %R11_56 = inttoptr i32 %R11_55 to i32*, !dbg !90
  %R11_57 = load i32* %R11_56, !dbg !90
  store i32 %R11_57, i32* @R1, !dbg !90
  %R3_14 = load i32* @R3, !dbg !92
  store i32 %R3_14, i32* @R2, !dbg !92
  call void @printf(), !dbg !94
  %R11_58 = load i32* @R11, !dbg !96
  %R11_59 = add i32 %R11_58, -16, !dbg !96
  %R11_60 = inttoptr i32 %R11_59 to i32*, !dbg !96
  %R11_61 = load i32* %R11_60, !dbg !96
  store i32 %R11_61, i32* @R3, !dbg !96
  %R3_15 = load i32* @R3, !dbg !98
  %R3_16 = add i32 %R3_15, 1, !dbg !98
  store i32 %R3_16, i32* @R3, !dbg !98
  %R3_17 = load i32* @R3, !dbg !100
  %R11_62 = load i32* @R11, !dbg !100
  %R11_63 = add i32 %R11_62, -16, !dbg !100
  %R11_64 = inttoptr i32 %R11_63 to i32*, !dbg !100
  store i32 %R3_17, i32* %R11_64, !dbg !100
  br label %"main+204", !dbg !100

"main+204":                                       ; preds = %"main+160", %"main+148"
  %R11_65 = load i32* @R11, !dbg !102
  %R11_66 = add i32 %R11_65, -16, !dbg !102
  %R11_67 = inttoptr i32 %R11_66 to i32*, !dbg !102
  %R11_68 = load i32* %R11_67, !dbg !102
  store i32 %R11_68, i32* @R3, !dbg !102
  %R3_18 = load i32* @R3, !dbg !104
  %CPSR5 = sub i32 %R3_18, 34, !dbg !104
  store i32 %CPSR5, i32* @CPSR, !dbg !104
  %CPSR6 = load i32* @CPSR, !dbg !106
  %2 = icmp ule i32 %R3_18, 34, !dbg !106
  br i1 %2, label %"main+160", label %"main+216", !dbg !106

"main+216":                                       ; preds = %"main+204"
  store i32 0, i32* @R3, !dbg !108
  %R3_19 = load i32* @R3, !dbg !110
  store i32 %R3_19, i32* @R0, !dbg !110
  %R11_69 = load i32* @R11, !dbg !112
  %SP11 = sub i32 %R11_69, 8, !dbg !112
  store i32 %SP11, i32* @SP, !dbg !112
  %SP12 = load i32* @SP, !dbg !114
  %SP13 = add i32 %SP12, 4, !dbg !114
  %SP14 = inttoptr i32 %SP13 to i32*, !dbg !114
  %SP15 = load i32* %SP14, !dbg !114
  store i32 %SP15, i32* @R4, !dbg !114
  %SP16 = add i32 %SP13, 4, !dbg !114
  %SP17 = inttoptr i32 %SP16 to i32*, !dbg !114
  %SP18 = load i32* %SP17, !dbg !114
  store i32 %SP18, i32* @R11, !dbg !114
  %SP19 = add i32 %SP16, 4, !dbg !114
  %SP20 = inttoptr i32 %SP19 to i32*, !dbg !114
  %SP21 = load i32* %SP20, !dbg !114
  store i32 %SP21, i32* @LR, !dbg !114
  store i32 %SP19, i32* @SP, !dbg !114
  ret void, !dbg !116
}

```

### Quit `fracture-cl`

    Debug+Asserts/bin/fracture-cl> quit
