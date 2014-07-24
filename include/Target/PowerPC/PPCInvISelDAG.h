//===- PowerPCInvISelDAG.h - Interface for PowerPC Inv ISel ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Provides inverse DAG selector functionality for PowerPC targets.
//
//===----------------------------------------------------------------------===//

#ifndef POWERPCINVISELDAG_H
#define POWERPCINVISELDAG_H

#include "CodeInv/InvISelDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/CodeGen.h"
#include "PPCISD.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "Target/PowerPC/PowerPCIREmitter.h"
// #include "ARMRegs.h"
#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"

namespace fracture {

class PPCInvISelDAG : public InvISelDAG {
public:
  PPCInvISelDAG(const TargetMachine &TMC,
      CodeGenOpt::Level OL = CodeGenOpt::Default,
      const Decompiler *TheDec = NULL) : InvISelDAG(TMC, OL, TheDec), Dec(TheDec) {};

  ~PPCInvISelDAG() {};

  //prob not going to work since I don't have this object...
  virtual IREmitter* getEmitter(Decompiler *Dec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls())
  { return new PowerPCIREmitter(Dec, InfoOut, ErrOut); }

  SDNode* InvertCode(SDNode *N);
  SDNode* Transmogrify(SDNode *N);
  SDValue ConvertNoRegToZero(const SDValue N);
private:
  const Decompiler *Dec;
};

} // end fracture namespace

#endif
