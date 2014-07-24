//===- PowerPCIREmitter - Generalize PowerPCISD Instrs  ================-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// This file modifies code originally found in LLVM.
//
//===----------------------------------------------------------------------===//
//
// Implements visitors for PowerPCISD SDNodes.
//
//===----------------------------------------------------------------------===//

#include "Target/PowerPC/PowerPCIREmitter.h"
#include "CodeInv/Decompiler.h"
#include "PowerPCBaseInfo.h"

using namespace llvm;

namespace fracture {

PowerPCIREmitter::PowerPCIREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : IREmitter(TheDec, InfoOut, ErrOut) {
  // Nothing to do here
}

PowerPCIREmitter::~PowerPCIREmitter() {
  // Nothing to do here
}

Value* PowerPCIREmitter::visit(const SDNode *N) {
  // return the parent if we are in IR only territory
  if (N->getOpcode() <= ISD::BUILTIN_OP_END){
    return IREmitter::visit(N);
  }

  // If we already visited the node, return the result.
  if (VisitMap.find(N) != VisitMap.end()) {
    return VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());
  DEBUG(Infos << "Visiting PowerPC specific Opcode.\n");
  switch (N->getOpcode()) {
    default: return NULL;
    //case PPCISD::BRCOND: return visitBRCOND(N);
    //case PPCISD::RET_FLAG: return visitRET(N);
    //case PPCISD::CALL: return visitCALL(N);
  }
}

//Value* PowerPCIREmitter::visitRET(const SDNode *N) {
//  return IRB->CreateRetVoid();
//}

} // end fracture namespace
