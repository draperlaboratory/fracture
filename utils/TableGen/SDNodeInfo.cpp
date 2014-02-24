//===- SDNodeInfo.cpp - Holds SDNode information =================-*- C++ -*-=//
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

#include "SDNodeInfo.h"

namespace llvm {

//===----------------------------------------------------------------------===//
// SDTypeConstraint implementation
//

SDTypeConstraint::SDTypeConstraint(Record *R) {
  OperandNo = R->getValueAsInt("OperandNum");

  if (R->isSubClassOf("SDTCisVT")) {
    ConstraintType = SDTCisVT;
    x.SDTCisVT_Info.VT = getValueType(R->getValueAsDef("VT"));
    if (x.SDTCisVT_Info.VT == MVT::isVoid) {
      errs() << " Cannot use 'Void' as type to SDTCisVT\n";
      abort();
    }

  } else if (R->isSubClassOf("SDTCisPtrTy")) {
    ConstraintType = SDTCisPtrTy;
  } else if (R->isSubClassOf("SDTCisInt")) {
    ConstraintType = SDTCisInt;
  } else if (R->isSubClassOf("SDTCisFP")) {
    ConstraintType = SDTCisFP;
  } else if (R->isSubClassOf("SDTCisVec")) {
    ConstraintType = SDTCisVec;
  } else if (R->isSubClassOf("SDTCisSameAs")) {
    ConstraintType = SDTCisSameAs;
    x.SDTCisSameAs_Info.OtherOperandNum = R->getValueAsInt("OtherOperandNum");
  } else if (R->isSubClassOf("SDTCisVTSmallerThanOp")) {
    ConstraintType = SDTCisVTSmallerThanOp;
    x.SDTCisVTSmallerThanOp_Info.OtherOperandNum =
      R->getValueAsInt("OtherOperandNum");
  } else if (R->isSubClassOf("SDTCisOpSmallerThanOp")) {
    ConstraintType = SDTCisOpSmallerThanOp;
    x.SDTCisOpSmallerThanOp_Info.BigOperandNum =
      R->getValueAsInt("BigOperandNum");
  } else if (R->isSubClassOf("SDTCisEltOfVec")) {
    ConstraintType = SDTCisEltOfVec;
    x.SDTCisEltOfVec_Info.OtherOperandNum = R->getValueAsInt("OtherOpNum");
  } else if (R->isSubClassOf("SDTCisSubVecOfVec")) {
    ConstraintType = SDTCisSubVecOfVec;
    x.SDTCisSubVecOfVec_Info.OtherOperandNum =
      R->getValueAsInt("OtherOpNum");
  } else {
    errs() << "Unrecognized SDTypeConstraint '" << R->getName() << "'!\n";
    exit(1);
  }
}


//===----------------------------------------------------------------------===//
// SDNodeInfo implementation
//
SDNodeInfo::SDNodeInfo(Record *R) : Def(R) {

  EnumName    = R->getValueAsString("Opcode");
  SDClassName = R->getValueAsString("SDClass");
  Record *TypeProfile = R->getValueAsDef("TypeProfile");
  NumResults = TypeProfile->getValueAsInt("NumResults");
  NumOperands = TypeProfile->getValueAsInt("NumOperands");

  // Parse the properties.
  Properties = 0;
  std::vector<Record*> PropList = R->getValueAsListOfDefs("Properties");
  for (unsigned i = 0, e = PropList.size(); i != e; ++i) {
    if (PropList[i]->getName() == "SDNPCommutative") {
      Properties |= 1 << SDNPCommutative;
    } else if (PropList[i]->getName() == "SDNPAssociative") {
      Properties |= 1 << SDNPAssociative;
    } else if (PropList[i]->getName() == "SDNPHasChain") {
      Properties |= 1 << SDNPHasChain;
    } else if (PropList[i]->getName() == "SDNPOutGlue") {
      Properties |= 1 << SDNPOutGlue;
    } else if (PropList[i]->getName() == "SDNPInGlue") {
      Properties |= 1 << SDNPInGlue;
    } else if (PropList[i]->getName() == "SDNPOptInGlue") {
      Properties |= 1 << SDNPOptInGlue;
    } else if (PropList[i]->getName() == "SDNPMayStore") {
      Properties |= 1 << SDNPMayStore;
    } else if (PropList[i]->getName() == "SDNPMayLoad") {
      Properties |= 1 << SDNPMayLoad;
    } else if (PropList[i]->getName() == "SDNPSideEffect") {
      Properties |= 1 << SDNPSideEffect;
    } else if (PropList[i]->getName() == "SDNPMemOperand") {
      Properties |= 1 << SDNPMemOperand;
    } else if (PropList[i]->getName() == "SDNPVariadic") {
      Properties |= 1 << SDNPVariadic;
    } else {
      errs() << "Unknown SD Node property '" << PropList[i]->getName()
             << "' on node '" << R->getName() << "'!\n";
      exit(1);
    }
  }


  // Parse the type constraints.
  // std::vector<Record*> ConstraintList =
  //   TypeProfile->getValueAsListOfDefs("Constraints");
  // TypeConstraints.assign(ConstraintList.begin(), ConstraintList.end());
}

/// getKnownType - If the type constraints on this node imply a fixed type
/// (e.g. all stores return void, etc), then return it as an
/// MVT::SimpleValueType.  Otherwise, return EEVT::Other.
MVT::SimpleValueType SDNodeInfo::getKnownType(unsigned ResNo) const {
  // unsigned NumResults = getNumResults();
  // assert(NumResults <= 1 &&
  //        "We only work with nodes with zero or one result so far!");
  // assert(ResNo == 0 && "Only handles single result nodes so far");

  // for (unsigned i = 0, e = TypeConstraints.size(); i != e; ++i) {
  //   // Make sure that this applies to the correct node result.
  //   if (TypeConstraints[i].OperandNo >= NumResults)  // FIXME: need value #
  //     continue;

  //   switch (TypeConstraints[i].ConstraintType) {
  //   default: break;
  //   case SDTypeConstraint::SDTCisVT:
  //     return TypeConstraints[i].x.SDTCisVT_Info.VT;
  //   case SDTypeConstraint::SDTCisPtrTy:
  //     return MVT::iPTR;
  //   }
  // }
  return MVT::Other;
}

} // end llvm namespace
