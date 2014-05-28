//===- X86InvISelDAG.h - Interface for X86 Inv ISel ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Provides inverse DAG selector functionality for X86 targets.
//
//===----------------------------------------------------------------------===//

#ifndef X86INVISELDAG_H
#define X86INVISELDAG_H

#include "CodeInv/InvISelDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/CodeGen.h"
#include "X86ISD.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "Target/X86/X86IREmitter.h"
// #include "ARMRegs.h"

namespace fracture {

class X86InvISelDAG : public InvISelDAG {
public:
	X86InvISelDAG(const TargetMachine &TMC,
    CodeGenOpt::Level OL = CodeGenOpt::Default) : InvISelDAG(TMC, OL) {};

  ~X86InvISelDAG() {};

  //prob not going to work since I don't have this object...
  virtual IREmitter* getEmitter(Decompiler *Dec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls())
  { return new X86IREmitter(Dec, InfoOut, ErrOut); }

  SDNode* InvertCode(SDNode *N);
  SDNode* Transmogrify(SDNode *N);
  SDValue ConvertNoRegToZero(const SDValue N);
};

} // end fracture namespace

#endif
