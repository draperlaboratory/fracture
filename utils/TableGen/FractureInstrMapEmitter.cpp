//===- FractureInstrMapEmitter.cpp - patternless Instrs =-*- C++ -*-=//
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

#include "CodeInvDAGPatterns.h"
#include "DAGISelMatcher.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/StringMatcher.h"
#include "llvm/TableGen/TableGenBackend.h"
#include "llvm/Support/Debug.h"
#include <algorithm>
#include <cctype>

using namespace llvm;

#define DEBUG_TYPE "invdag-patterns"

class FractureInstrMapEmitter {
  const CodeInvDAGPatterns CIP;
  Matcher *TheMatcher, *CurNode;
  StringMap<unsigned> VariableMap;
  unsigned NextOpNo;
  bool MatcherGenFailed;

  /// MatchedComplexPatterns - This maintains a list of all of the
  /// ComplexPatterns that we need to check.  The patterns are known to have
  /// names which were recorded.  The second element of each pair is the first
  /// slot number that the OPC_CheckComplexPat opcode drops the matched
  /// results into.
  SmallVector<std::pair<const InvTreePatternNode*,
                        unsigned>, 2> MatchedComplexPatterns;
  /// MatchedChainNodes - This maintains the position in the recorded nodes
  /// array of all of the recorded input nodes that have chains.
  SmallVector<unsigned, 2> MatchedChainNodes;

public:
  explicit FractureInstrMapEmitter(RecordKeeper &R)
    : CIP(R), TheMatcher(0), CurNode(0), NextOpNo(0) {}
  void run(raw_ostream &OS);
private:
  void AddMatcher(Matcher* NewNode);
  void CreateMatcherFromPattern(const InvPatternToMatch *Pattern);

  // These functions create pattern checks
  void EmitMatcherCode(const InvTreePatternNode *N);
  void EmitMatchCode(const InvTreePatternNode *N);
  void EmitOperatorMatchCode(const InvTreePatternNode *N);
  void EmitLeafMatchCode(const InvTreePatternNode *N);
  void EmitComplexPattern(const InvTreePatternNode* N, unsigned ChildBase);


  // These functions emit the resulting patterns
  void EmitResultCode(const InvPatternToMatch *Pattern);
  void EmitResultOfNamedOperand(const InvTreePatternNode *N, 
    SmallVectorImpl<unsigned> &Results);
  void EmitResultLeafAsOperand(const InvTreePatternNode *N, 
    SmallVectorImpl<unsigned> &Results);
  void EmitResultInstructionAsOperand(const InvTreePatternNode *N,
    SmallVectorImpl<unsigned> &Results);
  void EmitResultOperand(const InvTreePatternNode *N, 
    SmallVectorImpl<unsigned> &Results);
};

/// getRegisterValueType - Look up and return the ValueType of the specified
/// register. If the register is a member of multiple register classes which
/// have different associated types, return MVT::Other.
static MVT::SimpleValueType getRegisterValueType(Record *R,
                                                 const CodeGenTarget &T) {
  bool FoundRC = false;
  MVT::SimpleValueType VT = MVT::Other;
  const CodeGenRegister *Reg = T.getRegBank().getReg(R);

  //ArrayRef<CodeGenRegisterClass*> RCs = T.getRegBank().getRegClasses();
  std::list<CodeGenRegisterClass> RCs = T.getRegBank().getRegClasses();

  for ( auto RC : RCs ) {
    if (!RC.contains(Reg))
      continue;

    if (!FoundRC) {
      FoundRC = true;
      VT = RC.getValueTypeNum(0);
      continue;
    }

    // If this occurs in multiple register classes, they all have to agree.
    assert(VT == RC.getValueTypeNum(0));

  }
  /*
  for (unsigned rc = 0, e = RCs.size(); rc != e; ++rc) {
    const CodeGenRegisterClass &RC = RCs[rc];
    if (!RC.contains(Reg))
      continue;

    if (!FoundRC) {
      FoundRC = true;
      VT = RC.getValueTypeNum(0);
      continue;
    }

    // If this occurs in multiple register classes, they all have to agree.
    assert(VT == RC.getValueTypeNum(0));
  }
  */

  return VT;
}

namespace fracture {

// Emits the class definitions for attributes.
void EmitInstrMap(RecordKeeper &Records, raw_ostream &OS) {
  FractureInstrMapEmitter(Records).run(OS);
}

} // end namespace fracture

void  FractureInstrMapEmitter::run(raw_ostream &OS) {
  emitSourceFileHeader("DAG Instruction Deselector for the " +
    CIP.getTargetInfo().getName() + "target", OS);

  OS << "// *** NOTE: This file is included into the middle of the decompiler\n"
     << "// *** for the target. These functions are really just methods.\n\n";

  const std::vector<const InvPatternToMatch*> Patterns = CIP.getPatterns();

  DEBUG(
    errs() << "\n\nALL PATTERNS TO MATCH:\n\n";
    for (unsigned i = 0, e = Patterns.size(); i != e; ++i) {
      errs() << "IN:  ";
      Patterns[i]->SrcPattern->print(errs());
      errs() << "\nOUT: ";
      Patterns[i]->DstPattern->print(errs());
      errs() << "\n";
    }
  );

  // Convert each pattern into a matcher
  std::vector<Matcher*> PatternMatchers;
  unsigned FailCount = 0;
  for (unsigned i = 0, e = Patterns.size(); i != e; ++i) {
    MatcherGenFailed = false;
    // Create the matching logic
    errs() << "EmitMatchCode: ";
    Patterns[i]->getSrcPattern()->print(errs());
    errs() << "\n";
    EmitMatcherCode(Patterns[i]->getSrcPattern());
    // The resulting graph
    errs() << "EmitResultCode: ";
    Patterns[i]->getDstPattern()->print(errs());
    errs() << "\n";

    EmitResultCode(Patterns[i]);
    if (!MatcherGenFailed) {
      PatternMatchers.push_back(TheMatcher);
    } else {
      FailCount++;
    }

    // TheMatcher->dump();

    // Reset variables for next loop
    TheMatcher = 0;
    CurNode = 0;
    NextOpNo = 0;
    VariableMap.clear();
    MatchedComplexPatterns.clear();
    MatchedChainNodes.clear();
  }

  // this looks necessary for 3.5+ but it's mintor
  // ArrayRef<Matcher*> pMatchers = ArrayRef<Matcher*>( PatternMatchers );
  //   Matcher *FinalMatcher = new ScopeMatcher( pMatchers );
  Matcher *FinalMatcher = new ScopeMatcher( &PatternMatchers[0], PatternMatchers.size() );

  // TheMatcher->dump();

  outs() << "Fail Count: " << FailCount << "\n";
  outs() << "Number of Matchers: " << PatternMatchers.size() << "\n";

  EmitMatcherTable(FinalMatcher, CIP, OS);
  delete TheMatcher;
}

void FractureInstrMapEmitter::AddMatcher(Matcher *NewNode) {
  if (CurNode != 0) {
    CurNode->setNext(NewNode);
  } else {
    TheMatcher = NewNode;
  }
  CurNode = NewNode;
}

void FractureInstrMapEmitter::EmitResultCode(
  const InvPatternToMatch* Pattern) {

  // Patterns that match nodes with (potentially multiple) chain inputs have to
  // merge them together into a token factor.  This informs the generated code
  // what all the chained nodes are.
  if (!MatchedChainNodes.empty())
    AddMatcher(new EmitMergeInputChainsMatcher
               (MatchedChainNodes.data(), MatchedChainNodes.size()));

  SmallVector<unsigned, 8> Results;
  const InvTreePatternNode *N;
  N = Pattern->getDstPattern();

  if (!N->isLeaf() && N->getRecord()->getName() == "set") {
    N = N->getChild(N->getNumChildren() - 1);
  }

  EmitResultOperand(N, Results);

  unsigned NumResults = Pattern->getDstPattern()->getNumTypes();
  if (NumResults < Results.size()) {
    Results.resize(NumResults);
  }
  for (unsigned int i = NumResults - Results.size(); i != 0; --i) {
    Results.push_back(0);
  }

  // TODO: Add implicit defs/uses?
  // FIXME: Should probably add checks to make sure # of args is coherent.
  AddMatcher(new CompleteMatchMatcher(Results.data(), Results.size(), *Pattern));
}

void FractureInstrMapEmitter::EmitResultOperand(
  const InvTreePatternNode *N, SmallVectorImpl<unsigned> &Results) {
  if (!N->getName().empty()) {
    EmitResultOfNamedOperand(N, Results);
    return;
  }
  if (N->isLeaf()) {
    EmitResultLeafAsOperand(N, Results);
    return;
  }

  Record *OpRec = N->getRecord();
  if (OpRec->isSubClassOf("SDNode")) {
    EmitResultInstructionAsOperand(N, Results);
    return;
  }

  errs() << "Unknown result node to emit code for: " << *N << "\n";
  errs() << OpRec->getName() << "\n";
  std::vector<Record*> classes = OpRec->getSuperClasses();
  errs() << "Superclasses: " << classes.size() << "\n";
  for (unsigned i = 0, e = classes.size(); i != e; ++i) {
    errs() << classes[i]->getName() << "\n";
  }
  MatcherGenFailed = true;
  // abort();
}


void FractureInstrMapEmitter::EmitResultOfNamedOperand(
  const InvTreePatternNode *N, SmallVectorImpl<unsigned> &Results) {
  assert(!N->getName().empty() && "Operand is not named!");

  // A reference to a complex pattern gets all of the results of the complex
  // pattern's match.
  // if (const ComplexPattern *CP = N->getComplexPatternInfo(CIP)) {
  //   unsigned SlotNo = 0;
  //   for (unsigned i = 0, e = MatchedComplexPatterns.size(); i != e; ++i)
  //     if (MatchedComplexPatterns[i].first->getName() == N->getName()) {
  //       SlotNo = MatchedComplexPatterns[i].second;
  //       break;
  //     }
  //   assert(SlotNo != 0 && "Didn't get a slot number assigned?");

  //   // The first slot entry is the node itself, the subsequent entries are the
  //   // matched values.
  //   for (unsigned i = 0, e = CP->getNumOperands(); i != e; ++i)
  //     Ops.push_back(SlotNo+i);
  //   return;
  // }

  StringRef Name = N->getName();
  unsigned SlotNo = VariableMap[Name];
  // FIXME: Terrible hack here. SlotNo should never be 0...
  // Unfortunately our pattern matchers don't line up the names.
  if (SlotNo != 0) {
    errs() << "Variable " << Name
           << " referenced but not defined and not caught earlier!";
    SlotNo -= 1;
  }

  Results.push_back(SlotNo);
}

void FractureInstrMapEmitter::EmitResultLeafAsOperand(
  const InvTreePatternNode *N, SmallVectorImpl<unsigned> &Results) {
  assert(N->isLeaf() && "Not a leaf node!");

  // Integers
  if (IntInit *II = dyn_cast<IntInit>(N->getLeafVal())) {
    // TODO: may need to detect int size.
    AddMatcher(new EmitIntegerMatcher(II->getValue(), MVT::i64));
    Results.push_back(NextOpNo++);
    return;
  }

  // Registers
  if (DefInit *DI = dyn_cast<DefInit>(N->getLeafVal())) {
    Record *Def = DI->getDef();
    if (Def->isSubClassOf("Register")) {
      const CodeGenRegister *Reg =
        CIP.getTargetInfo().getRegBank().getReg(Def);
      MVT::SimpleValueType RT = getRegisterValueType(Def, CIP.getTargetInfo());
      AddMatcher(new EmitRegisterMatcher(Reg, RT));
      Results.push_back(NextOpNo++);
      return;
    }
  }

  errs() << "Unknown result node to emit code for: " << *N << "\n";
  Init *LeafVal = N->getLeafVal();
  errs() << "Kind: " << LeafVal->getKind() << "\n";


  // Don't handle anything else (for now)
  errs() << "Unhandled leaf node: \n";
  N->print(errs());
}

void FractureInstrMapEmitter::EmitResultInstructionAsOperand(
  const InvTreePatternNode *N, SmallVectorImpl<unsigned> &Results) {

  const SDNodeInfo &NInfo = CIP.getSDNodeInfo(N->getRecord());
  // Skip ConstantSDNodes, they are not instructions
  // Instead we treat them as unnamed operands
  if (NInfo.getSDClassName() == "ConstantSDNode"
    || NInfo.getSDClassName() == "ConstantFPSDNode") {
    Results.push_back(VariableMap[NInfo.getEnumName()]);
    return;
  }

  // errs() << NInfo.getSDClassName() << " " << NInfo.getEnumName() << " "
  //        << NInfo.getNumResults() << "\n";

  // FIXME: Figure out if we need any of these.
  bool NodeHasChain = false, TreeHasInGlue = false,
    TreeHasOutGlue = false, NodeHasMemRefs = false;

  // unsigned NumResults = NInfo.getNumResults();

  for (unsigned i = 0, e = N->getNumChildren(); i != e; ++i) {
    // outs() << "==>>>";
    // N->getChild(i)->print(outs());
    // outs() << "\n";
    EmitResultOperand(N->getChild(i), Results);

  }

  int NumFixedArityOperands = -1;

  // Determine the result types
  SmallVector<MVT::SimpleValueType, 4> ResultVTs;
  for (unsigned i = 0, e = N->getNumTypes(); i != e; ++i)
    ResultVTs.push_back(N->getType(i));

  // Chains indicate instructions that enforce running order
  // (mem ops, branches, etc)
  if (NInfo.hasProperty(SDNPHasChain)) {
    NodeHasChain = true;
  }
  if (NInfo.hasProperty(SDNPMayLoad) || NInfo.hasProperty(SDNPMayStore)
    || NInfo.hasProperty(SDNPMemOperand)) {
    NodeHasMemRefs = true;
  }

  AddMatcher(new EmitNodeMatcher(NInfo.getEnumName(),
      ResultVTs.data(), ResultVTs.size(), Results.data(), Results.size(),
      NodeHasChain, TreeHasInGlue, TreeHasOutGlue, NodeHasMemRefs,
      NumFixedArityOperands, NextOpNo));

  // FIXME: this is a hack
  if (NodeHasChain && Results.size() != 0) {
    if (Results[0] != 0)
    {
      Results[0] -= 1;
    }
  }

  for (unsigned i = 0, e = ResultVTs.size(); i != e; ++i) {
    if (ResultVTs[i] == MVT::Other || ResultVTs[i] == MVT::Glue) break;
    Results.push_back(NextOpNo++);
  }

}

void FractureInstrMapEmitter::EmitMatchCode(const InvTreePatternNode* N) {
  if (!N->getName().empty()) {
    unsigned &VarNo = VariableMap[N->getName()];
    if (VarNo == 0) {
      AddMatcher(new RecordMatcher("$" + N->getName(), NextOpNo));
      VarNo = ++NextOpNo;
    } else {
      AddMatcher(new CheckSameMatcher(VarNo-1));
    }
  }

  if (N->isLeaf())
    EmitLeafMatchCode(N);
  else
    EmitOperatorMatchCode(N);
}

void FractureInstrMapEmitter::EmitMatcherCode(
  const InvTreePatternNode* N) {

  EmitMatchCode(N);

  // AT this point we've done the structural type match
  for (unsigned i = 0, e = MatchedComplexPatterns.size(); i != e; ++i) {

    const InvTreePatternNode* N = MatchedComplexPatterns[i].first;

    MatchedComplexPatterns[i].second = NextOpNo;

    // Get the slot we recorded the value in from the name on the node.
    unsigned RecNodeEntry = VariableMap[N->getName()];
    assert(!N->getName().empty() && RecNodeEntry &&
           "Complex pattern should have a name and slot");
    --RecNodeEntry;  // Entries in VariableMap are biased.

    const ComplexPattern &CP =
      CIP.getComplexPattern(((DefInit*)N->getLeafVal())->getDef());

    // Emit a CheckComplexPat operation, which does the match (aborting if it
    // fails) and pushes the matched operands onto the recorded nodes list.
    AddMatcher(new CheckComplexPatMatcher(CP, RecNodeEntry,
                                          N->getName(), NextOpNo));

    // Record the right number of operands.
    NextOpNo += CP.getNumOperands();
    if (CP.hasProperty(SDNPHasChain)) {
      // If the complex pattern has a chain, then we need to keep track of the
      // fact that we just recorded a chain input.  The chain input will be
      // matched as the last operand of the predicate if it was successful.
      ++NextOpNo; // Chained node operand.

      // It is the last operand recorded.
      assert(NextOpNo > 1 &&
             "Should have recorded input/result chains at least!");
      MatchedChainNodes.push_back(NextOpNo-1);
    }

    // TODO: Complex patterns can't have output glues, if they did, we'd want
    // to record them.
  }

  return;
}

void FractureInstrMapEmitter::EmitOperatorMatchCode(
  const InvTreePatternNode* N) {
  assert(!N->isLeaf() && "Not an operator!");
  Record *Rec = N->getRecord();

  // FIXME: we are ignoring complex patterns sitting at the root here.

  if (Rec->getName() != "set") {
    // Check that the opcode matches up
    if (Rec->isSubClassOf("Instruction")) {
      AddMatcher(new CheckOpcodeMatcher(CIP.getTgtNodeInfo(Rec)));
    } else {
      const SDNodeInfo &NInfo = CIP.getSDNodeInfo(Rec);
      AddMatcher(new CheckOpcodeMatcher(CIP.getSDNodeInfo(Rec)));
      if (NInfo.getSDClassName() == "ConstantSDNode"
        || NInfo.getSDClassName() == "ConstantFPSDNode") {
        AddMatcher(new RecordMatcher("constant node",
            NextOpNo));
        unsigned &VarNo = VariableMap[NInfo.getEnumName()];
        VarNo = NextOpNo++;
      }
    }
  }

  // If this node has a chain, then the chain at operand #0 is the SDNode, and
  // the child numbers of the node are all offset by one.
  unsigned OpNo = 0;
  if (N->NodeHasProperty(SDNPHasChain , CIP)) {
    // Record the node and remember it in our chained nodes list.
    AddMatcher(new RecordMatcher("'" + N->getRecord()->getName() +
                                         "' chained node",
                                 NextOpNo));
    // Remember all of the input chains our pattern will match.
    MatchedChainNodes.push_back(NextOpNo++);

    // Don't look at the input chain when matching the tree pattern to the
    // SDNode.
    OpNo = 1;
  }

  // Match all the children
  unsigned NumComplexChildren = 0;
  for (unsigned i = 0, e = N->getNumChildren(); i != e; ++i) {
    // Check for complex paterns (which require us to add additional recordMatchers)
    InvTreePatternNode* CurChild = N->getChild(i);
    const ComplexPattern* CP = CurChild->getComplexPatternInfo(CIP);
    if (CP != NULL) {
      EmitComplexPattern(CurChild, i+NumComplexChildren+OpNo);
      // Note: -1 is because the current element is replaced, and it is not
      //       completely additive.
      NumComplexChildren += CP->getNumOperands()-1;
    } else {
      AddMatcher(new MoveChildMatcher(i+NumComplexChildren+OpNo));
      EmitMatchCode(CurChild);
      AddMatcher(new MoveParentMatcher());
    }
  }

  return;
}

void FractureInstrMapEmitter::
EmitComplexPattern(const InvTreePatternNode* N, unsigned ChildBase) {
  Record* CPRec = (dyn_cast<DefInit>(N->getLeafVal()))->getDef();

  // FIXME: Maybe put this in the CodeInvDAGPatterns class
  DagInit *DI = CPRec->getValueAsDag("MIOperandInfo");
  unsigned CurOpNo = NextOpNo + 1;
  for (unsigned i = 0, e = DI->getNumArgs(); i != e; ++i) {
    // outs() << "=====>>>>>MIOperand: " << DI->getArgName(i) << "\n";
    std::string ArgName = DI->getArgName(i);
    if (ArgName.empty()) ArgName = i;
    unsigned &VarNo = VariableMap[ArgName];
    AddMatcher(new MoveChildMatcher(ChildBase+i));
    if (VarNo == 0) {
      AddMatcher(new RecordMatcher("$" + ArgName, NextOpNo));
      VarNo = NextOpNo++;
    } else {
      AddMatcher(new CheckSameMatcher(VarNo-1));
    }
    AddMatcher(new MoveParentMatcher());
  }

  // We can't model ComplexPattern uses that don't have their name taken yet.
  // The OPC_CheckComplexPattern operation implicitly records the results.
  if (N->getName().empty()) {
    errs() << "We expect complex pattern uses to have names: " << *N << "\n";
    exit(1);
  }

  // Set the variable number for the complex pattern to the first operand of that
  // pattern.
  // NOTE: We are assuming that 2 complex patterns of the same name/type don't exist
  //       inside the same pattern!
  unsigned &VarNo = VariableMap[N->getName()];
  // FIXME: Terrible hack below...
  if (CurOpNo == 0) {
    CurOpNo = 1;
  }
  VarNo = CurOpNo;

  // Remember this ComplexPattern so that we can emit it after all the other
  // structural matches are done.
  MatchedComplexPatterns.push_back(std::make_pair(N, 0));

  // NOTE: The complex pattern functions are meant to be called AFTER all the
  // nodes are recorded.
  return;
}


/// Adds a leaf matcher
void FractureInstrMapEmitter::EmitLeafMatchCode(
  const InvTreePatternNode* N) {
  assert(N->isLeaf() && "Not a leaf node!");

  // Direct match against constants
  if (IntInit *II = dyn_cast<IntInit>(N->getLeafVal())) {
    AddMatcher(new CheckIntegerMatcher(II->getValue()));
    return;
  }

  DefInit *DI = dyn_cast<DefInit>(N->getLeafVal());
  if (DI == 0) {
    errs() << N->getName() + " has no DefInit.\n";
    return;
  }

  Record *LeafRec = DI->getDef();
  if ( // Handle register references.  Nothing tok do here, they always match.
    LeafRec->isSubClassOf("RegisterClass") ||
    LeafRec->isSubClassOf("RegisterOperand") ||
    LeafRec->isSubClassOf("PointerLikeRegClass") ||
    LeafRec->isSubClassOf("SubRegIndex") ||
    LeafRec->isSubClassOf("Register") ||
    LeafRec->isSubClassOf("CondCode") ||
    // Complex patterns are handled elsewhere.
    LeafRec->isSubClassOf("ComplexPattern") || 
    // Place holder for SRCVALUE nodes. Nothing to do here.
    LeafRec->getName() == "srcvalue" ||
    // FIXME: Ignore predicates for now
    LeafRec->isSubClassOf("PredicateOperand"))
    return;

  if (LeafRec->isSubClassOf("ValueType")) {
    return AddMatcher(new CheckValueTypeMatcher(LeafRec->getName()));
  }

  errs() << "Unknown result node to emit code for: " << *N << "\n";
  Record *OpRec = LeafRec;
  errs() << OpRec->getName() << "\n";
  std::vector<Record*> classes = OpRec->getSuperClasses();
  errs() << "Superclasses: " << classes.size() << "\n";
  for (unsigned i = 0, e = classes.size(); i != e; ++i) {
    errs() << classes[i]->getName() << "\n";
  }

  errs() << N->getName() << " is an unknown leaf node.\n";
}
