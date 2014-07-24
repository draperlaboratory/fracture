//===- ARMInvISelDAG.h - Interface for ARM Inv ISel ==============-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Provides inverse DAG selector functionality for the ARM targets.
//
//===----------------------------------------------------------------------===//

#ifndef ARMINVISELDAG_H
#define ARMINVISELDAG_H

#include "CodeInv/InvISelDAG.h"
#include "ARMISD.h"
#include "Target/ARM/ARMIREmitter.h"
// #include "ARMRegs.h"
#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"

namespace fracture {

enum AddrMode2Type {
  AM2_BASE, // Simple AM2 (+-imm12)
  AM2_SHOP  // Shifter-op AM2
};


class ARMInvISelDAG : public InvISelDAG {
public:
  ARMInvISelDAG(const TargetMachine &TMC,
      CodeGenOpt::Level OL = CodeGenOpt::Default,
      const Decompiler *TheDec = NULL) : InvISelDAG(TMC, OL, TheDec), Dec(TheDec) {};

  ~ARMInvISelDAG() {};

  virtual IREmitter* getEmitter(Decompiler *Dec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls())
  { return new ARMIREmitter(Dec, InfoOut, ErrOut); }


  SDNode* InvertCode(SDNode *N);
  SDNode* Transmogrify(SDNode *N);

  bool CheckComplexPattern(SDNode *Root, SDNode *Parent, SDValue N,
    unsigned PatternNo,
    SmallVectorImpl<std::pair<SDValue, SDNode*> > &Result, 
    unsigned StartNo);

  bool SelectRegShifterOperand(SDValue N, SDValue &A,
                               SDValue &B, SDValue &C,
                               bool CheckProfitability = true);
  bool SelectImmShifterOperand(SDValue N, SDValue &A,
                               SDValue &B, bool CheckProfitability = true);
  bool SelectShiftRegShifterOperand(SDValue N, SDValue &A,
                                    SDValue &B, SDValue &C) {
    // Don't apply the profitability check
    return SelectRegShifterOperand(N, A, B, C, false);
  }
  bool SelectShiftImmShifterOperand(SDValue N, SDValue &A,
                                    SDValue &B) {
    // Don't apply the profitability check
    return SelectImmShifterOperand(N, A, B, false);
  }

  bool SelectAddrModeImm12(SDValue N, SDValue &Base, SDValue &OffImm);
  bool SelectLdStSOReg(SDValue N, SDValue &Base, SDValue &Offset, SDValue &Opc);

  AddrMode2Type SelectAddrMode2Worker(SDValue N, SDValue &Base,
                                      SDValue &Offset, SDValue &Opc);
  bool SelectAddrMode2Base(SDValue N, SDValue &Base, SDValue &Offset,
                           SDValue &Opc) {
    return SelectAddrMode2Worker(N, Base, Offset, Opc) == AM2_BASE;
  }

  bool SelectAddrMode2ShOp(SDValue N, SDValue &Base, SDValue &Offset,
                           SDValue &Opc) {
    return SelectAddrMode2Worker(N, Base, Offset, Opc) == AM2_SHOP;
  }

  bool SelectAddrMode2(SDValue N, SDValue &Base, SDValue &Offset,
                       SDValue &Opc) {
    SelectAddrMode2Worker(N, Base, Offset, Opc);
//    return SelectAddrMode2ShOp(N, Base, Offset, Opc);
    // This always matches one way or another.
    return true;
  }

  bool SelectAddrMode2OffsetReg(SDNode *Op, SDValue N,
                             SDValue &Offset, SDValue &Opc);
  bool SelectAddrMode2OffsetImm(SDNode *Op, SDValue N,
                             SDValue &Offset, SDValue &Opc);
  bool SelectAddrMode2OffsetImmPre(SDNode *Op, SDValue N,
                             SDValue &Offset, SDValue &Opc);
  bool SelectAddrOffsetNone(SDValue N, SDValue &Base);
  bool SelectAddrMode3(SDValue N, SDValue &Base,
                       SDValue &Offset, SDValue &Opc);
  bool SelectAddrMode3Offset(SDNode *Op, SDValue N,
                             SDValue &Offset, SDValue &Opc);
  bool SelectAddrMode5(SDValue N, SDValue &Base,
                       SDValue &Offset);
  bool SelectAddrMode6(SDNode *Parent, SDValue N, SDValue &Addr,SDValue &Align);
  bool SelectAddrMode6Offset(SDNode *Op, SDValue N, SDValue &Offset);

  bool SelectAddrModePC(SDValue N, SDValue &Offset, SDValue &Label);

  // Thumb Addressing Modes:
  bool SelectThumbAddrModeRR(SDValue N, SDValue &Base, SDValue &Offset);
  bool SelectThumbAddrModeRI(SDValue N, SDValue &Base, SDValue &Offset,
                             unsigned Scale);
  bool SelectThumbAddrModeRI5S1(SDValue N, SDValue &Base, SDValue &Offset);
  bool SelectThumbAddrModeRI5S2(SDValue N, SDValue &Base, SDValue &Offset);
  bool SelectThumbAddrModeRI5S4(SDValue N, SDValue &Base, SDValue &Offset);
  bool SelectThumbAddrModeImm5S(SDValue N, unsigned Scale, SDValue &Base,
                                SDValue &OffImm);
  bool SelectThumbAddrModeImm5S1(SDValue N, SDValue &Base,
                                 SDValue &OffImm);
  bool SelectThumbAddrModeImm5S2(SDValue N, SDValue &Base,
                                 SDValue &OffImm);
  bool SelectThumbAddrModeImm5S4(SDValue N, SDValue &Base,
                                 SDValue &OffImm);
  bool SelectThumbAddrModeSP(SDValue N, SDValue &Base, SDValue &OffImm);

  // Thumb 2 Addressing Modes:
  bool SelectT2ShifterOperandReg(SDValue N,
                                 SDValue &BaseReg, SDValue &Opc);
  bool SelectT2AddrModeImm12(SDValue N, SDValue &Base, SDValue &OffImm);
  bool SelectT2AddrModeImm8(SDValue N, SDValue &Base,
                            SDValue &OffImm);
  bool SelectT2AddrModeImm8Offset(SDNode *Op, SDValue N,
                                 SDValue &OffImm);
  bool SelectT2AddrModeSoReg(SDValue N, SDValue &Base,
                             SDValue &OffReg, SDValue &ShImm);

  SDNode *SelectARMIndexedLoad(SDNode *N);
  SDNode *SelectT2IndexedLoad(SDNode *N);

  // Increment, Before, WriteBack
  void InvLoadOrStoreMultiple(SDNode *N, bool Ld, bool Inc, bool B, bool WB);
  bool SelectCMOVPred(SDValue N, SDValue &Pred, SDValue &Reg);

private:
  const Decompiler *Dec;
};

} // end fracture namespace

#endif
