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
    case X86ISD::BRCOND:    return visitBRCOND(N);
    case X86ISD::RET_FLAG:  return visitRET(N);
    case X86ISD::CALL:      return visitCALL(N);
  }
}

// Note: branch conditions, by definition, only have a chain user.
// This is why it should not be saved in a map for recall.
Value* X86IREmitter::visitBRCOND(const SDNode *N) {
  //llvm_unreachable("visitBRCOND Unimplemented visit..."); return NULL;

  // Get the address
  const CondCodeSDNode *Cond = dyn_cast<CondCodeSDNode>(N->getOperand(0));
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(1));

  if (!DestNode) {
    printError("X86IREmitter::visitBRCOND: Not a constant integer for branch!");
    return NULL;
  }

  uint64_t DestInt = DestNode->getSExtValue();
  uint64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  uint64_t Tgt = PC + DestInt;  //Address is added in X86 (potentially need to adjust ARM)

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = Dec->getOrCreateBasicBlock(Tgt, F);

  //Instruction *Br = IRB->CreateBr(BBTgt);

  SDNode *CPSR = N->getOperand(2)->getOperand(1).getNode();
  SDNode *CMPNode = NULL;

  //outs() << "X86ISD::BRCOND - " << X86ISD::BRCOND << "\n";
  //outs() << "X86ISD::CMP - " << X86ISD::CMP << "\n";

  for (SDNode::use_iterator I = CPSR->use_begin(), E = CPSR->use_end(); I != E;
      ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      if(I->getOperand(2)->getOpcode() == X86ISD::CMP){ //This could should go away...
        CMPNode = I->getOperand(2).getNode();
        //outs() << "Found ISD::CopyToReg && X86ISD::CMP\n";
        //CMPNode->dump();
      }
    }
  }

  if (CMPNode == NULL) {
    //Find a math op in the IR.
    //RevIterator to go through and find BB with Math op (tough) || walk the DAG look for copy to reg and see if it is setting eflags; if it is get op 2
    //Walk up the chain example?
    errs() << "X86IREmitter ERROR: Could not find CMP SDNode for BRCond!\n";
    return NULL;
  }

  //This code should currently be platform agnostic since CMPNode
  //    is an iterator that runs over ISD::CopyToReg, which is
  //    currently platform agnostic...
  Value *Cmp = NULL;
  //outs() << "----Name " << CurBB->getName() << "\n";
  Value *LHS = visit(CMPNode->getOperand(0).getNode());
  Value *RHS = visit(CMPNode->getOperand(1).getNode());

  // TODO: Add support for conditions that handle floating point
  switch(Cond->get()) {
  default:
    printError("Unknown condition code");
    return NULL;
  case ISD::SETEQ:
    Cmp = IRB->CreateICmpEQ(LHS, RHS);
    break;
  case ISD::SETNE:
    Cmp = IRB->CreateICmpNE(LHS, RHS);
    break;
  case ISD::SETGE:
    // GE - signed greater or equal
    Cmp = IRB->CreateICmpSGE(LHS, RHS);
    break;
  case ISD::SETLT:
    // LT - signed less than
    Cmp = IRB->CreateICmpSLT(LHS, RHS);
    break;
  case ISD::SETGT:
    // GT - signed greater than
    Cmp = IRB->CreateICmpSGT(LHS, RHS);
    break;
  case ISD::SETLE:
    // LE - signed less than or equal
    Cmp = IRB->CreateICmpSLE(LHS, RHS);
    break;
  }
  (dyn_cast<Instruction>(Cmp))->setDebugLoc(N->getOperand(2)->getDebugLoc());

  // If not a conditional branch, find the successor block and look at CC
  BasicBlock *NextBB = NULL;
  Function::iterator BI = F->begin(), BE= F->end();
  while (BI != BE && BI->getName() != CurBB->getName()) ++BI;
  ++BI;
  if (BI == BE) {               // NOTE: This should never happen...
    NextBB = Dec->getOrCreateBasicBlock("end", F);
  } else {
    NextBB = &(*BI);
  }

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  return Br;
}

Value* X86IREmitter::visitRET(const SDNode *N) {
  return IRB->CreateRetVoid();
}

} // end fracture namespace
