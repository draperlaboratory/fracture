//===- X86IREmitter - Generalize X86ISD Instrs  ================-*- C++ -*-=//
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
// Implements visitors for X86ISD SDNodes.
//
//===----------------------------------------------------------------------===//

#include "Target/X86/X86IREmitter.h"
#include "CodeInv/Decompiler.h"
#include "X86BaseInfo.h"

using namespace llvm;

namespace fracture {

X86IREmitter::X86IREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : IREmitter(TheDec, InfoOut, ErrOut) {
  // Nothing to do here
}

X86IREmitter::~X86IREmitter() {
  // Nothing to do here
}

Value* X86IREmitter::visit(const SDNode *N) {
  // return the parent if we are in IR only territory
  if (N->getOpcode() <= ISD::BUILTIN_OP_END){
  	return IREmitter::visit(N);
  }

  // If we already visited the node, return the result.
  if (VisitMap.find(N) != VisitMap.end()) {
    return VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());
  DEBUG(Infos << "Visiting X86 specific Opcode.\n");
  switch (N->getOpcode()) {
    default: return NULL;
    //case X86ISD::BRCOND: return visitBRCOND(N);
    case X86ISD::RET_FLAG: return visitRET(N);
    case X86ISD::CALL: return visitCALL(N);
  }
}

Value* X86IREmitter::visitRET(const SDNode *N) {
  return IRB->CreateRetVoid();
}

//Need to add semantics for visitCall; prob return null in short term...
Value* X86IREmitter::visitCALL(const SDNode *N) {
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(0));
  if (!DestNode) {
    printError("visitCALL: Not a constant integer for call!");
    return NULL;
  }

  int64_t DestInt = DestNode->getSExtValue();
  int64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 4 bytes
  int64_t Tgt = PC + 4 + DestInt;

  // TODO: Look up address in symbol table.
  std::string FName = Dec->getDisassembler()->getFunctionName(Tgt);

  Module *Mod = IRB->GetInsertBlock()->getParent()->getParent();

  FunctionType *FT =
      FunctionType::get(Type::getPrimitiveType(Mod->getContext(),
          Type::VoidTyID), false);

  Twine TgtAddr(Tgt);

  AttributeSet AS;
  AS = AS.addAttribute(Mod->getContext(), AttributeSet::FunctionIndex,
      "Address", TgtAddr.str());
  Value* Proto = Mod->getOrInsertFunction(FName, FT, AS);

  // CallInst* Call =
  IRB->CreateCall(dyn_cast<Value>(Proto));

  // TODO: Technically visitCall sets the LR to IP+8. We should return that.
  VisitMap[N] = NULL;
  return NULL;
}

} // end fracture namespace
