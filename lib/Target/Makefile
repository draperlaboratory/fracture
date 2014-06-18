#===- lib/Target/Makefile ----------------------------------*- Makefile -*-===##
#
#              Fracture: The Draper Decompiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..
#LIBRARYNAME = FractureTarget
PARALLEL_DIRS = ARM X86 PowerPC
#BUILD_ARCHIVE = 1

# We include this early so we can access the value of TARGETS_TO_BUILD as the
# value for PARALLEL_DIRS which must be set before Makefile.rules is included
include $(LEVEL)/Makefile.common
include $(LEVEL)/Makefile.llvm.config

# PARALLEL_DIRS := $(TARGETS_TO_BUILD)
