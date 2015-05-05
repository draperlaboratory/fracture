//===--- IREmitter - Emits IR from SDnodes ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class uses SDNodes and emits IR. It is intended to be extended by Target
// implementations who have special ISD legalization nodes.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: October 16, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/IREmitter.h"
#include "CodeInv/Decompiler.h"

using namespace llvm;

#define DEBUG_TYPE "iremitter"

namespace fracture {

IREmitter::IREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : Infos(InfoOut), Errs(ErrOut) {
  Dec = TheDec;
  DAG = Dec->getCurrentDAG();
  IRB = new IRBuilder<>(getGlobalContext());
  RegMap.grow(Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo(
     )->getNumRegs());
}

IREmitter::~IREmitter() {
  delete IRB;
}

void IREmitter::EmitIR(BasicBlock *BB, SDNode *CurNode,
    std::stack<SDNode *> &NodeStack, std::map<SDValue, Value*> OpMap) {
  // Who uses this node (so we can find the next node)
  for (SDNode::use_iterator I = CurNode->use_begin(), E = CurNode->use_end();
      I != E; ++I) {
    // Save any chain uses to the Nodestack (to guarantee they get evaluated)
    //I->dump();
    //outs() << "EVT String: " << I.getUse().getValueType().getEVTString() << "\n";
    if (I.getUse().getValueType() == MVT::Other) {
      NodeStack.push(*I);
      continue;
    }
  }

  Value *IRVal = visit(CurNode);

  if (IRVal != NULL && ReturnInst::classof(IRVal)) {
    // Reset Data Structures
    RegMap.clear();
    VisitMap.clear();
    BaseNames.clear();
    RegMap.grow(Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo(
     )->getNumRegs());
  }
}

StringRef IREmitter::getIndexedValueName(StringRef BaseName) {
  const ValueSymbolTable &ST = Dec->getModule()->getValueSymbolTable();

  // In the common case, the name is not already in the symbol table.
  Value *V = ST.lookup(BaseName);
  if (V == NULL) {
    return BaseName;
  }

  // Otherwise, there is a naming conflict.  Rename this value.
  // FIXME: AFAIK this is never deallocated (memory leak). It should be free'd
  // after it gets added to the symbol table (which appears to do a copy as
  // indicated by the original code that stack allocated this variable).
  SmallString<256> *UniqueName =
    new SmallString<256>(BaseName.begin(), BaseName.end());
  unsigned Size = BaseName.size();

  // Add '_' as the last character when BaseName ends in a number
  if (BaseName[Size-1] <= '9' && BaseName[Size-1] >= '0') {
    UniqueName->resize(Size+1);
    (*UniqueName)[Size] = '_';
    Size++;
  }

  unsigned LastUnique = 0;
  while (1) {
    // Trim any suffix off and append the next number.
    UniqueName->resize(Size);
    raw_svector_ostream(*UniqueName) << ++LastUnique;

    // Try insert the vmap entry with this suffix.
    V = ST.lookup(*UniqueName);
    // FIXME: ^^ this lookup does not appear to be working on non-globals...
    // Temporary Fix: check if it has a basenames entry
    if (V == NULL && BaseNames[*UniqueName].empty()) {
      BaseNames[*UniqueName] = BaseName;
      return *UniqueName;
    }
  }
}

StringRef IREmitter::getBaseValueName(StringRef BaseName) {
  // Note: An alternate approach would be to pull the Symbol table and
  // do a string search, but this is much easier to implement.
  StringRef Res = BaseNames.lookup(BaseName);
  if (Res.empty()) {
    return BaseName;
  }
  return Res;
}

StringRef IREmitter::getInstructionName(const SDNode *N) {
  // Look for register name in CopyToReg user
  for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
    if (I->getOpcode() == ISD::CopyToReg) {
      return getIndexedValueName(
        visitRegister(I->getOperand(1).getNode())->getName());
      //FIXME - Favor the first result number.  (EFLAGS vs ESI x86)
    }
  }
  return StringRef();
}

Value* IREmitter::visit(const SDNode *N) {
  // Note: Extenders of this class should probably copy the following block
  // Also note, however, that it is up to the visit function wether to save to
  // the map, because these functions have the option to return null.
  if (VisitMap.find(N) != VisitMap.end()) {
    return VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());

  DEBUG(Infos << "Visiting Node: ");
  DEBUG(N->print(Infos));
  DEBUG(Infos << "\n");
  DEBUG(Infos << format("%1" PRIx64, Dec->getDisassembler()->getDebugOffset(N->getDebugLoc())) << "\n");

  switch (N->getOpcode()) {
    default:{
      errs() << "OpCode: " << N->getOpcode() << "\n";
      N->dump();
      llvm_unreachable("IREmitter::visit - Every visit should be implemented...");
      return NULL;
    }
    // Do nothing nodes
    case ISD::EntryToken:         return NULL;
    case ISD::HANDLENODE:         EndHandleDAG = true;
    case ISD::UNDEF:              return NULL;
    case ISD::CopyFromReg:        return visitCopyFromReg(N);
    case ISD::CopyToReg:          return visitCopyToReg(N);
    case ISD::Constant:           return visitConstant(N);
    case ISD::TokenFactor:        return visitTokenFactor(N);
    case ISD::MERGE_VALUES:       return visitMERGE_VALUES(N);
    case ISD::ADD:                return visitADD(N);
    case ISD::SUB:                return visitSUB(N);
    case ISD::ADDC:               return visitADDC(N);
    case ISD::SUBC:               return visitSUBC(N);
    case ISD::ADDE:               return visitADDE(N);
    case ISD::SUBE:               return visitSUBE(N);
    case ISD::MUL:                return visitMUL(N);
    case ISD::SDIV:               return visitSDIV(N);
    case ISD::UDIV:               return visitUDIV(N);
    case ISD::SREM:               return visitSREM(N);
    case ISD::UREM:               return visitUREM(N);
    case ISD::MULHU:              return visitMULHU(N);
    case ISD::MULHS:              return visitMULHS(N);
    case ISD::SMUL_LOHI:          return visitSMUL_LOHI(N);
    case ISD::UMUL_LOHI:          return visitUMUL_LOHI(N);
    case ISD::SMULO:              return visitSMULO(N);
    case ISD::UMULO:              return visitUMULO(N);
    case ISD::SDIVREM:            return visitSDIVREM(N);
    case ISD::UDIVREM:            return visitUDIVREM(N);
    case ISD::AND:                return visitAND(N);
    case ISD::OR:                 return visitOR(N);
    case ISD::XOR:                return visitXOR(N);
    case ISD::SHL:                return visitSHL(N);
    case ISD::SRA:                return visitSRA(N);
    case ISD::SRL:                return visitSRL(N);
    case ISD::CTLZ:               return visitCTLZ(N);
    case ISD::CTLZ_ZERO_UNDEF:    return visitCTLZ_ZERO_UNDEF(N);
    case ISD::CTTZ:               return visitCTTZ(N);
    case ISD::CTTZ_ZERO_UNDEF:    return visitCTTZ_ZERO_UNDEF(N);
    case ISD::CTPOP:              return visitCTPOP(N);
    case ISD::SELECT:             return visitSELECT(N);
    case ISD::VSELECT:            return visitVSELECT(N);
    case ISD::SELECT_CC:          return visitSELECT_CC(N);
    case ISD::SETCC:              return visitSETCC(N);
    case ISD::SIGN_EXTEND:        return visitSIGN_EXTEND(N);
    case ISD::ZERO_EXTEND:        return visitZERO_EXTEND(N);
    case ISD::ANY_EXTEND:         return visitANY_EXTEND(N);
    case ISD::SIGN_EXTEND_INREG:  return visitSIGN_EXTEND_INREG(N);
    case ISD::TRUNCATE:           return visitTRUNCATE(N);
    case ISD::BITCAST:            return visitBITCAST(N);
    case ISD::BUILD_PAIR:         return visitBUILD_PAIR(N);
    case ISD::FADD:               return visitFADD(N);
    case ISD::FSUB:               return visitFSUB(N);
    case ISD::FMUL:               return visitFMUL(N);
    case ISD::FMA:                return visitFMA(N);
    case ISD::FDIV:               return visitFDIV(N);
    case ISD::FREM:               return visitFREM(N);
    case ISD::FCOPYSIGN:          return visitFCOPYSIGN(N);
    case ISD::SINT_TO_FP:         return visitSINT_TO_FP(N);
    case ISD::UINT_TO_FP:         return visitUINT_TO_FP(N);
    case ISD::FP_TO_SINT:         return visitFP_TO_SINT(N);
    case ISD::FP_TO_UINT:         return visitFP_TO_UINT(N);
    case ISD::FP_ROUND:           return visitFP_ROUND(N);
    case ISD::FP_ROUND_INREG:     return visitFP_ROUND_INREG(N);
    case ISD::FP_EXTEND:          return visitFP_EXTEND(N);
    case ISD::FNEG:               return visitFNEG(N);
    case ISD::FABS:               return visitFABS(N);
    case ISD::FFLOOR:             return visitFFLOOR(N);
    case ISD::FCEIL:              return visitFCEIL(N);
    case ISD::FTRUNC:             return visitFTRUNC(N);
    case ISD::BRCOND:             return visitBRCOND(N);
    case ISD::BR:                 return visitBR(N);
    case ISD::BR_CC:              return visitBR_CC(N);
    case ISD::LOAD:               return visitLOAD(N);
    case ISD::STORE:              return visitSTORE(N);
    case ISD::INSERT_VECTOR_ELT:  return visitINSERT_VECTOR_ELT(N);
    case ISD::EXTRACT_VECTOR_ELT: return visitEXTRACT_VECTOR_ELT(N);
    case ISD::BUILD_VECTOR:       return visitBUILD_VECTOR(N);
    case ISD::CONCAT_VECTORS:     return visitCONCAT_VECTORS(N);
    case ISD::EXTRACT_SUBVECTOR:  return visitEXTRACT_SUBVECTOR(N);
    case ISD::VECTOR_SHUFFLE:     return visitVECTOR_SHUFFLE(N);
  }
  return NULL;
}

Value* IREmitter::visitCopyFromReg(const SDNode *N) {
  // Operand 0 - Chain node (ignored)
  // Operand 1 - RegisterSDNode, a machine register. We create an alloca, which
  //             is typically removed in a mem2reg pass

  // Skip if the register is never used. This happens for %noreg registers.
  if (!N->hasAnyUseOfValue(0)) {
    return NULL;
  }

  Value *RegVal = visitRegister(N->getOperand(1).getNode());
  if (RegVal == NULL) {
    errs() << "visitCopyFromReg: Invalid Register!\n";
    return NULL;
  }

  StringRef Name = getIndexedValueName(RegVal->getName());
  Instruction* Res = IRB->CreateLoad(RegVal, Name);
  VisitMap[N] = Res;
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* IREmitter::visitCopyToReg(const SDNode *N) {
  // Operand 0 - Chain node (ignored)
  // Operand 1 - Register Destination
  // Operand 2 - Source
  Value *RegVal = visitRegister(N->getOperand(1).getNode());
  Value* V = visit(N->getOperand(2).getNode());

  if (V == NULL || RegVal == NULL) {
    errs() << "Null values on CopyToReg, skipping!\n";
    return NULL;
  }

  //errs() << "V:\t" <<"Output Type: " << V->getType()->getScalarSizeInBits() <<"\n";
  //V->dump();
  //errs() << "RegVal:\t" <<"Output Type: " << RegVal->getType()->getScalarSizeInBits() <<"\n";
  //RegVal->dump();

  Instruction* Res = IRB->CreateStore(V, RegVal);
  VisitMap[N] = Res;
  Res->setDebugLoc(N->getDebugLoc());
  return Res;
}

Value* IREmitter::visitConstant(const SDNode *N) {
  if (const ConstantSDNode *CSDN = dyn_cast<ConstantSDNode>(N)) {
    Value *Res = Constant::getIntegerValue(
      N->getValueType(0).getTypeForEVT(getGlobalContext()),
      CSDN->getAPIntValue());
    VisitMap[N] = Res;
    return Res;
  } else {
    Errs << "Could not convert ISD::Constant to integer!\n";
  }
  return NULL;
}


Value* IREmitter::visitTokenFactor(const SDNode *N) { llvm_unreachable("visitTokenFactor Unimplemented visit..."); return NULL; }
Value* IREmitter::visitMERGE_VALUES(const SDNode *N) { llvm_unreachable("visitMERGE_VALUES Unimplemented visit..."); return NULL; }

Value* IREmitter::visitADD(const SDNode *N) {
  // Operand 0 and 1 are values to add
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateAdd(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}

Value* IREmitter::visitSUB(const SDNode *N) {
  // Operand 0 and 1 are values to sub
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  //outs() << "IREmitter::visitSUB: " << Name.str() << " op0 op1 " << Op0 << " "<< Op1 << "\n";
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateSub(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}

Value* IREmitter::visitADDC(const SDNode *N) { llvm_unreachable("visitADDC Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSUBC(const SDNode *N) { llvm_unreachable("visitSUBC Unimplemented visit..."); return NULL; }
Value* IREmitter::visitADDE(const SDNode *N) { llvm_unreachable("visitADDE Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSUBE(const SDNode *N) { llvm_unreachable("visitSUBE Unimplemented visit..."); return NULL; }

Value* IREmitter::visitMUL(const SDNode *N) {
  // Operand 0 and 1 are values to sub
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  //outs() << "IREmitter::visitSUB: " << Name.str() << " op0 op1 " << Op0 << " "<< Op1 << "\n";
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateMul(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}

Value* IREmitter::visitSDIV(const SDNode *N) { llvm_unreachable("visitSDIV Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUDIV(const SDNode *N) { llvm_unreachable("visitUDIV Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSREM(const SDNode *N) { llvm_unreachable("visitSREM Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUREM(const SDNode *N) { llvm_unreachable("visitUREM Unimplemented visit..."); return NULL; }
Value* IREmitter::visitMULHU(const SDNode *N) { llvm_unreachable("visitMULHU Unimplemented visit..."); return NULL; }
Value* IREmitter::visitMULHS(const SDNode *N) { llvm_unreachable("visitMULHS Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSMUL_LOHI(const SDNode *N) { llvm_unreachable("visitSMUL_LOHI Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUMUL_LOHI(const SDNode *N) { llvm_unreachable("visitUMUL_LOHI Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSMULO(const SDNode *N) { llvm_unreachable("visitSMULO Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUMULO(const SDNode *N) { llvm_unreachable("visitUMULO Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSDIVREM(const SDNode *N) { llvm_unreachable("visitSDIVREM Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUDIVREM(const SDNode *N) { llvm_unreachable("visitUDIVREM Unimplemented visit..."); return NULL; }
Value* IREmitter::visitAND(const SDNode *N) {
  // Operand 0 and 1 are values to sub
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  //outs() << "IREmitter::visitXOR: " << Name.str() << " op0 op1 " << Op0 << " "<< Op1 << "\n";
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateAnd(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}
Value* IREmitter::visitOR(const SDNode *N) {
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateOr(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}
Value* IREmitter::visitXOR(const SDNode *N) {
  // Operand 0 and 1 are values to sub
  Value *Op0 = visit(N->getOperand(0).getNode());
  Value *Op1 = visit(N->getOperand(1).getNode());
  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  //outs() << "IREmitter::visitXOR: " << Name.str() << " op0 op1 " << Op0 << " "<< Op1 << "\n";
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateXor(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}
Value* IREmitter::visitSHL(const SDNode *N) { llvm_unreachable("visitSHL Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSRA(const SDNode *N) { llvm_unreachable("visitSRA Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSRL(const SDNode *N) { llvm_unreachable("visitSRL Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCTLZ(const SDNode *N) { llvm_unreachable("visitCTLZ Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCTLZ_ZERO_UNDEF(const SDNode *N) { llvm_unreachable("visitCTLZ_ZERO_UNDEF Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCTTZ(const SDNode *N) { llvm_unreachable("visitCTTZ Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCTTZ_ZERO_UNDEF(const SDNode *N) { llvm_unreachable("visitCTTZ_ZERO_UNDEF Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCTPOP(const SDNode *N) { llvm_unreachable("visitCTPOP Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSELECT(const SDNode *N) { llvm_unreachable("visitSELECT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitVSELECT(const SDNode *N) { llvm_unreachable("visitVSELECT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSELECT_CC(const SDNode *N) { llvm_unreachable("visitSELECT_CC Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSETCC(const SDNode *N) { llvm_unreachable("visitSETCC Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSIGN_EXTEND(const SDNode *N) { llvm_unreachable("visitSIGN_EXTEND Unimplemented visit..."); return NULL; }
Value* IREmitter::visitZERO_EXTEND(const SDNode *N) { llvm_unreachable("visitZERO_EXTEND Unimplemented visit..."); return NULL; }
Value* IREmitter::visitANY_EXTEND(const SDNode *N) { llvm_unreachable("visitANY_EXTEND Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSIGN_EXTEND_INREG(const SDNode *N) { llvm_unreachable("visitSIGN_EXTEND_INREG Unimplemented visit..."); return NULL; }
Value* IREmitter::visitTRUNCATE(const SDNode *N) { llvm_unreachable("visitTRUNCATE Unimplemented visit..."); return NULL; }
Value* IREmitter::visitBITCAST(const SDNode *N) { llvm_unreachable("visitBITCAST Unimplemented visit..."); return NULL; }
Value* IREmitter::visitBUILD_PAIR(const SDNode *N) { llvm_unreachable("visitBUILD_PAIR Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFADD(const SDNode *N) { llvm_unreachable("visitFADD Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFSUB(const SDNode *N) { llvm_unreachable("visitFSUB Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFMUL(const SDNode *N) { llvm_unreachable("visitFMUL Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFMA(const SDNode *N) { llvm_unreachable("visitFMA Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFDIV(const SDNode *N) { llvm_unreachable("visitFDIV Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFREM(const SDNode *N) { llvm_unreachable("visitFREM Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFCOPYSIGN(const SDNode *N) { llvm_unreachable("visitFCOPYSIGN Unimplemented visit..."); return NULL; }
Value* IREmitter::visitSINT_TO_FP(const SDNode *N) { llvm_unreachable("visitSINT_TO_FP Unimplemented visit..."); return NULL; }
Value* IREmitter::visitUINT_TO_FP(const SDNode *N) { llvm_unreachable("visitUINT_TO_FP Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFP_TO_SINT(const SDNode *N) { llvm_unreachable("visitFP_TO_SINT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFP_TO_UINT(const SDNode *N) { llvm_unreachable("visitFP_TO_UINT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFP_ROUND(const SDNode *N) { llvm_unreachable("visitFP_ROUND Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFP_ROUND_INREG(const SDNode *N) { llvm_unreachable("visitFP_ROUND_INREG Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFP_EXTEND(const SDNode *N) { llvm_unreachable("visitFP_EXTEND Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFNEG(const SDNode *N) { llvm_unreachable("visitFNEG Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFABS(const SDNode *N) { llvm_unreachable("visitFABS Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFCEIL(const SDNode *N) { llvm_unreachable("visitFCEIL Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFTRUNC(const SDNode *N) { llvm_unreachable("visitFTRUNC Unimplemented visit..."); return NULL; }
Value* IREmitter::visitFFLOOR(const SDNode *N) { llvm_unreachable("visitFFLOOR Unimplemented visit..."); return NULL; }
Value* IREmitter::visitBRCOND(const SDNode *N) { llvm_unreachable("visitBRCOND Unimplemented visit..."); return NULL; }

Value* IREmitter::visitBR(const SDNode *N) {
  //llvm_unreachable("Unimplemented visit..."); return NULL;
    // Get the address
    const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(0));

    if (!DestNode) {
      printError("visitBR: Not a constant integer for branch!");
      return NULL;
    }

    uint64_t DestInt = DestNode->getSExtValue();
    uint64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
    // Note: pipeline is 8 bytes
    uint64_t Tgt = PC + DestInt;  //Address is added in X86 (potentially need to adjust ARM)

    Function *F = IRB->GetInsertBlock()->getParent();
    //BasicBlock *CurBB = IRB->GetInsertBlock();

    BasicBlock *BBTgt = Dec->getOrCreateBasicBlock(Tgt, F);

    Instruction *Br = IRB->CreateBr(BBTgt);
    Br->setDebugLoc(N->getDebugLoc());
    return Br;
}

Value* IREmitter::visitBR_CC(const SDNode *N) { llvm_unreachable("visitBR_CC Unimplemented visit..."); return NULL; }

Value* IREmitter::visitLOAD(const SDNode *N) { 
  // Operand 0 - Addr to load, should be a pointer
  // Operand 1 - undef (ignored)
  // Operand 2 - Chain (ignored)
  // outs() << N->getDebugLoc().getLine() << "<<<\n";
  Value *Addr = visit(N->getOperand(0).getNode());
  StringRef BaseName = getBaseValueName(Addr->getName());
  StringRef Name = getIndexedValueName(BaseName);

  if (!Addr->getType()->isPointerTy()) {
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
    (dyn_cast<Instruction>(Addr))->setDebugLoc(N->getDebugLoc());
  }
  Name = getIndexedValueName(BaseName);
  Instruction *Res = IRB->CreateLoad(Addr, Name);
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}

Value* IREmitter::visitSTORE(const SDNode *N) {
  // Operand 0 - The Value to store, usually a register or Constant
  // Operand 1 - An address/register+offset, assuming addressing modes were
  //             implemented properly
  // Operand 2 - undef (ignored)
  // Operand 3 - Chain (ignored)
  Value* StoreVal = visit(N->getOperand(0).getNode());
  Value* Addr = visit(N->getOperand(1).getNode());
  StringRef Name = getIndexedValueName(getBaseValueName(Addr->getName()));

  if (!Addr->getType()->isPointerTy()) {
    //errs() << "-----Dump1\n";
    //Addr->dump();
    //N->getDebugLoc().dump(getGlobalContext());
    Addr = IRB->CreateIntToPtr(Addr, Addr->getType()->getPointerTo(), Name);
    //errs() << "-----Dump2\n";
    //Addr->dump();
    //N->getDebugLoc().dump(getGlobalContext());
    (dyn_cast<Instruction>(Addr))->setDebugLoc(N->getDebugLoc());
  }

  Instruction *Res = IRB->CreateStore(StoreVal, Addr);
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}

Value* IREmitter::visitINSERT_VECTOR_ELT(const SDNode *N) { llvm_unreachable("visitINSERT_VECTOR_ELT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitEXTRACT_VECTOR_ELT(const SDNode *N) { llvm_unreachable("visitEXTRACT_VECTOR_ELT Unimplemented visit..."); return NULL; }
Value* IREmitter::visitBUILD_VECTOR(const SDNode *N) { llvm_unreachable("visitBUILD_VECTOR Unimplemented visit..."); return NULL; }
Value* IREmitter::visitCONCAT_VECTORS(const SDNode *N) { llvm_unreachable("visitCONCAT_VECTORS Unimplemented visit..."); return NULL; }
Value* IREmitter::visitEXTRACT_SUBVECTOR(const SDNode *N) { llvm_unreachable("visitEXTRACT_SUBVECTOR Unimplemented visit..."); return NULL; }
Value* IREmitter::visitVECTOR_SHUFFLE(const SDNode *N) { llvm_unreachable("visitVECTOR_SHUFFLE Unimplemented visit..."); return NULL; }

Value* IREmitter::visitRegister(const SDNode *N) {
  const RegisterSDNode *R = dyn_cast<RegisterSDNode>(N);
  if (R == NULL) {
    errs() << "visitRegister with no register!?\n";
    return NULL;
  }

  Value *Reg = RegMap[R->getReg()];
  if (Reg == NULL) {
    // Regname is %regname when printed this way.
    std::string RegName;
    raw_string_ostream RP(RegName);
    RP << PrintReg(R->getReg(),
      DAG ? DAG->getTarget().getSubtargetImpl()->getRegisterInfo() : 0);
    RegName = RP.str().substr(1, RegName.size());

    Type* Ty = R->getValueType(0).getTypeForEVT(getGlobalContext());

    Reg = Dec->getModule()->getGlobalVariable(RegName);
    if (Reg == NULL) {
      Constant *Initializer = Constant::getNullValue(Ty);
      Reg = new GlobalVariable(*Dec->getModule(), // Module
                               Ty,                // Type
                               false,             // isConstant
                               GlobalValue::ExternalLinkage,
                               Initializer,
                               RegName);
    }
    RegMap[R->getReg()] = Reg;
  }
  return Reg;
}

Value* IREmitter::visitCALL(const SDNode *N) {
  const ConstantSDNode *DestNode = dyn_cast<ConstantSDNode>(N->getOperand(0));
  if (!DestNode) {
    printError("visitCALL: Not a constant integer for call!");
    return NULL;
  }

  int64_t DestInt = DestNode->getSExtValue();
  int64_t PC = Dec->getDisassembler()->getDebugOffset(N->getDebugLoc());
  unsigned InstrSize =
      Dec->getDisassembler()->getMachineInstr(PC)->getDesc().Size;
  // Note: This handles variable and fixed width pipelines
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

} // End namespace fracture
