//===-- X86BaseInfo.h - Top level definitions for ARM -------- --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the X86 target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

#ifndef X86BASEINFO_H
#define X86BASEINFO_H

// #include "ARMMCTargetDesc.h"
#include "llvm/Support/ErrorHandling.h"
#define GET_REGINFO_ENUM
#define GET_INSTRINFO_ENUM
#include "X86GenRegisterInfo.inc"
#include "X86GenInstrInfo.inc"

namespace llvm {
// Enums corresponding to ARM condition codes
namespace X86CC {
  // The CondCodes constants map directly to the 4-bit encoding of the
  // condition field for predicated instructions.

} // end namespace X86CC

} // end namespace llvm;
#endif
