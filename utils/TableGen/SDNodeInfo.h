//===- SDNodeInfo.h - Holds SDNode information ===================-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
// The following license applies to utils/TableGen in r173931 of clang:
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Use "diff" to determine changes made. 
//
//===----------------------------------------------------------------------===//
//
// This class was extracted from utils/TableGen/CodeGenDAGPatterns.h/.cpp and is 
// originally part of LLVM. 
//
//===----------------------------------------------------------------------===//

#ifndef SDNODEINFO_H
#define SDNODEINFO_H

#include "llvm/TableGen/Record.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "CodeGenTarget.h"

namespace llvm {

/// SDTypeConstraint - This is a discriminated union of constraints,
/// corresponding to the SDTypeConstraint tablegen class in Target.td.
struct SDTypeConstraint {
  SDTypeConstraint(Record *R);

  unsigned OperandNo;   // The operand # this constraint applies to.
  enum {
    SDTCisVT, SDTCisPtrTy, SDTCisInt, SDTCisFP, SDTCisVec, SDTCisSameAs,
    SDTCisVTSmallerThanOp, SDTCisOpSmallerThanOp, SDTCisEltOfVec,
    SDTCisSubVecOfVec
  } ConstraintType;

  union {   // The discriminated union.
    struct {
      MVT::SimpleValueType VT;
    } SDTCisVT_Info;
    struct {
      unsigned OtherOperandNum;
    } SDTCisSameAs_Info;
    struct {
      unsigned OtherOperandNum;
    } SDTCisVTSmallerThanOp_Info;
    struct {
      unsigned BigOperandNum;
    } SDTCisOpSmallerThanOp_Info;
    struct {
      unsigned OtherOperandNum;
    } SDTCisEltOfVec_Info;
    struct {
      unsigned OtherOperandNum;
    } SDTCisSubVecOfVec_Info;
  } x;

};

/// SDNodeInfo - One of these records is created for each SDNode instance in
/// the target .td file.  This represents the various dag nodes we will be
/// processing.
class SDNodeInfo {
  Record *Def;
  std::string EnumName;
  std::string SDClassName;
  unsigned Properties;
  unsigned NumResults;
  int NumOperands;
  // std::vector<SDTypeConstraint> TypeConstraints;
public:
  SDNodeInfo(Record *R);  // Parse the specified record.

  unsigned getNumResults() const { return NumResults; }

  /// getNumOperands - This is the number of operands required or -1 if
  /// variadic.
  int getNumOperands() const { return NumOperands; }
  Record *getRecord() const { return Def; }
  const std::string &getEnumName() const { return EnumName; }
  const std::string &getSDClassName() const { return SDClassName; }

  // const std::vector<SDTypeConstraint> &getTypeConstraints() const {
  //   return TypeConstraints;
  // }

  /// getKnownType - If the type constraints on this node imply a fixed type
  /// (e.g. all stores return void, etc), then return it as an
  /// MVT::SimpleValueType.  Otherwise, return MVT::Other.
  MVT::SimpleValueType getKnownType(unsigned ResNo) const;

  /// hasProperty - Return true if this node has the specified property.
  ///
  bool hasProperty(enum SDNP Prop) const { return Properties & (1 << Prop); }

};

} //end llvm namespace

#endif
