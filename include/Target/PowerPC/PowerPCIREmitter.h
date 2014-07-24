//===- PowerPCIREmitter.h - Generalize PowerPCISD Instrs  ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Implements visitors for PowerPCISD SDNodes.
//
//===----------------------------------------------------------------------===//

#ifndef POWERPCIREMITTER_H
#define POWERPCIREMITTER_H

#include "llvm/CodeGen/ISDOpcodes.h"
#include "PPCISD.h"

#include "CodeInv/IREmitter.h"

namespace fracture {

class Decompiler;

class PowerPCIREmitter : public IREmitter {
public:
  PowerPCIREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~PowerPCIREmitter();
private:
  virtual Value* visit(const SDNode *N);

  Value* visitFSEL(const SDNode *N);
  Value* visitFCFID(const SDNode *N);
  Value* visitFCFIDU(const SDNode *N);
  Value* visitFCFIDS(const SDNode *N);
  Value* visitFCFIDUS(const SDNode *N);
  Value* visitFCTIDZ(const SDNode *N);
  Value* visitFCTIWZ(const SDNode *N);
  Value* visitFCTIDUZ(const SDNode *N);
  Value* visitFCTIWUZ(const SDNode *N);
  Value* visitFRE(const SDNode *N);
  Value* visitFRSQRTE(const SDNode *N);
  Value* visitVMADDFP(const SDNode *N);
  Value* visitVNMSUBFP(const SDNode *N);
  Value* visitVPERM(const SDNode *N);
  Value* visitHi(const SDNode *N);
  Value* visitLo(const SDNode *N);
  Value* visitTOC_ENTRY(const SDNode *N);
  Value* visitTOC_RESTORE(const SDNode *N);
  Value* visitLOAD(const SDNode *N);
  Value* visitLOAD_TOC(const SDNode *N);
  Value* visitDYNALLOC(const SDNode *N);
  Value* visitGlobalBaseReg(const SDNode *N);
  Value* visitSRL(const SDNode *N);
  Value* visitSRA(const SDNode *N);
  Value* visitSHL(const SDNode *N);
  Value* visitCALL(const SDNode *N);
  Value* visitCALL_NOP(const SDNode *N);
  Value* visitMTCTR(const SDNode *N);
  Value* visitBCTRL(const SDNode *N);
  Value* visitRET_FLAG(const SDNode *N);
  Value* visitMFOCRF(const SDNode *N);
  Value* visitEH_SJLJ_SETJMP(const SDNode *N);
  Value* visitEH_SJLJ_LONGJMP(const SDNode *N);
  Value* visitVCMP(const SDNode *N);
  Value* visitVCMPo(const SDNode *N);
  Value* visitCOND_BRANCH(const SDNode *N);
  Value* visitBDNZ(const SDNode *N);
  Value* visitBDZ(const SDNode *N);
  Value* visitFADDRTZ(const SDNode *N);
  Value* visitMFFS(const SDNode *N);
  Value* visitLARX(const SDNode *N);
  Value* visitSTCX(const SDNode *N);
  Value* visitTC_RETURN(const SDNode *N);
  Value* visitCR6SET(const SDNode *N);
  Value* visitCR6UNSET(const SDNode *N);
  Value* visitPPC32_GOT(const SDNode *N);
  Value* visitADDIS_GOT_TPREL_HA(const SDNode *N);
  Value* visitLD_GOT_TPREL_L(const SDNode *N);
  Value* visitADD_TLS(const SDNode *N);
  Value* visitADDIS_TLSGD_HA(const SDNode *N);
  Value* visitADDI_TLSGD_L(const SDNode *N);
  Value* visitGET_TLS_ADDR(const SDNode *N);
  Value* visitADDIS_TLSLD_HA(const SDNode *N);
  Value* visitADDI_TLSLD_L(const SDNode *N);
  Value* visitGET_TLSLD_ADDR(const SDNode *N);
  Value* visitADDIS_DTPREL_HA(const SDNode *N);
  Value* visitADDI_DTPREL_L(const SDNode *N);
  Value* visitVADD_SPLAT(const SDNode *N);
  Value* visitSC(const SDNode *N);
  Value* visitSTBRX(const SDNode *N);
  Value* visitLBRX(const SDNode *N);
  Value* visitSTFIWX(const SDNode *N);
  Value* visitLFIWAX(const SDNode *N);
  Value* visitLFIWZX(const SDNode *N);
  Value* visitADDIS_TOC_HA(const SDNode *N);
  Value* visitLD_TOC_L(const SDNode *N);
  Value* visitADDI_TOC_L(const SDNode *N);

};

} // end fracture namespace

#endif
