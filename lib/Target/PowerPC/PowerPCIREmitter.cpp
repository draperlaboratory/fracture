//===- PowerPCIREmitter - Generalize PowerPCISD Instrs  ================-*- C++ -*-=//
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
// Implements visitors for PowerPCISD SDNodes.
//
//===----------------------------------------------------------------------===//

#include "Target/PowerPC/PowerPCIREmitter.h"
#include "CodeInv/Decompiler.h"
#include "PowerPCBaseInfo.h"

using namespace llvm;

namespace fracture {

PowerPCIREmitter::PowerPCIREmitter(Decompiler *TheDec, raw_ostream &InfoOut,
  raw_ostream &ErrOut) : IREmitter(TheDec, InfoOut, ErrOut) {
  // Nothing to do here
}

PowerPCIREmitter::~PowerPCIREmitter() {
  // Nothing to do here
}

Value* PowerPCIREmitter::visit(const SDNode *N) {
  // return the parent if we are in IR only territory
  if (N->getOpcode() <= ISD::BUILTIN_OP_END){
    return IREmitter::visit(N);
  }

  // If we already visited the node, return the result.
  if (VisitMap.find(N) != VisitMap.end()) {
    return VisitMap[N];
  }

  IRB->SetCurrentDebugLocation(N->getDebugLoc());
  DEBUG(Infos << "Visiting PowerPC specific Opcode.\n");
  switch (N->getOpcode()) {
    default:{
      errs() << "OpCode: " << N->getOpcode() << "\n";
      N->dump();
      llvm_unreachable("PPCIREmitter::visit - Every PPC visit should be implemented...");
      return NULL;
    }

    case PPCISD::FSEL:				return visitFSEL(N);
    case PPCISD::FCFID:				return visitFCFID(N);
    case PPCISD::FCFIDU:			return visitFCFIDU(N);
    case PPCISD::FCFIDS:			return visitFCFIDS(N);
    case PPCISD::FCFIDUS:			return visitFCFIDUS(N);
    case PPCISD::FCTIDZ:			return visitFCTIDZ(N);
    case PPCISD::FCTIWZ:			return visitFCTIWZ(N);
    case PPCISD::FCTIDUZ:			return visitFCTIDUZ(N);
    case PPCISD::FCTIWUZ:			return visitFCTIWUZ(N);
    case PPCISD::FRE:					return visitFRE(N);
    case PPCISD::FRSQRTE:			return visitFRSQRTE(N);
    case PPCISD::VMADDFP:			return visitVMADDFP(N);
    case PPCISD::VNMSUBFP:		return visitVNMSUBFP(N);
    case PPCISD::VPERM:				return visitVPERM(N);
    case PPCISD::Hi:					return visitHi(N);
    case PPCISD::Lo:					return visitLo(N);
    case PPCISD::TOC_ENTRY:		return visitTOC_ENTRY(N);
    case PPCISD::TOC_RESTORE:	return visitTOC_RESTORE(N);
    case PPCISD::LOAD:				return visitLOAD(N);
    case PPCISD::LOAD_TOC:		return visitLOAD_TOC(N);
    case PPCISD::DYNALLOC:		return visitDYNALLOC(N);
    case PPCISD::GlobalBaseReg:		return visitGlobalBaseReg(N);
    case PPCISD::SRL:					return visitSRL(N);
    case PPCISD::SRA:					return visitSRA(N);
    case PPCISD::SHL:					return visitSHL(N);
    case PPCISD::CALL:				return visitCALL(N);
    case PPCISD::CALL_NOP:		return visitCALL_NOP(N);
    case PPCISD::MTCTR:				return visitMTCTR(N);
    case PPCISD::BCTRL:				return visitBCTRL(N);
    case PPCISD::RET_FLAG:		return visitRET_FLAG(N);
    case PPCISD::MFOCRF:			return visitMFOCRF(N);
    case PPCISD::EH_SJLJ_SETJMP:	return visitEH_SJLJ_SETJMP(N);
    case PPCISD::EH_SJLJ_LONGJMP:	return visitEH_SJLJ_LONGJMP(N);
    case PPCISD::VCMP:				return visitVCMP(N);
    case PPCISD::VCMPo:				return visitVCMPo(N);
    case PPCISD::COND_BRANCH:	return visitCOND_BRANCH(N);
    case PPCISD::BDNZ:				return visitBDNZ(N);
    case PPCISD::BDZ:					return visitBDZ(N);
    case PPCISD::FADDRTZ:			return visitFADDRTZ(N);
    case PPCISD::MFFS:				return visitMFFS(N);
    case PPCISD::LARX:				return visitLARX(N);
    case PPCISD::STCX:				return visitSTCX(N);
    case PPCISD::TC_RETURN:		return visitTC_RETURN(N);
    case PPCISD::CR6SET:			return visitCR6SET(N);
    case PPCISD::CR6UNSET:		return visitCR6UNSET(N);
    case PPCISD::PPC32_GOT:		return visitPPC32_GOT(N);
    case PPCISD::ADDIS_GOT_TPREL_HA: return visitADDIS_GOT_TPREL_HA(N);
    case PPCISD::LD_GOT_TPREL_L:		return visitLD_GOT_TPREL_L(N);
    case PPCISD::ADD_TLS:						return visitADD_TLS(N);
    case PPCISD::ADDIS_TLSGD_HA:		return visitADDIS_TLSGD_HA(N);
    case PPCISD::ADDI_TLSGD_L:			return visitADDI_TLSGD_L(N);
    case PPCISD::GET_TLS_ADDR:			return visitGET_TLS_ADDR(N);
    case PPCISD::ADDIS_TLSLD_HA:		return visitADDIS_TLSLD_HA(N);
    case PPCISD::ADDI_TLSLD_L:			return visitADDI_TLSLD_L(N);
    case PPCISD::GET_TLSLD_ADDR:		return visitGET_TLSLD_ADDR(N);
    case PPCISD::ADDIS_DTPREL_HA:		return visitADDIS_DTPREL_HA(N);
    case PPCISD::ADDI_DTPREL_L:			return visitADDI_DTPREL_L(N);
    case PPCISD::VADD_SPLAT:				return visitVADD_SPLAT(N);
    case PPCISD::SC:								return visitSC(N);
    case PPCISD::STBRX:							return visitSTBRX(N);
    case PPCISD::LBRX:							return visitLBRX(N);
    case PPCISD::STFIWX:						return visitSTFIWX(N);
    case PPCISD::LFIWAX:						return visitLFIWAX(N);
    case PPCISD::LFIWZX:						return visitLFIWZX(N);
    case PPCISD::ADDIS_TOC_HA:			return visitADDIS_TOC_HA(N);
    case PPCISD::LD_TOC_L:					return visitLD_TOC_L(N);
    case PPCISD::ADDI_TOC_L: 				return visitADDI_TOC_L(N);

  }
}


Value* PowerPCIREmitter::visitFSEL(const SDNode *N) { llvm_unreachable("visitFSEL unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCFID(const SDNode *N) { llvm_unreachable("visitFCFID unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCFIDU(const SDNode *N) { llvm_unreachable("visitFCFIDU unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCFIDS(const SDNode *N) { llvm_unreachable("visitFCFIDS unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCFIDUS(const SDNode *N) { llvm_unreachable("visitFCFIDUS unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCTIDZ(const SDNode *N) { llvm_unreachable("visitFCTIDZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCTIWZ(const SDNode *N) { llvm_unreachable("visitFCTIWZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCTIDUZ(const SDNode *N) { llvm_unreachable("visitFCTIDUZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFCTIWUZ(const SDNode *N) { llvm_unreachable("visitFCTIWUZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFRE(const SDNode *N) { llvm_unreachable("visitFRE unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFRSQRTE(const SDNode *N) { llvm_unreachable("visitFRSQRTE unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVMADDFP(const SDNode *N) { llvm_unreachable("visitVMADDFP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVNMSUBFP(const SDNode *N) { llvm_unreachable("visitVNMSUBFP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVPERM(const SDNode *N) { llvm_unreachable("visitVPERM unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitHi(const SDNode *N) { llvm_unreachable("visitHi unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLo(const SDNode *N) { llvm_unreachable("visitLo unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitTOC_ENTRY(const SDNode *N) { llvm_unreachable("visitTOC_ENTRY unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitTOC_RESTORE(const SDNode *N) { llvm_unreachable("visitTOC_RESTORE unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLOAD(const SDNode *N) { llvm_unreachable("visitLOAD unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLOAD_TOC(const SDNode *N) { llvm_unreachable("visitLOAD_TOC unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitDYNALLOC(const SDNode *N) { llvm_unreachable("visitDYNALLOC unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitGlobalBaseReg(const SDNode *N) { llvm_unreachable("visitGlobalBaseReg unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSRL(const SDNode *N) { llvm_unreachable("visitSRL unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSRA(const SDNode *N) { llvm_unreachable("visitSRA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSHL(const SDNode *N) { llvm_unreachable("visitSHL unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitCALL(const SDNode *N) { llvm_unreachable("visitCALL unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitCALL_NOP(const SDNode *N) { llvm_unreachable("visitCALL_NOP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitMTCTR(const SDNode *N) { llvm_unreachable("visitMTCTR unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitBCTRL(const SDNode *N) { llvm_unreachable("visitBCTRL unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitRET_FLAG(const SDNode *N) { llvm_unreachable("visitRET_FLAG unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitMFOCRF(const SDNode *N) { llvm_unreachable("visitMFOCRF unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitEH_SJLJ_SETJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_SETJMP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitEH_SJLJ_LONGJMP(const SDNode *N) { llvm_unreachable("visitEH_SJLJ_LONGJMP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVCMP(const SDNode *N) { llvm_unreachable("visitVCMP unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVCMPo(const SDNode *N) { llvm_unreachable("visitVCMPo unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitCOND_BRANCH(const SDNode *N) { llvm_unreachable("visitCOND_BRANCH unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitBDNZ(const SDNode *N) { llvm_unreachable("visitBDNZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitBDZ(const SDNode *N) { llvm_unreachable("visitBDZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitFADDRTZ(const SDNode *N) { llvm_unreachable("visitFADDRTZ unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitMFFS(const SDNode *N) { llvm_unreachable("visitMFFS unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLARX(const SDNode *N) { llvm_unreachable("visitLARX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSTCX(const SDNode *N) { llvm_unreachable("visitSTCX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitTC_RETURN(const SDNode *N) { llvm_unreachable("visitTC_RETURN unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitCR6SET(const SDNode *N) { llvm_unreachable("visitCR6SET unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitCR6UNSET(const SDNode *N) { llvm_unreachable("visitCR6UNSET unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitPPC32_GOT(const SDNode *N) { llvm_unreachable("visitPPC32_GOT unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDIS_GOT_TPREL_HA(const SDNode *N) { llvm_unreachable("visitADDIS_GOT_TPREL_HA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLD_GOT_TPREL_L(const SDNode *N) { llvm_unreachable("visitLD_GOT_TPREL_L unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADD_TLS(const SDNode *N) { llvm_unreachable("visitADD_TLS unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDIS_TLSGD_HA(const SDNode *N) { llvm_unreachable("visitADDIS_TLSGD_HA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDI_TLSGD_L(const SDNode *N) { llvm_unreachable("visitADDI_TLSGD_L unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitGET_TLS_ADDR(const SDNode *N) { llvm_unreachable("visitGET_TLS_ADDR unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDIS_TLSLD_HA(const SDNode *N) { llvm_unreachable("visitADDIS_TLSLD_HA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDI_TLSLD_L(const SDNode *N) { llvm_unreachable("visitADDI_TLSLD_L unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitGET_TLSLD_ADDR(const SDNode *N) { llvm_unreachable("visitGET_TLSLD_ADDR unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDIS_DTPREL_HA(const SDNode *N) { llvm_unreachable("visitADDIS_DTPREL_HA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDI_DTPREL_L(const SDNode *N) { llvm_unreachable("visitADDI_DTPREL_L unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitVADD_SPLAT(const SDNode *N) { llvm_unreachable("visitVADD_SPLAT unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSC(const SDNode *N) { llvm_unreachable("visitSC unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSTBRX(const SDNode *N) { llvm_unreachable("visitSTBRX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLBRX(const SDNode *N) { llvm_unreachable("visitLBRX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitSTFIWX(const SDNode *N) { llvm_unreachable("visitSTFIWX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLFIWAX(const SDNode *N) { llvm_unreachable("visitLFIWAX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLFIWZX(const SDNode *N) { llvm_unreachable("visitLFIWZX unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDIS_TOC_HA(const SDNode *N) { llvm_unreachable("visitADDIS_TOC_HA unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitLD_TOC_L(const SDNode *N) { llvm_unreachable("visitLD_TOC_L unimplemented PPC visit..."); return NULL; }
Value* PowerPCIREmitter::visitADDI_TOC_L(const SDNode *N) { llvm_unreachable("visitADDI_TOC_L unimplemented PPC visit..."); return NULL; }



} // end fracture namespace
