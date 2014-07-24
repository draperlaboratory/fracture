##===- fracture-cl/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=mkAllInsts

#
# List libraries that we'll need
#
#USEDLIBS = Commands.a FractureCodeInv.a FractureARMCodeInv.a \
#           FractureX86CodeInv.a TypeRecovery.a

#
# LLVM Components we wish to link with.
#
LINK_COMPONENTS = all-targets DebugInfo MC MCParser MCDisassembler Object \
                  IRReader

#LLVMLIBS = LLVMTarget.a LLVMDebugInfo.a LLVMMC.a LLVMMCParser.a \
#           LLVMMCDisassembler.a LLVMObject.a LLVMIRReader.a


#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common
