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

#ifndef IREMITTER_H
#define IREMITTER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include <stack>
#include <map>

using namespace llvm;

namespace fracture {

class Decompiler;

class IREmitter {
public:
  IREmitter(Decompiler *TheDec, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  virtual ~IREmitter();

  void EmitIR(BasicBlock *BB, SDNode *CurNode,
    std::stack<SDNode *> &NodeStack, std::map<SDValue, Value*> OpMap);

  // This function emulates createValueName(StringRef Name, Value *V) in the
  // ValueSymbolTable class, with exception that BaseName's ending in a number
  // get an additional "_" added to the end.
  StringRef getIndexedValueName(StringRef BaseName);
  // Returns the BaseName used to create the given indexed value name, or the
  // same name if it doesn't exist.
  StringRef getBaseValueName(StringRef BaseName);
  // Returns the Instruction Name by looking for a CopyToReg as a node user, or
  // returns empty.
  StringRef getInstructionName(const SDNode *N);


  IRBuilder<>* getIRB() { return IRB; }
  void setDAG(SelectionDAG *NewDAG) {
    EndHandleDAG = false;
    DAG = NewDAG;
  }

  void endDAG() { assert(EndHandleDAG && "Reached End of DAG and did not see handle node."); }
protected:
  bool EndHandleDAG;
  Decompiler *Dec;
  SelectionDAG *DAG;
  IRBuilder<> *IRB;
  // FIXME: The following need to be reset when we hit a Return instruction.
  //        We should change the design to avoid this kind of state tracking.
  // RegMap saves register ID's to a variable that can be loaded/stored
  IndexedMap<Value*> RegMap;
  DenseMap<const SDNode*, Value*> VisitMap;
  StringMap<StringRef> BaseNames;

  // Visit Functions (Convert SDNode into Instruction/Value)
  virtual Value* visit(const SDNode *N);
  Value* visitCopyFromReg(const SDNode *N);
  Value* visitCopyToReg(const SDNode *N);
  Value* visitConstant(const SDNode *N);
  Value* visitTokenFactor(const SDNode *N);
  Value* visitMERGE_VALUES(const SDNode *N);
  Value* visitADD(const SDNode *N);
  Value* visitSUB(const SDNode *N);
  Value* visitADDC(const SDNode *N);
  Value* visitSUBC(const SDNode *N);
  Value* visitADDE(const SDNode *N);
  Value* visitSUBE(const SDNode *N);
  Value* visitMUL(const SDNode *N);
  Value* visitSDIV(const SDNode *N);
  Value* visitUDIV(const SDNode *N);
  Value* visitSREM(const SDNode *N);
  Value* visitUREM(const SDNode *N);
  Value* visitMULHU(const SDNode *N);
  Value* visitMULHS(const SDNode *N);
  Value* visitSMUL_LOHI(const SDNode *N);
  Value* visitUMUL_LOHI(const SDNode *N);
  Value* visitSMULO(const SDNode *N);
  Value* visitUMULO(const SDNode *N);
  Value* visitSDIVREM(const SDNode *N);
  Value* visitUDIVREM(const SDNode *N);
  Value* visitAND(const SDNode *N);
  Value* visitOR(const SDNode *N);
  Value* visitXOR(const SDNode *N);
  Value* visitSHL(const SDNode *N);
  Value* visitSRA(const SDNode *N);
  Value* visitSRL(const SDNode *N);
  Value* visitCTLZ(const SDNode *N);
  Value* visitCTLZ_ZERO_UNDEF(const SDNode *N);
  Value* visitCTTZ(const SDNode *N);
  Value* visitCTTZ_ZERO_UNDEF(const SDNode *N);
  Value* visitCTPOP(const SDNode *N);
  Value* visitSELECT(const SDNode *N);
  Value* visitVSELECT(const SDNode *N);
  Value* visitSELECT_CC(const SDNode *N);
  Value* visitSETCC(const SDNode *N);
  Value* visitSIGN_EXTEND(const SDNode *N);
  Value* visitZERO_EXTEND(const SDNode *N);
  Value* visitANY_EXTEND(const SDNode *N);
  Value* visitSIGN_EXTEND_INREG(const SDNode *N);
  Value* visitTRUNCATE(const SDNode *N);
  Value* visitBITCAST(const SDNode *N);
  Value* visitBUILD_PAIR(const SDNode *N);
  Value* visitFADD(const SDNode *N);
  Value* visitFSUB(const SDNode *N);
  Value* visitFMUL(const SDNode *N);
  Value* visitFMA(const SDNode *N);
  Value* visitFDIV(const SDNode *N);
  Value* visitFREM(const SDNode *N);
  Value* visitFCOPYSIGN(const SDNode *N);
  Value* visitSINT_TO_FP(const SDNode *N);
  Value* visitUINT_TO_FP(const SDNode *N);
  Value* visitFP_TO_SINT(const SDNode *N);
  Value* visitFP_TO_UINT(const SDNode *N);
  Value* visitFP_ROUND(const SDNode *N);
  Value* visitFP_ROUND_INREG(const SDNode *N);
  Value* visitFP_EXTEND(const SDNode *N);
  Value* visitFNEG(const SDNode *N);
  Value* visitFABS(const SDNode *N);
  Value* visitFCEIL(const SDNode *N);
  Value* visitFTRUNC(const SDNode *N);
  Value* visitFFLOOR(const SDNode *N);
  Value* visitBRCOND(const SDNode *N);
  Value* visitBR(const SDNode *N);
  Value* visitBR_CC(const SDNode *N);
  Value* visitLOAD(const SDNode *N);
  Value* visitSTORE(const SDNode *N);
  Value* visitINSERT_VECTOR_ELT(const SDNode *N);
  Value* visitEXTRACT_VECTOR_ELT(const SDNode *N);
  Value* visitBUILD_VECTOR(const SDNode *N);
  Value* visitCONCAT_VECTORS(const SDNode *N);
  Value* visitEXTRACT_SUBVECTOR(const SDNode *N);
  Value* visitVECTOR_SHUFFLE(const SDNode *N);
  Value* visitRegister(const SDNode *N);
  Value* visitCALL(const SDNode *N);

  /// Error printing
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const {
    Infos << "IREmitter: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "IREmitter: " << Msg << "\n";
    Errs.flush();
  }
};

} // end namespace fracture

#endif /* IREMITTER_H */
