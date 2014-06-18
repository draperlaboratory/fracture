//===- PPCIREmitter.h - Generalize PPCISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for PPCISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef PPCIREMITTER_H
#define PPCIREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "PPCISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class PPCIREmitter : public IREmitter {
public:
  PPCIREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~PPCIREmitter();
private:
  virtual Value* visit(const SDNode *N);
  Value* visitRET(const SDNode *N);
};

} // end fracture namespace

#endif
