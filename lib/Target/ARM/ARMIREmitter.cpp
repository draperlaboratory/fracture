//===- ARMIREmitter - Generalize ARMISD Instrs  ================-*- C++ -*-=//
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
// Implements visitors for ARMISD SDNodes.
//
//===----------------------------------------------------------------------===//

#include "Target/ARM/ARMIREmitter.h"
#include "CodeInv/Decompiler.h"
#include "ARMBaseInfo.h"

using namespace llvm;

namespace fracture {

ARMIREmitter::ARMIREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : IREmitter(TheDec, InfoOut, ErrOut) {
  // Nothing to do here
}

ARMIREmitter::~ARMIREmitter() {
  // Nothing to do here
}

Value* ARMIREmitter::visit(const SDNode *N) {
  // return the parent if we are in IR only territory
  if (N->getOpcode() <= ISD::BUILTIN_OP_END) return IREmitter::visit(N);

  // If we already visited the node, return the result.
  if (VisitMap.find(N) != VisitMap.end()) {
    return VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());

  DEBUG(Infos << "Visiting ARM specific Opcode.\n");
  switch (N->getOpcode()) {
    default: return NULL;
    case ARMISD::BRCOND: return visitBRCOND(N);
    case ARMISD::RET_FLAG: return visitRET(N);
    case ARMISD::CALL: return visitCALL(N);
  }
}

// Note: branch conditions, by definition, only have a chain user.
// This is why it should not be saved in a map for recall.
Value* ARMIREmitter::visitBRCOND(const SDNode *N) {
  // Get the address
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(0));
  if (!DestNode) {
    printError("visitBRCOND: Not a constant integer for branch!");
    return NULL;
  }

  uint64_t DestInt = DestNode->getSExtValue();
  uint64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  uint64_t Tgt = PC + 8 + DestInt;

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();

  BasicBlock *BBTgt = Dec->getOrCreateBasicBlock(Tgt, F);

  // Parse the branch condition code
  const ConstantSDNode *CCNode = dyn_cast<ConstantSDNode>(N->getOperand(1));
  if (!CCNode) {
    printError("visitBRCOND: Condition code is not a constant integer!");
    return NULL;
  }
  ARMCC::CondCodes ARMcc = ARMCC::CondCodes(CCNode->getZExtValue());

  // Unconditional branch
  if (ARMcc == ARMCC::AL) {
    Instruction *Br = IRB->CreateBr(BBTgt);
    Br->setDebugLoc(N->getDebugLoc());
    return Br;
  }

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


  SDNode *CPSR = N->getOperand(2)->getOperand(1).getNode();
  SDNode *CMPNode = NULL;
  for (SDNode::use_iterator I = CPSR->use_begin(), E = CPSR->use_end(); I != E;
       ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      CMPNode = I->getOperand(2).getNode();
    }
  }

  if (CMPNode == NULL) {
    errs() << "ARMIREmitter ERROR: Could not find CMP SDNode for ARMBRCond!\n";
    return NULL;
  }

  Value *Cmp = NULL;
  Value *LHS = visit(CMPNode->getOperand(0).getNode());
  Value *RHS = visit(CMPNode->getOperand(1).getNode());
  // See ARMCC::CondCodes IntCCToARMCC(ISD::CondCode CC); in ARMISelLowering.cpp
  // TODO: Add support for conditions that handle floating point
  switch(ARMcc) {
    default:
      printError("Unknown condition code");
      return NULL;
    case ARMCC::EQ:
      Cmp = IRB->CreateICmpEQ(LHS, RHS);
      break;
    case ARMCC::NE:
      Cmp = IRB->CreateICmpNE(LHS, RHS);
      break;
    case ARMCC::HS:
      // HS - unsigned higher or same (or carry set)
      Cmp = IRB->CreateICmpUGE(LHS, RHS);
      break;
    case ARMCC::LO:
      // LO - unsigned lower (or carry clear)
      Cmp = IRB->CreateICmpULT(LHS, RHS);
      break;
    case ARMCC::MI:
      // MI - minus (negative)
      printError("Condition code MI is not handled at this time!");
      return NULL;
      // break;
    case ARMCC::PL:
      // PL - plus (positive or zero)
      printError("Condition code PL is not handled at this time!");
      return NULL;
      // break;
    case ARMCC::VS:
      // VS - V Set (signed overflow)
      printError("Condition code VS is not handled at this time!");
      return NULL;
      // break;
    case ARMCC::VC:
      // VC - V clear (no signed overflow)
      printError("Condition code VC is not handled at this time!");
      return NULL;
      // break;
    case ARMCC::HI:
      // HI - unsigned higher
      Cmp = IRB->CreateICmpUGT(LHS, RHS);
      break;
    case ARMCC::LS:
      // LS - unsigned lower or same
      Cmp = IRB->CreateICmpULE(LHS, RHS);
      break;
    case ARMCC::GE:
      // GE - signed greater or equal
      Cmp = IRB->CreateICmpSGE(LHS, RHS);
      break;
    case ARMCC::LT:
      // LT - signed less than
      Cmp = IRB->CreateICmpSLT(LHS, RHS);
      break;
    case ARMCC::GT:
      // GT - signed greater than
      Cmp = IRB->CreateICmpSGT(LHS, RHS);
      break;
    case ARMCC::LE:
      // LE - signed less than or equal
      Cmp = IRB->CreateICmpSLE(LHS, RHS);
      break;
  }
  (dyn_cast<Instruction>(Cmp))->setDebugLoc(N->getOperand(2)->getDebugLoc());

  // Conditional branch
  Instruction *Br = IRB->CreateCondBr(Cmp, BBTgt, NextBB);
  Br->setDebugLoc(N->getDebugLoc());
  return Br;
}

Value* ARMIREmitter::visitRET(const SDNode *N) {
  return IRB->CreateRetVoid();
}

Value* ARMIREmitter::visitCALL(const SDNode *N) {
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(0));
  if (!DestNode) {
    printError("visitCALL: Not a constant integer for call!");
    return NULL;
  }

  int64_t DestInt = DestNode->getSExtValue();
  int64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  unsigned InstrSize = 8;   // Note: ARM defaults to 4; should be 8.
  int64_t Tgt = PC + InstrSize + DestInt;

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
