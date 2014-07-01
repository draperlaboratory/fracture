//===- X86IREmitter.h - Generalize X86ISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for X86ISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef X86IREMITTER_H
#define X86IREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "X86ISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class X86IREmitter : public IREmitter {
public:
  X86IREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~X86IREmitter();
private:
  virtual Value* visit(const SDNode *N);
  Value* visitRET(const SDNode *N);
  Value* visitBRCOND(const SDNode *N);
};

} // end fracture namespace

#endif
