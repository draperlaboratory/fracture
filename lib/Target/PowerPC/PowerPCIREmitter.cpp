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
    default: return NULL;

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
    case PPCISD::CR6SET:			return visitSET(N);
    case PPCISD::CR6UNSET:		return visitUNSET(N);
    case PPCISD::PPC32_GOT:		return visit_GOT(N);
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



//Value* PowerPCIREmitter::visitRET(const SDNode *N) {
//  return IRB->CreateRetVoid();
//}

} // end fracture namespace
