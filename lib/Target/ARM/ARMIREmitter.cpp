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
	default: {
		errs() << "OpCode: " << N->getOpcode() << "\n";
		N->dump();
		llvm_unreachable("ARMIREmitter::visit - Every ARM visit should be implemented...");
		return NULL;
	}
    case ARMISD::Wrapper:				return visitWrapper(N);
    case ARMISD::WrapperPIC:			return visitWrapperPIC(N);
    case ARMISD::WrapperJT:				return visitWrapperJT(N);
    case ARMISD::COPY_STRUCT_BYVAL:		return visitCOPY_STRUCT_BYVAL(N);
    case ARMISD::CALL:					return visitCALL(N);
    case ARMISD::CALL_PRED:				return visitCALL_PRED(N);
    case ARMISD::CALL_NOLINK:			return visitCALL_NOLINK(N);
    case ARMISD::tCALL:					return visittCALL(N);
    case ARMISD::BRCOND:				return visitBRCOND(N);
    case ARMISD::BR_JT:					return visitBR_JT(N);
    case ARMISD::BR2_JT:				return visitBR2_JT(N);
    case ARMISD::RET_FLAG:				return visitRET_FLAG(N);
    case ARMISD::INTRET_FLAG:			return visitINTRET_FLAG(N);
    case ARMISD::PIC_ADD:				return visitPIC_ADD(N);
    case ARMISD::CMP:					return visitCMP(N);
    case ARMISD::CMN:					return visitCMN(N);
    case ARMISD::CMPZ:					return visitCMPZ(N);
    case ARMISD::CMPFP:					return visitCMPFP(N);
    case ARMISD::CMPFPw0:				return visitCMPFPw0(N);
    case ARMISD::FMSTAT:				return visitFMSTAT(N);
    case ARMISD::CMOV:					return visitCMOV(N);
    case ARMISD::BCC_i64:				return visitBCC_i64(N);
    case ARMISD::RBIT:					return visitRBIT(N);
    case ARMISD::FTOSI:					return visitFTOSI(N);
    case ARMISD::FTOUI:					return visitFTOUI(N);
    case ARMISD::SITOF:					return visitSITOF(N);
    case ARMISD::UITOF:					return visitUITOF(N);
    case ARMISD::SRL_FLAG:				return visitSRL_FLAG(N);
    case ARMISD::SRA_FLAG:				return visitSRA_FLAG(N);
    case ARMISD::RRX:					return visitRRX(N);
    case ARMISD::ADDC:					return visitADDC(N);
    case ARMISD::ADDE:					return visitADDE(N);
    case ARMISD::SUBC:					return visitSUBC(N);
    case ARMISD::SUBE:					return visitSUBE(N);
    case ARMISD::VMOVRRD:				return visitVMOVRRD(N);
    case ARMISD::VMOVDRR:				return visitVMOVDRR(N);
    case ARMISD::EH_SJLJ_SETJMP:		return visitEH_SJLJ_SETJMP(N);
    case ARMISD::EH_SJLJ_LONGJMP:		return visitEH_SJLJ_LONGJMP(N);
    case ARMISD::TC_RETURN:				return visitTC_RETURN(N);
    case ARMISD::THREAD_POINTER:		return visitTHREAD_POINTER(N);
    case ARMISD::DYN_ALLOC:				return visitDYN_ALLOC(N);
    case ARMISD::MEMBARRIER_MCR:		return visitMEMBARRIER_MCR(N);
    case ARMISD::PRELOAD:				return visitPRELOAD(N);
    case ARMISD::VCEQ:					return visitVCEQ(N);
    case ARMISD::VCEQZ:					return visitVCEQZ(N);
    case ARMISD::VCGE:					return visitVCGE(N);
    case ARMISD::VCGEZ:					return visitVCGEZ(N);
    case ARMISD::VCLEZ:					return visitVCLEZ(N);
    case ARMISD::VCGEU:					return visitVCGEU(N);
    case ARMISD::VCGT:					return visitVCGT(N);
    case ARMISD::VCGTZ:					return visitVCGTZ(N);
    case ARMISD::VCLTZ:					return visitVCLTZ(N);
    case ARMISD::VCGTU:					return visitVCGTU(N);
    case ARMISD::VTST:					return visitVTST(N);
    case ARMISD::VSHL:					return visitVSHL(N);
    case ARMISD::VSHRs:					return visitVSHRs(N);
    case ARMISD::VSHRu:					return visitVSHRu(N);
    case ARMISD::VSHLLs:				return visitVSHLLs(N);
    case ARMISD::VSHLLu:				return visitVSHLLu(N);
    case ARMISD::VSHLLi:				return visitVSHLLi(N);
    case ARMISD::VSHRN:					return visitVSHRN(N);
    case ARMISD::VRSHRs:				return visitVRSHRs(N);
    case ARMISD::VRSHRu:				return visitVRSHRu(N);
    case ARMISD::VRSHRN:				return visitVRSHRN(N);
    case ARMISD::VQSHLs:				return visitVQSHLs(N);
    case ARMISD::VQSHLu:				return visitVQSHLu(N);
    case ARMISD::VQSHLsu:				return visitVQSHLsu(N);
    case ARMISD::VQSHRNs:				return visitVQSHRNs(N);
    case ARMISD::VQSHRNu:				return visitVQSHRNu(N);
    case ARMISD::VQSHRNsu:				return visitVQSHRNsu(N);
    case ARMISD::VQRSHRNs:				return visitVQRSHRNs(N);
    case ARMISD::VQRSHRNu:				return visitVQRSHRNu(N);
    case ARMISD::VQRSHRNsu:				return visitVQRSHRNsu(N);
    case ARMISD::VSLI:					return visitVSLI(N);
    case ARMISD::VSRI:					return visitVSRI(N);
    case ARMISD::VGETLANEu:				return visitVGETLANEu(N);
    case ARMISD::VGETLANEs:				return visitVGETLANEs(N);
    case ARMISD::VMOVIMM:				return visitVMOVIMM(N);
    case ARMISD::VMVNIMM:				return visitVMVNIMM(N);
    case ARMISD::VMOVFPIMM:				return visitVMOVFPIMM(N);
    case ARMISD::VDUP:					return visitVDUP(N);
    case ARMISD::VDUPLANE:				return visitVDUPLANE(N);
    case ARMISD::VEXT:					return visitVEXT(N);
    case ARMISD::VREV64:				return visitVREV64(N);
    case ARMISD::VREV32:				return visitVREV32(N);
    case ARMISD::VREV16:				return visitVREV16(N);
    case ARMISD::VZIP:					return visitVZIP(N);
    case ARMISD::VUZP:					return visitVUZP(N);
    case ARMISD::VTRN:					return visitVTRN(N);
    case ARMISD::VTBL1:					return visitVTBL1(N);
    case ARMISD::VTBL2:					return visitVTBL2(N);
    case ARMISD::VMULLs:				return visitVMULLs(N);
    case ARMISD::VMULLu:				return visitVMULLu(N);
    case ARMISD::UMLAL:					return visitUMLAL(N);
    case ARMISD::SMLAL:					return visitSMLAL(N);
    case ARMISD::BUILD_VECTOR:			return visitBUILD_VECTOR(N);
    case ARMISD::FMAX:					return visitFMAX(N);
    case ARMISD::FMIN:					return visitFMIN(N);
    case ARMISD::VMAXNM:				return visitVMAXNM(N);
    case ARMISD::VMINNM:				return visitVMINNM(N);
    case ARMISD::BFI:					return visitBFI(N);
    case ARMISD::VORRIMM:				return visitVORRIMM(N);
    case ARMISD::VBICIMM:				return visitVBICIMM(N);
    case ARMISD::VBSL:					return visitVBSL(N);
    case ARMISD::VLD2DUP:				return visitVLD2DUP(N);
    case ARMISD::VLD3DUP:				return visitVLD3DUP(N);
    case ARMISD::VLD4DUP:				return visitVLD4DUP(N);
    case ARMISD::VLD1_UPD:				return visitVLD1_UPD(N);
    case ARMISD::VLD2_UPD:				return visitVLD2_UPD(N);
    case ARMISD::VLD3_UPD:				return visitVLD3_UPD(N);
    case ARMISD::VLD4_UPD:				return visitVLD4_UPD(N);
    case ARMISD::VLD2LN_UPD:			return visitVLD2LN_UPD(N);
    case ARMISD::VLD3LN_UPD:			return visitVLD3LN_UPD(N);
    case ARMISD::VLD4LN_UPD:			return visitVLD4LN_UPD(N);
    case ARMISD::VLD2DUP_UPD:			return visitVLD2DUP_UPD(N);
    case ARMISD::VLD3DUP_UPD:			return visitVLD3DUP_UPD(N);
    case ARMISD::VLD4DUP_UPD:			return visitVLD4DUP_UPD(N);
    case ARMISD::VST1_UPD:				return visitVST1_UPD(N);
    case ARMISD::VST2_UPD:				return visitVST2_UPD(N);
    case ARMISD::VST3_UPD:				return visitVST3_UPD(N);
    case ARMISD::VST4_UPD:				return visitVST4_UPD(N);
    case ARMISD::VST2LN_UPD:			return visitVST2LN_UPD(N);
    case ARMISD::VST3LN_UPD:			return visitVST3LN_UPD(N);
    case ARMISD::VST4LN_UPD:			return visitVST4LN_UPD(N);
  }
}

Value* ARMIREmitter::visitWrapper(const SDNode *N) { llvm_unreachable("visitWrapper unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitWrapperPIC(const SDNode *N) { llvm_unreachable("visitWrapperPIC unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitWrapperJT(const SDNode *N) { llvm_unreachable("visitWrapperJT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCOPY_STRUCT_BYVAL(const SDNode *N) { llvm_unreachable("visitCOPY_STRUCT_BYVAL unimplemented ARM visit..."); return NULL; }

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

Value* ARMIREmitter::visitCALL_PRED(const SDNode *N) { llvm_unreachable("visitCALL_PRED unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCALL_NOLINK(const SDNode *N) { llvm_unreachable("visitCALL_NOLINK unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visittCALL(const SDNode *N) { llvm_unreachable("visittCALL unimplemented ARM visit..."); return NULL; }

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

Value* ARMIREmitter::visitBR_JT(const SDNode *N) { llvm_unreachable("visitBR_JT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitBR2_JT(const SDNode *N) { llvm_unreachable("visitBR2_JT unimplemented ARM visit..."); return NULL; }

Value* ARMIREmitter::visitRET_FLAG(const SDNode *N) {
  return IRB->CreateRetVoid();
}

Value* ARMIREmitter::visitINTRET_FLAG(const SDNode *N) { llvm_unreachable("visitINTRET_FLAG unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitPIC_ADD(const SDNode *N) { llvm_unreachable("visitPIC_ADD unimplemented ARM visit..."); return NULL; }

Value* ARMIREmitter::visitCMP(const SDNode *N) {
	errs() << "visitCMP unimplemented ARM visit...\n";
	return NULL;
}

Value* ARMIREmitter::visitCMN(const SDNode *N) { llvm_unreachable("visitCMN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCMPZ(const SDNode *N) { llvm_unreachable("visitCMPZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCMPFP(const SDNode *N) { llvm_unreachable("visitCMPFP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCMPFPw0(const SDNode *N) { llvm_unreachable("visitCMPFPw0 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitFMSTAT(const SDNode *N) { llvm_unreachable("visitFMSTAT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitCMOV(const SDNode *N) { llvm_unreachable("visitCMOV unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitBCC_i64(const SDNode *N) { llvm_unreachable("visitBCC_i64 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitRBIT(const SDNode *N) { llvm_unreachable("visitRBIT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitFTOSI(const SDNode *N) { llvm_unreachable("visitFTOSI unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitFTOUI(const SDNode *N) { llvm_unreachable("visitFTOUI unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSITOF(const SDNode *N) { llvm_unreachable("visitSITOF unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitUITOF(const SDNode *N) { llvm_unreachable("visitUITOF unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSRL_FLAG(const SDNode *N) { llvm_unreachable("visitSRL_FLAG unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSRA_FLAG(const SDNode *N) { llvm_unreachable("visitSRA_FLAG unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitRRX(const SDNode *N) { llvm_unreachable("visitRRX unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitADDC(const SDNode *N) { llvm_unreachable("visitADDC unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitADDE(const SDNode *N) { llvm_unreachable("visitADDE unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSUBC(const SDNode *N) { llvm_unreachable("visitSUBC unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSUBE(const SDNode *N) { llvm_unreachable("visitSUBE unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMOVRRD(const SDNode *N) { llvm_unreachable("visitVMOVRRD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMOVDRR(const SDNode *N) { llvm_unreachable("visitVMOVDRR unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitEH_SJLJ_SETJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_SETJMP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitEH_SJLJ_LONGJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_LONGJMP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitTC_RETURN(const SDNode *N) { llvm_unreachable("visitTC_RETURN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitTHREAD_POINTER(const SDNode *N) { llvm_unreachable("visitTHREAD_POINTER unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitDYN_ALLOC(const SDNode *N) { llvm_unreachable("visitDYN_ALLOC unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitMEMBARRIER_MCR(const SDNode *N) { llvm_unreachable("visitMEMBARRIER_MCR unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitPRELOAD(const SDNode *N) { llvm_unreachable("visitPRELOAD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCEQ(const SDNode *N) { llvm_unreachable("visitVCEQ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCEQZ(const SDNode *N) { llvm_unreachable("visitVCEQZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGE(const SDNode *N) { llvm_unreachable("visitVCGE unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGEZ(const SDNode *N) { llvm_unreachable("visitVCGEZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCLEZ(const SDNode *N) { llvm_unreachable("visitVCLEZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGEU(const SDNode *N) { llvm_unreachable("visitVCGEU unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGT(const SDNode *N) { llvm_unreachable("visitVCGT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGTZ(const SDNode *N) { llvm_unreachable("visitVCGTZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCLTZ(const SDNode *N) { llvm_unreachable("visitVCLTZ unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVCGTU(const SDNode *N) { llvm_unreachable("visitVCGTU unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVTST(const SDNode *N) { llvm_unreachable("visitVTST unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHL(const SDNode *N) { llvm_unreachable("visitVSHL unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHRs(const SDNode *N) { llvm_unreachable("visitVSHRs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHRu(const SDNode *N) { llvm_unreachable("visitVSHRu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHLLs(const SDNode *N) { llvm_unreachable("visitVSHLLs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHLLu(const SDNode *N) { llvm_unreachable("visitVSHLLu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHLLi(const SDNode *N) { llvm_unreachable("visitVSHLLi unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSHRN(const SDNode *N) { llvm_unreachable("visitVSHRN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVRSHRs(const SDNode *N) { llvm_unreachable("visitVRSHRs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVRSHRu(const SDNode *N) { llvm_unreachable("visitVRSHRu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVRSHRN(const SDNode *N) { llvm_unreachable("visitVRSHRN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHLs(const SDNode *N) { llvm_unreachable("visitVQSHLs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHLu(const SDNode *N) { llvm_unreachable("visitVQSHLu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHLsu(const SDNode *N) { llvm_unreachable("visitVQSHLsu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHRNs(const SDNode *N) { llvm_unreachable("visitVQSHRNs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHRNu(const SDNode *N) { llvm_unreachable("visitVQSHRNu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQSHRNsu(const SDNode *N) { llvm_unreachable("visitVQSHRNsu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQRSHRNs(const SDNode *N) { llvm_unreachable("visitVQRSHRNs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQRSHRNu(const SDNode *N) { llvm_unreachable("visitVQRSHRNu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVQRSHRNsu(const SDNode *N) { llvm_unreachable("visitVQRSHRNsu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSLI(const SDNode *N) { llvm_unreachable("visitVSLI unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVSRI(const SDNode *N) { llvm_unreachable("visitVSRI unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVGETLANEu(const SDNode *N) { llvm_unreachable("visitVGETLANEu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVGETLANEs(const SDNode *N) { llvm_unreachable("visitVGETLANEs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMOVIMM(const SDNode *N) { llvm_unreachable("visitVMOVIMM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMVNIMM(const SDNode *N) { llvm_unreachable("visitVMVNIMM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMOVFPIMM(const SDNode *N) { llvm_unreachable("visitVMOVFPIMM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVDUP(const SDNode *N) { llvm_unreachable("visitVDUP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVDUPLANE(const SDNode *N) { llvm_unreachable("visitVDUPLANE unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVEXT(const SDNode *N) { llvm_unreachable("visitVEXT unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVREV64(const SDNode *N) { llvm_unreachable("visitVREV64 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVREV32(const SDNode *N) { llvm_unreachable("visitVREV32 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVREV16(const SDNode *N) { llvm_unreachable("visitVREV16 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVZIP(const SDNode *N) { llvm_unreachable("visitVZIP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVUZP(const SDNode *N) { llvm_unreachable("visitVUZP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVTRN(const SDNode *N) { llvm_unreachable("visitVTRN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVTBL1(const SDNode *N) { llvm_unreachable("visitVTBL1 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVTBL2(const SDNode *N) { llvm_unreachable("visitVTBL2 unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMULLs(const SDNode *N) { llvm_unreachable("visitVMULLs unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMULLu(const SDNode *N) { llvm_unreachable("visitVMULLu unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitUMLAL(const SDNode *N) { llvm_unreachable("visitUMLAL unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitSMLAL(const SDNode *N) { llvm_unreachable("visitSMLAL unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitBUILD_VECTOR(const SDNode *N) { llvm_unreachable("visitBUILD_VECTOR unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitFMAX(const SDNode *N) { llvm_unreachable("visitFMAX unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitFMIN(const SDNode *N) { llvm_unreachable("visitFMIN unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMAXNM(const SDNode *N) { llvm_unreachable("visitVMAXNM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVMINNM(const SDNode *N) { llvm_unreachable("visitVMINNM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitBFI(const SDNode *N) { llvm_unreachable("visitBFI unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVORRIMM(const SDNode *N) { llvm_unreachable("visitVORRIMM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVBICIMM(const SDNode *N) { llvm_unreachable("visitVBICIMM unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVBSL(const SDNode *N) { llvm_unreachable("visitVBSL unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD2DUP(const SDNode *N) { llvm_unreachable("visitVLD2DUP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD3DUP(const SDNode *N) { llvm_unreachable("visitVLD3DUP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD4DUP(const SDNode *N) { llvm_unreachable("visitVLD4DUP unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD1_UPD(const SDNode *N) { llvm_unreachable("visitVLD1_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD2_UPD(const SDNode *N) { llvm_unreachable("visitVLD2_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD3_UPD(const SDNode *N) { llvm_unreachable("visitVLD3_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD4_UPD(const SDNode *N) { llvm_unreachable("visitVLD4_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD2LN_UPD(const SDNode *N) { llvm_unreachable("visitVLD2LN_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD3LN_UPD(const SDNode *N) { llvm_unreachable("visitVLD3LN_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD4LN_UPD(const SDNode *N) { llvm_unreachable("visitVLD4LN_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD2DUP_UPD(const SDNode *N) { llvm_unreachable("visitVLD2DUP_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD3DUP_UPD(const SDNode *N) { llvm_unreachable("visitVLD3DUP_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVLD4DUP_UPD(const SDNode *N) { llvm_unreachable("visitVLD4DUP_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST1_UPD(const SDNode *N) { llvm_unreachable("visitVST1_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST2_UPD(const SDNode *N) { llvm_unreachable("visitVST2_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST3_UPD(const SDNode *N) { llvm_unreachable("visitVST3_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST4_UPD(const SDNode *N) { llvm_unreachable("visitVST4_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST2LN_UPD(const SDNode *N) { llvm_unreachable("visitVST2LN_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST3LN_UPD(const SDNode *N) { llvm_unreachable("visitVST3LN_UPD unimplemented ARM visit..."); return NULL; }
Value* ARMIREmitter::visitVST4LN_UPD(const SDNode *N) { llvm_unreachable("visitVST4LN_UPD unimplemented ARM visit..."); return NULL; }

} // end fracture namespace
