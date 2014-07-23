//===- X86IREmitter.h - Generalize X86ISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for X86ISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef X86IREMITTER_H
#define X86IREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "X86ISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class X86IREmitter : public IREmitter {
public:
  X86IREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~X86IREmitter();
private:
  virtual Value* visit(const SDNode *N);

  Value* visitBSF(const SDNode *N);
  Value* visitBSR(const SDNode *N);
  Value* visitSHLD(const SDNode *N);
  Value* visitSHRD(const SDNode *N);
  Value* visitFAND(const SDNode *N);
  Value* visitFOR(const SDNode *N);
  Value* visitFXOR(const SDNode *N);
  Value* visitFANDN(const SDNode *N);
  Value* visitFSRL(const SDNode *N);
  Value* visitCALL(const SDNode *N);
  Value* visitRDTSC_DAG(const SDNode *N);
  Value* visitCMP(const SDNode *N);
  Value* visitCOMI(const SDNode *N);
  Value* visitUCOMI(const SDNode *N);
  Value* visitBT(const SDNode *N);
  Value* visitSETCC(const SDNode *N);
  Value* visitSELECT(const SDNode *N);
  Value* visitSETCC_CARRY(const SDNode *N);
  Value* visitFSETCC(const SDNode *N);
  Value* visitFGETSIGNx86(const SDNode *N);
  Value* visitCMOV(const SDNode *N);
  Value* visitBRCOND(const SDNode *N);
  Value* visitBRCONDAdvanced(const SDNode *N);
  Value* visitRET_FLAG(const SDNode *N);
  Value* visitREP_STOS(const SDNode *N);
  Value* visitREP_MOVS(const SDNode *N);
  Value* visitGlobalBaseReg(const SDNode *N);
  Value* visitWrapper(const SDNode *N);
  Value* visitWrapperRIP(const SDNode *N);
  Value* visitMOVDQ2Q(const SDNode *N);
  Value* visitMMX_MOVD2W(const SDNode *N);
  Value* visitPEXTRB(const SDNode *N);
  Value* visitPEXTRW(const SDNode *N);
  Value* visitINSERTPS(const SDNode *N);
  Value* visitPINSRB(const SDNode *N);
  Value* visitPINSRW(const SDNode *N);
  Value* visitMMX_PINSRW(const SDNode *N);
  Value* visitPSHUFB(const SDNode *N);
  Value* visitANDNP(const SDNode *N);
  Value* visitPSIGN(const SDNode *N);
  Value* visitBLENDV(const SDNode *N);
  Value* visitBLENDI(const SDNode *N);
  Value* visitSUBUS(const SDNode *N);
  Value* visitHADD(const SDNode *N);
  Value* visitHSUB(const SDNode *N);
  Value* visitFHADD(const SDNode *N);
  Value* visitFHSUB(const SDNode *N);
  Value* visitUMAX(const SDNode *N);
  Value* visitUMIN(const SDNode *N);
  Value* visitSMAX(const SDNode *N);
  Value* visitSMIN(const SDNode *N);
  Value* visitFMAX(const SDNode *N);
  Value* visitFMIN(const SDNode *N);
  Value* visitFMAXC(const SDNode *N);
  Value* visitFMINC(const SDNode *N);
  Value* visitFRSQRT(const SDNode *N);
  Value* visitFRCP(const SDNode *N);
  Value* visitTLSADDR(const SDNode *N);
  Value* visitTLSBASEADDR(const SDNode *N);
  Value* visitTLSCALL(const SDNode *N);
  Value* visitEH_RETURN(const SDNode *N);
  Value* visitEH_SJLJ_SETJMP(const SDNode *N);
  Value* visitEH_SJLJ_LONGJMP(const SDNode *N);
  Value* visitTC_RETURN(const SDNode *N);
  Value* visitVZEXT_MOVL(const SDNode *N);
  Value* visitVZEXT(const SDNode *N);
  Value* visitVSEXT(const SDNode *N);
  Value* visitVTRUNC(const SDNode *N);
  Value* visitVTRUNCM(const SDNode *N);
  Value* visitVFPEXT(const SDNode *N);
  Value* visitVFPROUND(const SDNode *N);
  Value* visitVSHLDQ(const SDNode *N);
  Value* visitVSRLDQ(const SDNode *N);
  Value* visitVSHL(const SDNode *N);
  Value* visitVSRL(const SDNode *N);
  Value* visitVSRA(const SDNode *N);
  Value* visitVSHLI(const SDNode *N);
  Value* visitVSRLI(const SDNode *N);
  Value* visitVSRAI(const SDNode *N);
  Value* visitCMPP(const SDNode *N);
  Value* visitPCMPEQ(const SDNode *N);
  Value* visitPCMPGT(const SDNode *N);
  Value* visitPCMPEQM(const SDNode *N);
  Value* visitPCMPGTM(const SDNode *N);
  Value* visitCMPM(const SDNode *N);
  Value* visitCMPMU(const SDNode *N);
  Value* visitADD(const SDNode *N);
  Value* visitSUB(const SDNode *N);
  Value* visitADC(const SDNode *N);
  Value* visitSBB(const SDNode *N);
  Value* visitSMUL(const SDNode *N);
  Value* visitINC(const SDNode *N);
  Value* visitDEC(const SDNode *N);
  Value* visitOR(const SDNode *N);
  Value* visitXOR(const SDNode *N);
  Value* visitAND(const SDNode *N);
  Value* visitBZHI(const SDNode *N);
  Value* visitBEXTR(const SDNode *N);
  Value* visitUMUL(const SDNode *N);
  Value* visitMUL_IMM(const SDNode *N);
  Value* visitPTEST(const SDNode *N);
  Value* visitTESTP(const SDNode *N);
  Value* visitTESTM(const SDNode *N);
  Value* visitTESTNM(const SDNode *N);
  Value* visitKORTEST(const SDNode *N);
  Value* visitPALIGNR(const SDNode *N);
  Value* visitPSHUFD(const SDNode *N);
  Value* visitPSHUFHW(const SDNode *N);
  Value* visitPSHUFLW(const SDNode *N);
  Value* visitSHUFP(const SDNode *N);
  Value* visitMOVDDUP(const SDNode *N);
  Value* visitMOVSHDUP(const SDNode *N);
  Value* visitMOVSLDUP(const SDNode *N);
  Value* visitMOVLHPS(const SDNode *N);
  Value* visitMOVLHPD(const SDNode *N);
  Value* visitMOVHLPS(const SDNode *N);
  Value* visitMOVLPS(const SDNode *N);
  Value* visitMOVLPD(const SDNode *N);
  Value* visitMOVSD(const SDNode *N);
  Value* visitMOVSS(const SDNode *N);
  Value* visitUNPCKL(const SDNode *N);
  Value* visitUNPCKH(const SDNode *N);
  Value* visitVPERMILP(const SDNode *N);
  Value* visitVPERMV(const SDNode *N);
  Value* visitVPERMV3(const SDNode *N);
  Value* visitVPERMIV3(const SDNode *N);
  Value* visitVPERMI(const SDNode *N);
  Value* visitVPERM2X128(const SDNode *N);
  Value* visitVBROADCAST(const SDNode *N);
  Value* visitVBROADCASTM(const SDNode *N);
  Value* visitVINSERT(const SDNode *N);
  Value* visitVEXTRACT(const SDNode *N);
  Value* visitPMULUDQ(const SDNode *N);
  Value* visitFMADD(const SDNode *N);
  Value* visitFNMADD(const SDNode *N);
  Value* visitFMSUB(const SDNode *N);
  Value* visitFNMSUB(const SDNode *N);
  Value* visitFMADDSUB(const SDNode *N);
  Value* visitFMSUBADD(const SDNode *N);
  Value* visitVASTART_SAVE_XMM_REGS(const SDNode *N);
  Value* visitWIN_ALLOCA(const SDNode *N);
  Value* visitSEG_ALLOCA(const SDNode *N);
  Value* visitWIN_FTOL(const SDNode *N);
  Value* visitMEMBARRIER(const SDNode *N);
  Value* visitMFENCE(const SDNode *N);
  Value* visitSFENCE(const SDNode *N);
  Value* visitLFENCE(const SDNode *N);
  Value* visitFNSTSW16r(const SDNode *N);
  Value* visitSAHF(const SDNode *N);
  Value* visitRDRAND(const SDNode *N);
  Value* visitRDSEED(const SDNode *N);
  Value* visitPCMPISTRI(const SDNode *N);
  Value* visitPCMPESTRI(const SDNode *N);
  Value* visitXTEST(const SDNode *N);
  Value* visitATOMADD64_DAG(const SDNode *N);
  Value* visitATOMSUB64_DAG(const SDNode *N);
  Value* visitATOMOR64_DAG(const SDNode *N);
  Value* visitATOMXOR64_DAG(const SDNode *N);
  Value* visitATOMAND64_DAG(const SDNode *N);
  Value* visitATOMNAND64_DAG(const SDNode *N);
  Value* visitATOMMAX64_DAG(const SDNode *N);
  Value* visitATOMMIN64_DAG(const SDNode *N);
  Value* visitATOMUMAX64_DAG(const SDNode *N);
  Value* visitATOMUMIN64_DAG(const SDNode *N);
  Value* visitATOMSWAP64_DAG(const SDNode *N);
  Value* visitLCMPXCHG_DAG(const SDNode *N);
  Value* visitLCMPXCHG8_DAG(const SDNode *N);
  Value* visitLCMPXCHG16_DAG(const SDNode *N);
  Value* visitVZEXT_LOAD(const SDNode *N);
  Value* visitFNSTCW16m(const SDNode *N);
  Value* visitFP_TO_INT64_IN_MEM(const SDNode *N);
  Value* visitFILD_FLAG(const SDNode *N);
  Value* visitFLD(const SDNode *N);
  Value* visitFST(const SDNode *N);
  Value* visitVAARG_64(const SDNode *N);
};

} // end fracture namespace

#endif
