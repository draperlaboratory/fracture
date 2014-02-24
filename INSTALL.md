INSTALLATION INSTRUCTIONS
=========================

This version of the Fracture library works with LLVM r202033. You will have to
compile this version of LLVM before you try to use Fracture. This
guide will walk you through the compilation and installation of both
tools and show usage statements to verify that the fracture library is
compiled correctly.

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
    ./configure --enable-debug-symbols --prefix=/usr/local --build=
    make -j16
    sudo make install
    



Install libeditline

    sudo urpmi lib64edit-devel
