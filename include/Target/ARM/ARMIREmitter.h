//===- ARMIREmitter.h - Generalize ARMISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for ARMISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef ARMIREMITTER_H
#define ARMIREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "ARMISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class ARMIREmitter : public IREmitter {
public:
  ARMIREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~ARMIREmitter();
private:
  virtual Value* visit(const SDNode *N);
  Value* visitWrapper(const SDNode *N);
  Value* visitWrapperPIC(const SDNode *N);
  Value* visitWrapperJT(const SDNode *N);
  Value* visitCOPY_STRUCT_BYVAL(const SDNode *N);
  Value* visitCALL(const SDNode *N);
  Value* visitCALL_PRED(const SDNode *N);
  Value* visitCALL_NOLINK(const SDNode *N);
  Value* visittCALL(const SDNode *N);
  Value* visitBRCOND(const SDNode *N);
  Value* visitBR_JT(const SDNode *N);
  Value* visitBR2_JT(const SDNode *N);
  Value* visitRET_FLAG(const SDNode *N);
  Value* visitINTRET_FLAG(const SDNode *N);
  Value* visitPIC_ADD(const SDNode *N);
  Value* visitCMP(const SDNode *N);
  Value* visitCMN(const SDNode *N);
  Value* visitCMPZ(const SDNode *N);
  Value* visitCMPFP(const SDNode *N);
  Value* visitCMPFPw0(const SDNode *N);
  Value* visitFMSTAT(const SDNode *N);
  Value* visitCMOV(const SDNode *N);
  Value* visitBCC_i64(const SDNode *N);
  Value* visitRBIT(const SDNode *N);
  Value* visitFTOSI(const SDNode *N);
  Value* visitFTOUI(const SDNode *N);
  Value* visitSITOF(const SDNode *N);
  Value* visitUITOF(const SDNode *N);
  Value* visitSRL_FLAG(const SDNode *N);
  Value* visitSRA_FLAG(const SDNode *N);
  Value* visitRRX(const SDNode *N);
  Value* visitADDC(const SDNode *N);
  Value* visitADDE(const SDNode *N);
  Value* visitSUBC(const SDNode *N);
  Value* visitSUBE(const SDNode *N);
  Value* visitVMOVRRD(const SDNode *N);
  Value* visitVMOVDRR(const SDNode *N);
  Value* visitEH_SJLJ_SETJMP(const SDNode *N);
  Value* visitEH_SJLJ_LONGJMP(const SDNode *N);
  Value* visitTC_RETURN(const SDNode *N);
  Value* visitTHREAD_POINTER(const SDNode *N);
  Value* visitDYN_ALLOC(const SDNode *N);
  Value* visitMEMBARRIER_MCR(const SDNode *N);
  Value* visitPRELOAD(const SDNode *N);
  Value* visitVCEQ(const SDNode *N);
  Value* visitVCEQZ(const SDNode *N);
  Value* visitVCGE(const SDNode *N);
  Value* visitVCGEZ(const SDNode *N);
  Value* visitVCLEZ(const SDNode *N);
  Value* visitVCGEU(const SDNode *N);
  Value* visitVCGT(const SDNode *N);
  Value* visitVCGTZ(const SDNode *N);
  Value* visitVCLTZ(const SDNode *N);
  Value* visitVCGTU(const SDNode *N);
  Value* visitVTST(const SDNode *N);
  Value* visitVSHL(const SDNode *N);
  Value* visitVSHRs(const SDNode *N);
  Value* visitVSHRu(const SDNode *N);
  Value* visitVSHLLs(const SDNode *N);
  Value* visitVSHLLu(const SDNode *N);
  Value* visitVSHLLi(const SDNode *N);
  Value* visitVSHRN(const SDNode *N);
  Value* visitVRSHRs(const SDNode *N);
  Value* visitVRSHRu(const SDNode *N);
  Value* visitVRSHRN(const SDNode *N);
  Value* visitVQSHLs(const SDNode *N);
  Value* visitVQSHLu(const SDNode *N);
  Value* visitVQSHLsu(const SDNode *N);
  Value* visitVQSHRNs(const SDNode *N);
  Value* visitVQSHRNu(const SDNode *N);
  Value* visitVQSHRNsu(const SDNode *N);
  Value* visitVQRSHRNs(const SDNode *N);
  Value* visitVQRSHRNu(const SDNode *N);
  Value* visitVQRSHRNsu(const SDNode *N);
  Value* visitVSLI(const SDNode *N);
  Value* visitVSRI(const SDNode *N);
  Value* visitVGETLANEu(const SDNode *N);
  Value* visitVGETLANEs(const SDNode *N);
  Value* visitVMOVIMM(const SDNode *N);
  Value* visitVMVNIMM(const SDNode *N);
  Value* visitVMOVFPIMM(const SDNode *N);
  Value* visitVDUP(const SDNode *N);
  Value* visitVDUPLANE(const SDNode *N);
  Value* visitVEXT(const SDNode *N);
  Value* visitVREV64(const SDNode *N);
  Value* visitVREV32(const SDNode *N);
  Value* visitVREV16(const SDNode *N);
  Value* visitVZIP(const SDNode *N);
  Value* visitVUZP(const SDNode *N);
  Value* visitVTRN(const SDNode *N);
  Value* visitVTBL1(const SDNode *N);
  Value* visitVTBL2(const SDNode *N);
  Value* visitVMULLs(const SDNode *N);
  Value* visitVMULLu(const SDNode *N);
  Value* visitUMLAL(const SDNode *N);
  Value* visitSMLAL(const SDNode *N);
  Value* visitBUILD_VECTOR(const SDNode *N);
  Value* visitFMAX(const SDNode *N);
  Value* visitFMIN(const SDNode *N);
  Value* visitVMAXNM(const SDNode *N);
  Value* visitVMINNM(const SDNode *N);
  Value* visitBFI(const SDNode *N);
  Value* visitVORRIMM(const SDNode *N);
  Value* visitVBICIMM(const SDNode *N);
  Value* visitVBSL(const SDNode *N);
  Value* visitVLD2DUP(const SDNode *N);
  Value* visitVLD3DUP(const SDNode *N);
  Value* visitVLD4DUP(const SDNode *N);
  Value* visitVLD1_UPD(const SDNode *N);
  Value* visitVLD2_UPD(const SDNode *N);
  Value* visitVLD3_UPD(const SDNode *N);
  Value* visitVLD4_UPD(const SDNode *N);
  Value* visitVLD2LN_UPD(const SDNode *N);
  Value* visitVLD3LN_UPD(const SDNode *N);
  Value* visitVLD4LN_UPD(const SDNode *N);
  Value* visitVLD2DUP_UPD(const SDNode *N);
  Value* visitVLD3DUP_UPD(const SDNode *N);
  Value* visitVLD4DUP_UPD(const SDNode *N);
  Value* visitVST1_UPD(const SDNode *N);
  Value* visitVST2_UPD(const SDNode *N);
  Value* visitVST3_UPD(const SDNode *N);
  Value* visitVST4_UPD(const SDNode *N);
  Value* visitVST2LN_UPD(const SDNode *N);
  Value* visitVST3LN_UPD(const SDNode *N);
  Value* visitVST4LN_UPD(const SDNode *N);
};

} // end fracture namespace

#endif
