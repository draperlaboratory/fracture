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
    case X86ISD::BRCOND:    return visitBRCONDBasic(N);
    case X86ISD::RET_FLAG:  return visitRET(N);
    case X86ISD::CALL:      return visitCALL(N);
  }
}

// Note: branch conditions, by definition, only have a chain user.
// This is why it should not be saved in a map for recall.
Value* X86IREmitter::visitBRCONDBasic(const SDNode *N) {
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

  SDNode *CPSR = N->getOperand(2)->getOperand(1).getNode();
  SDNode *CMPNode = NULL;

  for (SDNode::use_iterator I = CPSR->use_begin(), E = CPSR->use_end(); I != E;
      ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      if(I->getOperand(2)->getOpcode() == X86ISD::CMP){ //This could should go away...
        CMPNode = I->getOperand(2).getNode();
      }
    }
  }

  if (CMPNode == NULL) {
    //errs() << "X86IREmitter ERROR: Could not find CMP SDNode for BRCond!\n";
    //return NULL;
    DEBUG(outs() << "X86IREmitter::visitBRCONDBasic: Branch type is advanced -> visitBRCONDAdvanced\n");
    return visitBRCONDAdvanced(N);
  }

  //This code should currently be platform agnostic since CMPNode
  //    is an iterator that runs over ISD::CopyToReg, which is
  //    currently platform agnostic...
  Value *Cmp = NULL;
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

// Note: branch conditions, by definition, only have a chain user.
// This is why it should not be saved in a map for recall.
Value* X86IREmitter::visitBRCONDAdvanced(const SDNode *N) {
  // Get the address
  const CondCodeSDNode *Cond = dyn_cast<CondCodeSDNode>(N->getOperand(0));
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(1));

  if (!DestNode) {
    printError("X86IREmitter::visitBRCONDAdvanced: Not a constant integer for branch!");
    return NULL;
  }

  uint64_t DestInt = DestNode->getSExtValue();
  uint64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  // Note: pipeline is 8 bytes
  uint64_t Tgt = PC + DestInt;  //Address is added in X86 (potentially need to adjust ARM)

  Function *F = IRB->GetInsertBlock()->getParent();
  BasicBlock *CurBB = IRB->GetInsertBlock();
  BasicBlock *BBTgt = Dec->getOrCreateBasicBlock(Tgt, F);

  RegisterSDNode *CMPNode = NULL;                     //Store EFLAGS for the current Compare.
  SDNode *BinOpNode = NULL;
  uint32_t Ctr = 1;                                   //Reverse order through the graph from our current posistion
  SDValue Chain = N->getOperand(N->getNumOperands()-Ctr);
  while((N->getNumOperands()-Ctr) != 0){              //Get nearest CopyToReg || CopyFromReg
    if(Chain.getOpcode() == ISD::CopyToReg || Chain.getOpcode() == ISD::CopyFromReg){
      CMPNode = dyn_cast<RegisterSDNode>(Chain.getOperand(1).getNode());
      BinOpNode = Chain.getOperand(2).getNode();      //Get the math operation {add, mult, etc...}
      if(CMPNode->getReg() == X86::EFLAGS){           //Verify that we find the first EFLAGS Register
        break;
      }
    }
    Chain = N->getOperand(N->getNumOperands()-(Ctr++));
  }

  if(CMPNode == NULL || CMPNode->getReg() != X86::EFLAGS){  //Ensure we didn't just fall through the while loop
    printError("X86IREmitter::visitBRCOND: Could not find EFLAGS Register...");
  }

  /*
   * The EFLAGS is a 32-bit register used as a collection of bits representing
   *    Boolean values to store the results of operations and the state of the processor.
   *
   * Value  State
   * 0      CF      - Carry Flag                        - Status Flag
   * 1      1
   * 2      PF      - Parity Flag                       - Status Flag
   * 3      0
   * 4      AF      - Auxiliary Carry Flag              - Status Flag
   * 5      0
   * 6      ZF      - Zero Flag                         - Status Flag
   * 7      SF      - Sign Flag                         - Status Flag
   * 8      TF      - Trap Flag                         - System Flag
   * 9      IF      - Interruption Flag                 - System Flag
   * 10     DF      - Direction Flag                    - Control Flag
   * 11     OF      - Overflow Flag                     - Status Flag
   * 12/13  IOPL    - I/O Privilege Level field (2 bits)- System Flag
   * 14     NT      - Nested Task flag                  - System Flag
   * 0      0
   * 16     RF      - Resume Flag                       - System Flag
   * 17     VM      - Virtual-8086 Mode                 - System Flag
   * 18     AC      - Alignment Check                   - System Flag
   * 19     VIF     - Virtual Interrupt Flag            - System Flag
   * 20     VIP     - Virtual Interrupt Pending Flag    - System Flag
   * 21     ID      - Identification Flag               - System Flag
   * 22-31  0
   * Source: Fig 3-38 Intel Arch Software Devl Manual
   *    Section 3.4.3.1 has descriptions
   */

  //The switch needs to evolve to look at EFLAGS bit hunting...
  Value *Cmp = NULL;
  //Value *LHS = visit(CMPNode->getOperand(0).getNode());
  //Value *RHS = visit(CMPNode->getOperand(1).getNode());
  Value *Vis = visit(BinOpNode);
  //Get the current context - this includes the operation that populated EFLAGS
  //    That operation is the key component so that EFLAGS can be abstracted to
  //    native LLVM IR.
  LLVMContext *CurrentContext = Dec->getDisassembler()->getMCDirector()->getContext();
  //We need to convert the context into an Integer.  That integer will be the
  //    used for the conditional statement.  The entire point of this operation
  //    is to abstract EFLAGS as a register, which means we really don't care about
  //    the specific location of the flag, just its meaning and context so it can
  //    be translated into IR.  This is why the ZF flag maps into our ConstIntZero
  //    variable.  This works because the context provides the cmp instruction which
  //    performs a subtraction; the IR for that result is stored in the variable.
  ConstantInt *ConstIntZero = ConstantInt::get(IntegerType::get(*CurrentContext, 32), 0, true);
  switch(Cond->get()) {
  default:
    printError("Unknown condition code");
    return NULL;
  case ISD::SETEQ:  //JE_1: ZF == 1
    // ZF ISD::AND 32 (b100000)
    Cmp = IRB->CreateICmpEQ(ConstIntZero, Vis);
    outs() << "CMP " << Cmp << "\n";
    break;
  case ISD::SETNE:  //JNE_1: ZF == 0
    // ZF ISD::AND 32 (b100000) == 0
    //Cmp = IRB->CreateICmpNE(LHS, RHS);
    Cmp = IRB->CreateICmpNE(ConstIntZero, Vis);
    outs() << "CMP " << Cmp << "\n";
    break;
  case ISD::SETGE:  //JAE_1: CF == 0
    // CF ISD::AND 1 (b1) == 0
    //Cmp = IRB->CreateICmpSGE(LHS, RHS);
    break;
  case ISD::SETLT:  //JB_1: CF == 1
    // CF ISD::AND 1 (b1) == 1
    //Cmp = IRB->CreateICmpSLT(LHS, RHS);
    break;
  case ISD::SETGT:  //JA_1: CF == 0 && ZF == 0
    // ZF ISD::AND 32 (b100000) == 0 && CF ISD::AND 1 (b1) == 0
    //Cmp = IRB->CreateICmpSGT(LHS, RHS);
    break;
  case ISD::SETLE:  //JBE_1: CF == 1 || ZF == 1
    // ZF ISD::AND 32 (b100000) == 1 && CF ISD::AND 1 (b1) == 1
    //Cmp = IRB->CreateICmpSLE(LHS, RHS);
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
