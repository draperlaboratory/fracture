Project Roadmap
===============

These are in no particular order:

1. Finish basic type recovery to distinguish between structs/stack/class
   variables and pointer types
2. Add support for detecting and defining how variables are passed through
   functions.
3. Copy data elements from executable into IR output to make it run in lli.
4. Target support - x86, mips, and PPC
5. Add recursive decent parser and capture all control flow (including
   annotations of invariants for loops)
6. More advanced type recovery using library and system calls
7. Indirect control flow smt/sat solver
8. Support for conditional instructions
9. Support for multi-def instructions
10. Support for conditionals
11. High level type recovery
12. Optimizing codeinvisel tables
13. Exception handlers, interrupts, signals and other things that can affect
    control flow
14. Recovery of complex language structs (slices in python or c++ classes)
