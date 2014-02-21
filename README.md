Fracture
========

Fracture is an architecture-independent decompiler to LLVM IR. It is open source
software. You may freely distributed it under the terms of the 3-clause BSD
license agreement found in the LICENSE file. See the INSTALL.md file for
instructions on how to compile and install this software.

Fracture can speed up a variety of applications and also enable generic
implementations of a number of static and dynamic analysis tools. Examples
include interactive debuggers or disassemblers that provide LLVM IR
representations to users unfamiliar with the instruction set, static analysis
algorithms that solve indirect control transfer (ICT) problems modified for IR
to use KLEE or other LLVM technologies, and IR-based decompilers or emulators
extended to work on machine binaries.

Fracture consists of an LLVM TableGen backend and associated library that
ingests a basic block of target instructions and emits a directed acyclic graph
(DAG) which resembles the post-legalization phase of LLVM’s SelectionDAG
instruction selection process. It leverages the pre-existing target LLVM
TableGen definitions, without modification, to provide a generic way to
abstract LLVM IR efficiently from different target instruction sets.

Fracture currently works with the ARM backend, and is still in the proof of
concept stage, with several missing pieces before it can be utilitized as we
intend. See the ROADMAP.md file for more information.
