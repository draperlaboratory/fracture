##===- lib/CodeInv/Makefile --------------------------------*- Makefile -*-===##
#
#              Fracture: The Draper Decompiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..
LIBRARYNAME = FractureCodeInv
# PARALLEL_DIRS = SelectionDAG AsmPrinter
# BUILD_ARCHIVE = 1

include $(LEVEL)/Makefile.common

# Xcode prior to 2.4 generates an error in -pedantic mode with use of HUGE_VAL
# in this directory.  Disable -pedantic for this broken compiler.
# ifneq ($(HUGE_VAL_SANITY),yes)
# CompileCommonOpts := $(filter-out -pedantic, $(CompileCommonOpts))
# endif

