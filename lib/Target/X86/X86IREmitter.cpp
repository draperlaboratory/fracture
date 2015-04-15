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

#define DEBUG_TYPE "x86iremitter"

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
    default:{
      errs() << "OpCode: " << N->getOpcode();
      N->dump();
      llvm_unreachable("X86IREmitter::visit - Every X86 visit should be implemented...");
      return NULL;
    }
    case X86ISD::BSF:             return visitBSF(N);
    case X86ISD::BSR:             return visitBSR(N);
    case X86ISD::SHLD:            return visitSHLD(N);
    case X86ISD::SHRD:            return visitSHRD(N);
    case X86ISD::FAND:            return visitFAND(N);
    case X86ISD::FOR:             return visitFOR(N);
    case X86ISD::FXOR:            return visitFXOR(N);
    case X86ISD::FANDN:           return visitFANDN(N);
    case X86ISD::FSRL:            return visitFSRL(N);
    case X86ISD::CALL:            return visitCALL(N);
    case X86ISD::RDTSC_DAG:       return visitRDTSC_DAG(N);
    case X86ISD::CMP:             return visitCMP(N);
    case X86ISD::COMI:            return visitCOMI(N);
    case X86ISD::UCOMI:           return visitUCOMI(N);
    case X86ISD::BT:              return visitBT(N);
    case X86ISD::SETCC:           return visitSETCC(N);
    case X86ISD::SELECT:          return visitSELECT(N);
    case X86ISD::SETCC_CARRY:     return visitSETCC_CARRY(N);
    case X86ISD::FSETCC:          return visitFSETCC(N);
    case X86ISD::FGETSIGNx86:     return visitFGETSIGNx86(N);
    case X86ISD::CMOV:            return visitCMOV(N);
    case X86ISD::BRCOND:          return visitBRCOND(N);
    case X86ISD::RET_FLAG:        return visitRET_FLAG(N);
    case X86ISD::REP_STOS:        return visitREP_STOS(N);
    case X86ISD::REP_MOVS:        return visitREP_MOVS(N);
    case X86ISD::GlobalBaseReg:   return visitGlobalBaseReg(N);
    case X86ISD::Wrapper:         return visitWrapper(N);
    case X86ISD::WrapperRIP:      return visitWrapperRIP(N);
    case X86ISD::MOVDQ2Q:         return visitMOVDQ2Q(N);
    case X86ISD::MMX_MOVD2W:      return visitMMX_MOVD2W(N);
    case X86ISD::PEXTRB:          return visitPEXTRB(N);
    case X86ISD::PEXTRW:          return visitPEXTRW(N);
    case X86ISD::INSERTPS:        return visitINSERTPS(N);
    case X86ISD::PINSRB:          return visitPINSRB(N);
    case X86ISD::PINSRW:          return visitPINSRW(N);
    case X86ISD::MMX_PINSRW:      return visitMMX_PINSRW(N);
    case X86ISD::PSHUFB:          return visitPSHUFB(N);
    case X86ISD::ANDNP:           return visitANDNP(N);
    case X86ISD::PSIGN:           return visitPSIGN(N);
    case X86ISD::BLENDI:          return visitBLENDI(N);
    case X86ISD::SUBUS:           return visitSUBUS(N);
    case X86ISD::HADD:            return visitHADD(N);
    case X86ISD::HSUB:            return visitHSUB(N);
    case X86ISD::FHADD:           return visitFHADD(N);
    case X86ISD::FHSUB:           return visitFHSUB(N);
    case X86ISD::UMAX:            return visitUMAX(N);
    case X86ISD::UMIN:            return visitUMIN(N);
    case X86ISD::SMAX:            return visitSMAX(N);
    case X86ISD::SMIN:            return visitSMIN(N);
    case X86ISD::FMAX:            return visitFMAX(N);
    case X86ISD::FMIN:            return visitFMIN(N);
    case X86ISD::FMAXC:           return visitFMAXC(N);
    case X86ISD::FMINC:           return visitFMINC(N);
    case X86ISD::FRSQRT:          return visitFRSQRT(N);
    case X86ISD::FRCP:            return visitFRCP(N);
    case X86ISD::TLSADDR:         return visitTLSADDR(N);
    case X86ISD::TLSBASEADDR:     return visitTLSBASEADDR(N);
    case X86ISD::TLSCALL:         return visitTLSCALL(N);
    case X86ISD::EH_RETURN:       return visitEH_RETURN(N);
    case X86ISD::EH_SJLJ_SETJMP:  return visitEH_SJLJ_SETJMP(N);
    case X86ISD::EH_SJLJ_LONGJMP: return visitEH_SJLJ_LONGJMP(N);
    case X86ISD::TC_RETURN:       return visitTC_RETURN(N);
    case X86ISD::VZEXT_MOVL:      return visitVZEXT_MOVL(N);
    case X86ISD::VZEXT:           return visitVZEXT(N);
    case X86ISD::VSEXT:           return visitVSEXT(N);
    case X86ISD::VTRUNC:          return visitVTRUNC(N);
    case X86ISD::VTRUNCM:         return visitVTRUNCM(N);
    case X86ISD::VFPEXT:          return visitVFPEXT(N);
    case X86ISD::VFPROUND:        return visitVFPROUND(N);
    case X86ISD::VSHLDQ:          return visitVSHLDQ(N);
    case X86ISD::VSRLDQ:          return visitVSRLDQ(N);
    case X86ISD::VSHL:            return visitVSHL(N);
    case X86ISD::VSRL:            return visitVSRL(N);
    case X86ISD::VSRA:            return visitVSRA(N);
    case X86ISD::VSHLI:           return visitVSHLI(N);
    case X86ISD::VSRLI:           return visitVSRLI(N);
    case X86ISD::VSRAI:           return visitVSRAI(N);
    case X86ISD::CMPP:            return visitCMPP(N);
    case X86ISD::PCMPEQ:          return visitPCMPEQ(N);
    case X86ISD::PCMPGT:          return visitPCMPGT(N);
    case X86ISD::PCMPEQM:         return visitPCMPEQM(N);
    case X86ISD::PCMPGTM:         return visitPCMPGTM(N);
    case X86ISD::CMPM:            return visitCMPM(N);
    case X86ISD::CMPMU:           return visitCMPMU(N);
    case X86ISD::ADD:             return visitADD(N);
    case X86ISD::SUB:             return visitSUB(N);
    case X86ISD::ADC:             return visitADC(N);
    case X86ISD::SBB:             return visitSBB(N);
    case X86ISD::SMUL:            return visitSMUL(N);
    case X86ISD::INC:             return visitINC(N);
    case X86ISD::DEC:             return visitDEC(N);
    case X86ISD::OR:              return visitOR(N);
    case X86ISD::XOR:             return visitXOR(N);
    case X86ISD::AND:             return visitAND(N);
      //case X86ISD::BZHI:            return visitBZHI(N);
    case X86ISD::BEXTR:           return visitBEXTR(N);
    case X86ISD::UMUL:            return visitUMUL(N);
    case X86ISD::MUL_IMM:         return visitMUL_IMM(N);
    case X86ISD::PTEST:           return visitPTEST(N);
    case X86ISD::TESTP:           return visitTESTP(N);
    case X86ISD::TESTM:           return visitTESTM(N);
    case X86ISD::TESTNM:          return visitTESTNM(N);
    case X86ISD::KORTEST:         return visitKORTEST(N);
    case X86ISD::PALIGNR:         return visitPALIGNR(N);
    case X86ISD::PSHUFD:          return visitPSHUFD(N);
    case X86ISD::PSHUFHW:         return visitPSHUFHW(N);
    case X86ISD::PSHUFLW:         return visitPSHUFLW(N);
    case X86ISD::SHUFP:           return visitSHUFP(N);
    case X86ISD::MOVDDUP:         return visitMOVDDUP(N);
    case X86ISD::MOVSHDUP:        return visitMOVSHDUP(N);
    case X86ISD::MOVSLDUP:        return visitMOVSLDUP(N);
    case X86ISD::MOVLHPS:         return visitMOVLHPS(N);
    case X86ISD::MOVLHPD:         return visitMOVLHPD(N);
    case X86ISD::MOVHLPS:         return visitMOVHLPS(N);
    case X86ISD::MOVLPS:          return visitMOVLPS(N);
    case X86ISD::MOVLPD:          return visitMOVLPD(N);
    case X86ISD::MOVSD:           return visitMOVSD(N);
    case X86ISD::MOVSS:           return visitMOVSS(N);
    case X86ISD::UNPCKL:          return visitUNPCKL(N);
    case X86ISD::UNPCKH:          return visitUNPCKH(N);
    case X86ISD::VPERMV:          return visitVPERMV(N);
    case X86ISD::VPERMV3:         return visitVPERMV3(N);
    case X86ISD::VPERMIV3:        return visitVPERMIV3(N);
    case X86ISD::VPERMI:          return visitVPERMI(N);
    case X86ISD::VPERM2X128:      return visitVPERM2X128(N);
    case X86ISD::VBROADCAST:      return visitVBROADCAST(N);
    case X86ISD::VBROADCASTM:     return visitVBROADCASTM(N);
    case X86ISD::VINSERT:         return visitVINSERT(N);
    case X86ISD::VEXTRACT:        return visitVEXTRACT(N);
    case X86ISD::PMULUDQ:         return visitPMULUDQ(N);
    case X86ISD::FMADD:           return visitFMADD(N);
    case X86ISD::FNMADD:          return visitFNMADD(N);
    case X86ISD::FMSUB:           return visitFMSUB(N);
    case X86ISD::FNMSUB:          return visitFNMSUB(N);
    case X86ISD::FMADDSUB:        return visitFMADDSUB(N);
    case X86ISD::FMSUBADD:        return visitFMSUBADD(N);
    case X86ISD::VASTART_SAVE_XMM_REGS: return visitVASTART_SAVE_XMM_REGS(N);
    case X86ISD::WIN_ALLOCA:      return visitWIN_ALLOCA(N);
    case X86ISD::SEG_ALLOCA:      return visitSEG_ALLOCA(N);
    case X86ISD::WIN_FTOL:        return visitWIN_FTOL(N);
    case X86ISD::MEMBARRIER:      return visitMEMBARRIER(N);
    case X86ISD::MFENCE:          return visitMFENCE(N);
    case X86ISD::SFENCE:          return visitSFENCE(N);
    case X86ISD::LFENCE:          return visitLFENCE(N);
    case X86ISD::FNSTSW16r:       return visitFNSTSW16r(N);
    case X86ISD::SAHF:            return visitSAHF(N);
    case X86ISD::RDRAND:          return visitRDRAND(N);
    case X86ISD::RDSEED:          return visitRDSEED(N);
    case X86ISD::PCMPISTRI:       return visitPCMPISTRI(N);
    case X86ISD::PCMPESTRI:       return visitPCMPESTRI(N);
    case X86ISD::XTEST:           return visitXTEST(N);
    // case X86ISD::ATOMADD64_DAG:   return visitATOMADD64_DAG(N);
    // case X86ISD::ATOMSUB64_DAG:   return visitATOMSUB64_DAG(N);
    // case X86ISD::ATOMOR64_DAG:    return visitATOMOR64_DAG(N);
    // case X86ISD::ATOMXOR64_DAG:   return visitATOMXOR64_DAG(N);
    // case X86ISD::ATOMAND64_DAG:   return visitATOMAND64_DAG(N);
    // case X86ISD::ATOMNAND64_DAG:  return visitATOMNAND64_DAG(N);
    // case X86ISD::ATOMMAX64_DAG:   return visitATOMMAX64_DAG(N);
    // case X86ISD::ATOMMIN64_DAG:   return visitATOMMIN64_DAG(N);
    // case X86ISD::ATOMUMAX64_DAG:  return visitATOMUMAX64_DAG(N);
    // case X86ISD::ATOMUMIN64_DAG:  return visitATOMUMIN64_DAG(N);
    // case X86ISD::ATOMSWAP64_DAG:  return visitATOMSWAP64_DAG(N);
    case X86ISD::LCMPXCHG_DAG:    return visitLCMPXCHG_DAG(N);
    case X86ISD::LCMPXCHG8_DAG:   return visitLCMPXCHG8_DAG(N);
    case X86ISD::LCMPXCHG16_DAG:  return visitLCMPXCHG16_DAG(N);
    case X86ISD::VZEXT_LOAD:      return visitVZEXT_LOAD(N);
    case X86ISD::FNSTCW16m:       return visitFNSTCW16m(N);
    case X86ISD::FP_TO_INT64_IN_MEM: return visitFP_TO_INT64_IN_MEM(N);
    case X86ISD::FILD_FLAG:       return visitFILD_FLAG(N);
    case X86ISD::FLD:             return visitFLD(N);
    case X86ISD::FST:             return visitFST(N);
    case X86ISD::VAARG_64:        return visitVAARG_64(N);
  }
}

Value* X86IREmitter::visitBSF(const SDNode *N) { llvm_unreachable("visitBSF Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitBSR(const SDNode *N) { llvm_unreachable("visitBSR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSHLD(const SDNode *N) { llvm_unreachable("visitSHLD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSHRD(const SDNode *N) { llvm_unreachable("visitSHRD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFAND(const SDNode *N) { llvm_unreachable("visitFAND Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFOR(const SDNode *N) { llvm_unreachable("visitFOR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFXOR(const SDNode *N) { llvm_unreachable("visitFXOR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFANDN(const SDNode *N) { llvm_unreachable("visitFANDN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFSRL(const SDNode *N) { llvm_unreachable("visitFSRL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitCALL(const SDNode *N) {
  return IREmitter::visitCALL(N);
}
Value* X86IREmitter::visitRDTSC_DAG(const SDNode *N) { llvm_unreachable("visitRDTSC_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitCMP(const SDNode *N) {
  /*
   * Compares the first source operand with the second source operand and
   * sets the status flags in the EFLAGS register according to the results.
   * The comparison is performed by subtracting the second operand from the
   * first operand and then setting the status flags in the same manner as
   * the SUB instruction. When an immediate value is used as an operand,
   * it is sign-extended to the length of the first operand.
   *
   * The CMP instruction is typically used in conjunction with a conditional
   * jump (Jcc), condition move (CMOVcc), or SETcc instruction. The condition
   * codes used by the Jcc, CMOVcc, and SETcc instructions are based on the
   * results of a CMP instruction. Appendix B, EFLAGS Condition Codes, in
   * the IA-32 Intel Architecture Software Developer's Manual, Volume 1, shows
   * the relationship of the status flags and the condition codes.
   *
   * OPC_EmitNode, TARGET_VAL(X86ISD::CMP), 0,
   *    1 #VTs, MVT::i32, 2 #Ops , 0, 0,  // Results = #3
   */
  return IREmitter::visitSUB(N);
}
Value* X86IREmitter::visitCOMI(const SDNode *N) { llvm_unreachable("visitCOMI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUCOMI(const SDNode *N) { llvm_unreachable("visitUCOMI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitBT(const SDNode *N) { llvm_unreachable("visitBT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSETCC(const SDNode *N) { llvm_unreachable("visitSETCC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSELECT(const SDNode *N) { llvm_unreachable("visitSELECT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSETCC_CARRY(const SDNode *N) { llvm_unreachable("visitSETCC_CARRY Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFSETCC(const SDNode *N) { llvm_unreachable("visitFSETCC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFGETSIGNx86(const SDNode *N) { llvm_unreachable("visitFGETSIGNx86 Unimplemented X86 visit..."); return NULL; }

//TODO: This looks like it will need to be a hybrid version of BRCOND
//http://www.rcollins.org/p6/opcodes/CMOV.html
//fib_O1_llvm_elf_x86 in fastfib
Value* X86IREmitter::visitCMOV(const SDNode *N) {
  //llvm_unreachable("visitCMOV Unimplemented X86 visit...");
  return NULL;
}
//visitBRCOND - Basic handles most conditional branches.  The ideas is that it will search
//  for the CopyToRegs and look for the compare (CMP).  In situations where there isn't a
//  compare, visitBRCONDAdvanced will step in.
Value* X86IREmitter::visitBRCOND(const SDNode *N) {
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
  /*
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
   */
  SDNode *CMPNode = NULL;
  SDValue Iter = N->getOperand(N->getNumOperands()-1);
  while(Iter.getOpcode() != ISD::EntryToken){
    Iter.dump();
    if(Iter.getOpcode() == ISD::CopyToReg && Iter.getNumOperands() == 3){   //Get nearest CopyToReg
      if(Iter.getOperand(2).getNode()->getOpcode() == X86ISD::CMP){
        CMPNode = Iter.getOperand(2).getNode();
        break;
      }
    }
    Iter = Iter.getOperand(0);
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

//visitBRCONDAdvanced handles optimized conditional branches.  Often optimized
//  conditional branches do not have compare statements - instead the use
//  mathematical operations to set the EFLAGS register.  Once EFLAGS are set the
//  condition can be based on specific field(s).  The two EFLAGS fields that are
//  of specific interst are CF (Carry Flag) and ZF (Zero Flag).
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
  SDValue Iter = N->getOperand(N->getNumOperands()-1);
  while(Iter.getOpcode() != ISD::EntryToken){              //Get nearest CopyToReg || CopyFromReg
    Iter.dump();
    if(Iter.getOpcode() == ISD::CopyToReg || Iter.getOpcode() == ISD::CopyFromReg){
      CMPNode = dyn_cast<RegisterSDNode>(Iter.getOperand(1).getNode()); //Change from EFLAGS to the output to ESI
      if(CMPNode->getReg() == X86::EFLAGS && Iter.getOpcode() == ISD::CopyToReg && Iter.getNumOperands() == 3){           //Verify that we find the first EFLAGS Register
        BinOpNode = Iter.getOperand(2).getNode();    //Get the math operation {add, mult, etc...}
        break;
      }
      Iter = Iter.getOperand(0);
    } else {
      Iter = Iter.getOperand(Iter.getNumOperands()-1);
    }
  }

  if(BinOpNode == NULL || CMPNode == NULL || CMPNode->getReg() != X86::EFLAGS){  //Ensure we didn't just fall through the while loop
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced: Could not find EFLAGS Register or the Math Node...");
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

  Value *Cmp = NULL;
  Value *Vis = visit(BinOpNode);
  if(Vis == NULL){
    errs() << "Compare Node:\n";
    CMPNode->dump();
    errs() << "BinOp Node:\n";
    BinOpNode->dump();
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced - RHS is NULL - is the IREmitter returning NULL somewhere?");
  }
  //Get the current context - this includes the operation that populated EFLAGS
  //    That operation is the key component so that EFLAGS can be abstracted to
  //    native LLVM IR.
  LLVMContext *CurrentContext = Dec->getDisassembler()->getMCDirector()->getContext();
  //We need to convert the context into an Integer.  That integer will be the
  //    used for the conditional statement.  The entire point of this operation
  //    is to abstract EFLAGS as a register, which means we really don't care about
  //    the specific location of the flag, the operation that is setting it.
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
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced: SETGE Unimplemented");
    break;
  case ISD::SETLT:  //JB_1: CF == 1
    // CF ISD::AND 1 (b1) == 1
    //Cmp = IRB->CreateICmpSLT(LHS, RHS);
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced: SETLT Unimplemented");
    break;
  case ISD::SETGT:  //JA_1: CF == 0 && ZF == 0
    // ZF ISD::AND 32 (b100000) == 0 && CF ISD::AND 1 (b1) == 0
    //Cmp = IRB->CreateICmpSGT(LHS, RHS);
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced: SETGT Unimplemented");
    break;
  case ISD::SETLE:  //JBE_1: CF == 1 || ZF == 1
    // ZF ISD::AND 32 (b100000) == 1 && CF ISD::AND 1 (b1) == 1
    //Cmp = IRB->CreateICmpSLE(LHS, RHS);
    llvm_unreachable("X86IREmitter::visitBRCONDAdvanced: SETLE Unimplemented");
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
Value* X86IREmitter::visitRET_FLAG(const SDNode *N){
  return IRB->CreateRetVoid();
}

Value* X86IREmitter::visitREP_STOS(const SDNode *N) { llvm_unreachable("visitREP_STOS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitREP_MOVS(const SDNode *N) { llvm_unreachable("visitREP_MOVS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitGlobalBaseReg(const SDNode *N) { llvm_unreachable("visitGlobalBaseReg Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitWrapper(const SDNode *N) { llvm_unreachable("visitWrapper Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitWrapperRIP(const SDNode *N) { llvm_unreachable("visitWrapperRIP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVDQ2Q(const SDNode *N) { llvm_unreachable("visitMOVDQ2Q Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMMX_MOVD2W(const SDNode *N) { llvm_unreachable("visitMMX_MOVD2W Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPEXTRB(const SDNode *N) { llvm_unreachable("visitPEXTRB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPEXTRW(const SDNode *N) { llvm_unreachable("visitPEXTRW Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitINSERTPS(const SDNode *N) { llvm_unreachable("visitINSERTPS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPINSRB(const SDNode *N) { llvm_unreachable("visitPINSRB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPINSRW(const SDNode *N) { llvm_unreachable("visitPINSRW Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMMX_PINSRW(const SDNode *N) { llvm_unreachable("visitMMX_PINSRW Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPSHUFB(const SDNode *N) { llvm_unreachable("visitPSHUFB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitANDNP(const SDNode *N) { llvm_unreachable("visitANDNP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPSIGN(const SDNode *N) { llvm_unreachable("visitPSIGN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitBLENDV(const SDNode *N) { llvm_unreachable("visitBLENDV Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitBLENDI(const SDNode *N) { llvm_unreachable("visitBLENDI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSUBUS(const SDNode *N) { llvm_unreachable("visitSUBUS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitHADD(const SDNode *N) { llvm_unreachable("visitHADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitHSUB(const SDNode *N) { llvm_unreachable("visitHADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFHADD(const SDNode *N) { llvm_unreachable("visitFHADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFHSUB(const SDNode *N) { llvm_unreachable("visitFHSUB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUMAX(const SDNode *N) { llvm_unreachable("visitUMAX Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUMIN(const SDNode *N) { llvm_unreachable("visitUMIN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSMAX(const SDNode *N) { llvm_unreachable("visitSMAX Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSMIN(const SDNode *N) { llvm_unreachable("visitSMIN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMAX(const SDNode *N) { llvm_unreachable("visitFMAX Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMIN(const SDNode *N) { llvm_unreachable("visitFMIN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMAXC(const SDNode *N) { llvm_unreachable("visitFMAXC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMINC(const SDNode *N) { llvm_unreachable("visitFMINC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFRSQRT(const SDNode *N) { llvm_unreachable("visitFRSQRT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFRCP(const SDNode *N) { llvm_unreachable("visitFRCP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTLSADDR(const SDNode *N) { llvm_unreachable("visitTLSADDR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTLSBASEADDR(const SDNode *N) { llvm_unreachable("visitTLSBASEADDR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTLSCALL(const SDNode *N) { llvm_unreachable("visitTLSBASEADDR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitEH_RETURN(const SDNode *N) { llvm_unreachable("visitEH_RETURN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitEH_SJLJ_SETJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_SETJMP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitEH_SJLJ_LONGJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_LONGJMP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTC_RETURN(const SDNode *N) { llvm_unreachable("visitTC_RETURN Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVZEXT_MOVL(const SDNode *N) { llvm_unreachable("visitVZEXT_MOVL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVZEXT(const SDNode *N) { llvm_unreachable("visitVZEXT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSEXT(const SDNode *N) { llvm_unreachable("visitVSEXT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVTRUNC(const SDNode *N) { llvm_unreachable("visitVTRUNC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVTRUNCM(const SDNode *N) { llvm_unreachable("visitVTRUNCM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVFPEXT(const SDNode *N) { llvm_unreachable("visitVFPEXT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVFPROUND(const SDNode *N) { llvm_unreachable("visitVFPROUND Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSHLDQ(const SDNode *N) { llvm_unreachable("visitVSHLDQ Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSRLDQ(const SDNode *N) { llvm_unreachable("visitVSRLDQ Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSHL(const SDNode *N) { llvm_unreachable("visitVSHL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSRL(const SDNode *N) { llvm_unreachable("visitVSRL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSRA(const SDNode *N) { llvm_unreachable("visitVSRA Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSHLI(const SDNode *N) { llvm_unreachable("visitVSHLI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSRLI(const SDNode *N) { llvm_unreachable("visitVSRLI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVSRAI(const SDNode *N) { llvm_unreachable("visitVSRAI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitCMPP(const SDNode *N) { llvm_unreachable("visitCMPP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPEQ(const SDNode *N) { llvm_unreachable("visitPCMPEQ Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPGT(const SDNode *N) { llvm_unreachable("visitPCMPGT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPEQM(const SDNode *N) { llvm_unreachable("visitPCMPEQM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPGTM(const SDNode *N) { llvm_unreachable("visitPCMPGTM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitCMPM(const SDNode *N) { llvm_unreachable("visitCMPM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitCMPMU(const SDNode *N) { llvm_unreachable("visitCMPMU Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitADD(const SDNode *N) {
  return IREmitter::visitADD(N);
}
Value* X86IREmitter::visitSUB(const SDNode *N) {
  return IREmitter::visitSUB(N);
}
Value* X86IREmitter::visitADC(const SDNode *N) { llvm_unreachable("visitADC Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSBB(const SDNode *N) { llvm_unreachable("visitSBB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSMUL(const SDNode *N) { llvm_unreachable("visitSMUL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitINC(const SDNode *N) {
  //Looks like there are two valid operands based on inc def:
  //2/*#VTs*/, MVT::i16, MVT::i32, 1/*#Ops*/, 0,  // Results = #1 #2

  // Operand 0 and 1 are values to add
  Value *Op0 = visit(N->getOperand(0).getNode());

  Constant *Initializer = Constant::getAllOnesValue(Op0->getType());
  Value *Op1 = Initializer->stripInBoundsConstantOffsets();

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
//in fib_O1_llvm_elf_x86
Value* X86IREmitter::visitDEC(const SDNode *N) {
  //Looks like there are two valid operands based on dec def:
  //2/*#VTs*/, MVT::i16, MVT::i32, 1/*#Ops*/, 0,  // Results = #1 #2

  // Operand 0 and 1 are values to add
  Value *Op0 = visit(N->getOperand(0).getNode());
  Constant *Initializer = Constant::getAllOnesValue(Op0->getType());
  Value *Op1 = Initializer->stripInBoundsConstantOffsets();

  StringRef BaseName = getInstructionName(N);
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op0->getName());
  }
  if (BaseName.empty()) {
    BaseName = getBaseValueName(Op1->getName());
  }
  StringRef Name = getIndexedValueName(BaseName);
  Instruction *Res = dyn_cast<Instruction>(IRB->CreateSub(Op0, Op1, Name));
  Res->setDebugLoc(N->getDebugLoc());
  VisitMap[N] = Res;
  return Res;
}
Value* X86IREmitter::visitOR(const SDNode *N) { llvm_unreachable("visitOR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitXOR(const SDNode *N) {
  return IREmitter::visitXOR(N);
}
Value* X86IREmitter::visitAND(const SDNode *N) {
  return IREmitter::visitAND(N);
}
Value* X86IREmitter::visitBZHI(const SDNode *N) { llvm_unreachable("visitBZHI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitBEXTR(const SDNode *N) { llvm_unreachable("visitBEXTR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUMUL(const SDNode *N) { llvm_unreachable("visitUMUL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMUL_IMM(const SDNode *N) { llvm_unreachable("visitMUL_IMM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPTEST(const SDNode *N) { llvm_unreachable("visitPTEST Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTESTP(const SDNode *N) { llvm_unreachable("visitTESTP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTESTM(const SDNode *N) { llvm_unreachable("visitTESTM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitTESTNM(const SDNode *N) { llvm_unreachable("visitTESTNM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitKORTEST(const SDNode *N) { llvm_unreachable("visitKORTEST Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPALIGNR(const SDNode *N) { llvm_unreachable("visitPALIGNR Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPSHUFD(const SDNode *N) { llvm_unreachable("visitPSHUFD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPSHUFHW(const SDNode *N) { llvm_unreachable("visitPSHUFHW Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPSHUFLW(const SDNode *N) { llvm_unreachable("visitPSHUFLW Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSHUFP(const SDNode *N) { llvm_unreachable("visitSHUFP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVDDUP(const SDNode *N) { llvm_unreachable("visitMOVDDUP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVSHDUP(const SDNode *N) { llvm_unreachable("visitMOVSHDUP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVSLDUP(const SDNode *N) { llvm_unreachable("visitMOVSLDUP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVLHPS(const SDNode *N) { llvm_unreachable("visitMOVLHPS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVLHPD(const SDNode *N) { llvm_unreachable("visitMOVLHPD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVHLPS(const SDNode *N) { llvm_unreachable("visitMOVHLPS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVLPS(const SDNode *N) { llvm_unreachable("visitMOVLPS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVLPD(const SDNode *N) { llvm_unreachable("visitMOVLPD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVSD(const SDNode *N) { llvm_unreachable("visitMOVSD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMOVSS(const SDNode *N) { llvm_unreachable("visitMOVSS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUNPCKL(const SDNode *N) { llvm_unreachable("visitUNPCKL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitUNPCKH(const SDNode *N) { llvm_unreachable("visitUNPCKH Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERMILP(const SDNode *N) { llvm_unreachable("visitVPERMILP Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERMV(const SDNode *N) { llvm_unreachable("visitVPERMV Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERMV3(const SDNode *N) { llvm_unreachable("visitVPERMV3 Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERMIV3(const SDNode *N) { llvm_unreachable("visitVPERMIV3 Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERMI(const SDNode *N) { llvm_unreachable("visitVPERMI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVPERM2X128(const SDNode *N) { llvm_unreachable("visitVPERM2X128 Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVBROADCAST(const SDNode *N) { llvm_unreachable("visitVBROADCAST Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVBROADCASTM(const SDNode *N) { llvm_unreachable("visitVBROADCASTM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVINSERT(const SDNode *N) { llvm_unreachable("visitVINSERT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVEXTRACT(const SDNode *N) { llvm_unreachable("visitVEXTRACT Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPMULUDQ(const SDNode *N) { llvm_unreachable("visitPMULUDQ Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMADD(const SDNode *N) { llvm_unreachable("visitFMADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFNMADD(const SDNode *N) { llvm_unreachable("visitFNMADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMSUB(const SDNode *N) { llvm_unreachable("visitFMSUB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFNMSUB(const SDNode *N) { llvm_unreachable("visitFNMSUB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMADDSUB(const SDNode *N) { llvm_unreachable("visitFMADDSUB Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFMSUBADD(const SDNode *N) { llvm_unreachable("visitFMSUBADD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVASTART_SAVE_XMM_REGS(const SDNode *N) { llvm_unreachable("visitVASTART_SAVE_XMM_REGS Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitWIN_ALLOCA(const SDNode *N) { llvm_unreachable("visitWIN_ALLOCA Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSEG_ALLOCA(const SDNode *N) { llvm_unreachable("visitSEG_ALLOCA Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitWIN_FTOL(const SDNode *N) { llvm_unreachable("visitWIN_FTOL Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMEMBARRIER(const SDNode *N) { llvm_unreachable("visitMEMBARRIER Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitMFENCE(const SDNode *N) { llvm_unreachable("visitMFENCE Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSFENCE(const SDNode *N) { llvm_unreachable("visitSFENCE Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitLFENCE(const SDNode *N) { llvm_unreachable("visitLFENCE Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFNSTSW16r(const SDNode *N) { llvm_unreachable("visitFNSTSW16r Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitSAHF(const SDNode *N) { llvm_unreachable("visitSAHF Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitRDRAND(const SDNode *N) { llvm_unreachable("visitRDRAND Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitRDSEED(const SDNode *N) { llvm_unreachable("visitRDSEED Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPISTRI(const SDNode *N) { llvm_unreachable("visitPCMPISTRI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitPCMPESTRI(const SDNode *N) { llvm_unreachable("visitPCMPESTRI Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitXTEST(const SDNode *N) { llvm_unreachable("visitXTEST Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMADD64_DAG(const SDNode *N) { llvm_unreachable("visitATOMADD64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMSUB64_DAG(const SDNode *N) { llvm_unreachable("visitATOMSUB64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMOR64_DAG(const SDNode *N) { llvm_unreachable("visitATOMOR64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMXOR64_DAG(const SDNode *N) { llvm_unreachable("visitATOMXOR64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMAND64_DAG(const SDNode *N) { llvm_unreachable("visitATOMAND64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMNAND64_DAG(const SDNode *N) { llvm_unreachable("visitATOMNAND64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMMAX64_DAG(const SDNode *N) { llvm_unreachable("visitATOMMAX64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMMIN64_DAG(const SDNode *N) { llvm_unreachable("visitATOMMIN64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMUMAX64_DAG(const SDNode *N) { llvm_unreachable("visitATOMUMAX64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMUMIN64_DAG(const SDNode *N) { llvm_unreachable("visitATOMUMIN64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitATOMSWAP64_DAG(const SDNode *N) { llvm_unreachable("visitATOMSWAP64_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitLCMPXCHG_DAG(const SDNode *N) { llvm_unreachable("visitLCMPXCHG_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitLCMPXCHG8_DAG(const SDNode *N) { llvm_unreachable("visitLCMPXCHG8_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitLCMPXCHG16_DAG(const SDNode *N) { llvm_unreachable("visitLCMPXCHG16_DAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVZEXT_LOAD(const SDNode *N) { llvm_unreachable("visitVZEXT_LOAD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFNSTCW16m(const SDNode *N) { llvm_unreachable("visitFNSTCW16m Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFP_TO_INT64_IN_MEM(const SDNode *N) { llvm_unreachable("visitFP_TO_INT64_IN_MEM Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFILD_FLAG(const SDNode *N) { llvm_unreachable("visitFILD_FLAG Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFLD(const SDNode *N) { llvm_unreachable("visitFLD Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitFST(const SDNode *N) { llvm_unreachable("visitFST Unimplemented X86 visit..."); return NULL; }
Value* X86IREmitter::visitVAARG_64(const SDNode *N) { llvm_unreachable("visitVAARG_64 Unimplemented X86 visit..."); return NULL; }

} // end fracture namespace
