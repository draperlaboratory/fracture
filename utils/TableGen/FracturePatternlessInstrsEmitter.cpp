//===- FracturePatternlessInstrsEmitter.cpp - patternless Instrs =-*- C++ -*-=//
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
// This tablegen backend emits patternless instructions.
//
//===----------------------------------------------------------------------===//

#include "CodeGenInstruction.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/StringMatcher.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <algorithm>
#include <cctype>

using namespace llvm;

namespace {

/// hasNullFragReference - Return true if the DAG has any reference to the
/// null_frag operator.
static bool hasNullFragReference(DagInit *DI) {
  DefInit *OpDef = dyn_cast<DefInit>(DI->getOperator());
  if (!OpDef) return false;
  Record *Operator = OpDef->getDef();

  // If this is the null fragment, return true.
  if (Operator->getName() == "null_frag") return true;
  // If any of the arguments reference the null fragment, return true.
  for (unsigned i = 0, e = DI->getNumArgs(); i != e; ++i) {
    DagInit *Arg = dyn_cast<DagInit>(DI->getArg(i));
    if (Arg && hasNullFragReference(Arg))
      return true;
  }

  return false;
}

/// hasNullFragReference - Return true if any DAG in the list references
/// the null_frag operator.
static bool hasNullFragReference(ListInit *LI) {
  for (unsigned i = 0, e = LI->getSize(); i != e; ++i) {
    DagInit *DI = dyn_cast<DagInit>(LI->getElement(i));
    assert(DI && "non-dag in an instruction Pattern list?!");
    if (hasNullFragReference(DI))
      return true;
  }
  return false;
}

class FracturePatternlessInstrsEmitter {
public:
  std::vector<CodeGenInstruction*> Instructions;
  explicit FracturePatternlessInstrsEmitter(RecordKeeper &R) : Records(&R) {}
  void run(raw_ostream &OS);
private:
  RecordKeeper *Records;
  void ParseInstructions();
};


void FracturePatternlessInstrsEmitter::ParseInstructions() {
  std::vector<Record*> Instrs = Records->getAllDerivedDefinitions("Instruction");
  for (unsigned i = 0, e = Instrs.size(); i != e; ++i) {
    ListInit *LI = 0;

    if (isa<ListInit>(Instrs[i]->getValueInit("Pattern")))
      LI = Instrs[i]->getValueAsListInit("Pattern");

    // If there is no pattern, only collect minimal information about the
    // instruction for its operand list.  We have to assume that there is one
    // result, as we have no detailed info. A pattern which references the
    // null_frag operator is as-if no pattern were specified. Normally this
    // is from a multiclass expansion w/ a SDPatternOperator passed in as
    // null_frag.
    if (!LI || LI->getSize() == 0 || hasNullFragReference(LI)) {
      Instructions.push_back(new CodeGenInstruction(Instrs[i]));
    }
  }
}


}

namespace fracture {

// Emits the class definitions for attributes.
void EmitPatternlessInstrs(RecordKeeper &Records, raw_ostream &OS) {
  FracturePatternlessInstrsEmitter(Records).run(OS);
}

} // end namespace fracture

void  FracturePatternlessInstrsEmitter::run(raw_ostream &OS) {
  ParseInstructions();

  emitSourceFileHeader("Patternless instructions for the " 
    + Records->getAllDerivedDefinitions("Target")[0]->getName()
    + " target", OS);

  // TODO: Add implicit uses/defs to the results and ops list below.
  for (unsigned i = 0; i < Instructions.size(); i++) {
    CodeGenInstruction *CurInst = Instructions[i];

    OS << "[";

    // The Results 
    unsigned NumDefs = CurInst->Operands.NumDefs;
    for (unsigned i = 0; i < NumDefs; i++) {
      if (i == 0) OS << "(";
      std::string OpName = CurInst->Operands[i].Rec->getName();
      OpName += ":$" + CurInst->Operands[i].Name;
      if (OpName == "") OS << "UnkOp";
      else OS << OpName;
      OS << ", ";
    }

    // Instruction name
    OS << "(" << CurInst->TheDef->getName() << " ";

    // Operands
    for (unsigned i = NumDefs; i < CurInst->Operands.size(); i++) {
      std::string OpName = CurInst->Operands[i].Rec->getName();
      OpName += ":$" + CurInst->Operands[i].Name;
      if (OpName == "") OS << "UnkOp";
      else OS << OpName;
      if (i < CurInst->Operands.size()-1) OS << ", ";
    }

    if (NumDefs != 0) OS << ")";
    OS << ")]\n";
  }
}
