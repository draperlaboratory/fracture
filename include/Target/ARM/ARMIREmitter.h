//===- ARMIREmitter.h - Generalize ARMISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for ARMISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef ARMIREMITTER_H
#define ARMIREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "ARMISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class ARMIREmitter : public IREmitter {
public:
  ARMIREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~ARMIREmitter();
private:
  virtual Value* visit(const SDNode *N);
  Value* visitBRCOND(const SDNode *N);
  Value* visitRET(const SDNode *N);
};

} // end fracture namespace

#endif
