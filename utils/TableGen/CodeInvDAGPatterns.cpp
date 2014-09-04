//===- CodeInvDAGPatterns.cpp - Inverse Code Patterns ============-*- C++ -*-=//
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
// This class creates inverse DAG patterns to convert target-specific
// instructions back to LLVM-generic or target-generic instructions.
//
// Whereas the compilation infrastructure is trying to match patterns to
// potential instructions, we already have instructions and need to
// generate the appropriate IR code graphs so that we can recover IR
// source information. The following aspects make the process different:
//   * Types are defined by the target instruction and need no conversion/map
//   * Side effects need to be recorded
//   * Memory addresses are hard and so are regs which store them
//===----------------------------------------------------------------------===//

#include "CodeInvDAGPatterns.h"
#include "CodeGenInstruction.h"

#define DEBUG_TYPE "invdag-patterns"

namespace llvm {

void InvTreePatternNode::print(raw_ostream &OS) const {
  if (isLeaf()) {
    // OS << *getLeafValue();
    OS << *Val;
  }
  else {
    // OS << '(' << getOperator()->getName();
    OS << '(' << TheRec->getName();

    // if (TheRec->isSubClassOf("PatFrag")) {
    //   OS << "\n\n\n";
    //   if (TheRec->isAnonymous()) OS << "PATFRAG!\n";
    //   TheRec->dump();
    //   const std::vector<Record*> SuperClasses = TheRec->getSuperClasses();
    //   OS << "Superclasses: \n";
    //   for (unsigned i = 0, e = SuperClasses.size(); i != e; ++i) {
    //     OS << "\t" << SuperClasses[i]->getNameInitAsString() << "\n";
    //   }

    //   OS << "\n\n\n";
    // }
  }
  // for (unsigned i = 0, e = Types.size(); i != e; ++i)
  //   OS << ':' << getExtType(i).getName();

  if (!isLeaf()) {
    if (getNumChildren() != 0) {
      OS << " ";
      getChild(0)->print(OS);
      for (unsigned i = 1, e = getNumChildren(); i != e; ++i) {
        OS << ", ";
        getChild(i)->print(OS);
      }
    }
    OS << ")";
  }

  // for (unsigned i = 0, e = PredicateFns.size(); i != e; ++i)
  //   OS << "<<P:" << PredicateFns[i].getFnName() << ">>";
  // if (TransformFn)
  //   OS << "<<X:" << TransformFn->getName() << ">>";
  if (!getName().empty())
    OS << ":$" << getName();

}

/// getIntrinsicInfo - If this node corresponds to an intrinsic, return the
/// CodeGenIntrinsic information for it, otherwise return a null pointer.
const CodeGenIntrinsic *InvTreePatternNode::
getIntrinsicInfo(const CodeInvDAGPatterns &CDP) const {
  std::vector<Record*> classes = getRecord()->getSuperClasses();
  for (unsigned i = 0, e = classes.size(); i != e; ++i) {
    if (classes[i]->getName() == "Intrinsic") {
      return &CDP.getIntrinsic(getRecord());
    }
  }
  return NULL;
}


InvTreePattern::InvTreePattern(Record *TheRec, ListInit *RawPat) : Rec(TheRec) {
  for (unsigned i = 0, e = RawPat->getSize(); i != e; ++i) {
    Trees.push_back(ParseTreePattern(RawPat->getElement(i), ""));
  }
}

InvTreePattern::InvTreePattern(Record *TheRec, DagInit *Pat) : Rec(TheRec) {
  Trees.push_back(ParseTreePattern(Pat, ""));
}

InvTreePattern::InvTreePattern(Record *TheRec, InvTreePatternNode *Pat) 
  : Rec (TheRec) {
  Trees.push_back(Pat);
}

InvTreePatternNode *InvTreePattern::ParseTreePattern(Init *TheInit, StringRef OpName) {
  if (DefInit *DI = dyn_cast<DefInit>(TheInit)) {
    Record *R = DI->getDef();

    // On direct reference to a leaf DagNode (SDNode) or a pattern fragment, create
    // a new InvTreePatternNode.
    if (R->isSubClassOf("SDNode") || R->isSubClassOf("PatFrag")) {
      return ParseTreePattern(DagInit::get(DI, "", 
          std::vector<std::pair<Init*, std::string> >()), OpName);
    }

    // Treat as an input element
    InvTreePatternNode *Res = new InvTreePatternNode(DI);
    if (R->getName() == "node" && OpName.empty()) {
      error("'node' requires an opname to match operand lists!");
    }

    Res->setName(OpName);
    return Res;
   }

  if (IntInit *II = dyn_cast<IntInit>(TheInit)) {
    if (!OpName.empty()) {
      error("Constant int args should not have a name!");
    }
    return new InvTreePatternNode(II);
  }

  if (BitsInit *BI = dyn_cast<BitsInit>(TheInit)) {
    // Convert to IntInit
    Init *II = BI->convertInitializerTo(IntRecTy::get());
    if (II == 0 || !isa<IntInit>(II)) {
      error("Bits values must be integer constants!");
    }
    return ParseTreePattern(II, OpName);
  }

  DagInit *Dag = dyn_cast<DagInit>(TheInit);
  if (!Dag) {
    TheInit->dump();
    error("The Pattern has an unexpected init type.");
  }
  DefInit *OpDef = dyn_cast<DefInit>(Dag->getOperator());
  if (!OpDef) {
    error("Thye Pattern has an unexpected operator type.");
  }
  Record *OpRec = OpDef->getDef();

  if (OpRec->isSubClassOf("ValueType")) {
    // ValueType is the type of a leaf node.
    if (Dag->getNumArgs() != 1) {
      error("Expected 1 argument for a ValueType operator.");
    }
    InvTreePatternNode *New = ParseTreePattern(Dag->getArg(0), Dag->getArgName(0));

    // assert(New->getNumTypes() == 1 && "Unable to handle multiple types!");
    // New->UpdateNodeType(0, getValueType(OpRec), *this);

    if (!OpName.empty()) {
      error("ValueType should not have a name!");
    }
    return New;
  }

  // Verify that this makes sense for an operator
  if (!OpRec->isSubClassOf("PatFrag") && !OpRec->isSubClassOf("SDNode") &&
    !OpRec->isSubClassOf("Instruction") && !OpRec->isSubClassOf("SDNodeXForm") &&
    !OpRec->isSubClassOf("Intrinsic") && OpRec->getName() != "set" &&
    OpRec->getName() != "implicit" && OpRec->getName() != "outs" &&
    OpRec->getName() != "ins" && OpRec->getName() != "null_frag") {
    error("Unrecognized node '" + OpRec->getName() + "'!");
  }

  // Unlike Regular treepatterns, we assume all patterns are "input" patterns
  // in the TableGen context
  if (OpRec->isSubClassOf("Instruction") ||
    OpRec->isSubClassOf("SDNodeXForm")) {
    error("Cannot use '" + OpRec->getName() + "' in the output pattern.");
  }

  std::vector<InvTreePatternNode*> Children;

  // Parse operands
  for (unsigned i = 0, e = Dag->getNumArgs(); i != e; ++i) {
    Children.push_back(ParseTreePattern(Dag->getArg(i), Dag->getArgName(i)));
  }

  if (OpRec->isSubClassOf("Intrinsic")) {
    // Unhandled...
    DEBUG(error("Intrinsics unhandled at this time."););
  }

  InvTreePatternNode *Res = new InvTreePatternNode(OpRec, Children);
  Res->setName(OpName);

  if (!Dag->getName().empty()) {
    assert(Res->getName().empty());
    Res->setName(Dag->getName());
  }
  return Res;
}

/// getComplexPatternInfo - If this node corresponds to a ComplexPattern,
/// return the ComplexPattern information, otherwise return null.
const ComplexPattern *
InvTreePatternNode::getComplexPatternInfo(const CodeInvDAGPatterns &CGP) const {
  if (!isLeaf()) return 0;

  DefInit *DI = dyn_cast<DefInit>(getLeafVal());
  if (DI && DI->getDef()->isSubClassOf("ComplexPattern"))
    return &CGP.getComplexPattern(DI->getDef());
  return 0;
}

/// NodeHasProperty - Return true if this node has the specified property.
bool InvTreePatternNode::NodeHasProperty(SDNP Property,
                                      const CodeInvDAGPatterns &CGP) const {
  if (isLeaf()) {
    if (const ComplexPattern *CP = getComplexPatternInfo(CGP))
      return CP->hasProperty(Property);
    return false;
  }

  Record *Operator = getRecord();
  if (Operator->isSubClassOf("SDNode")) {
    return CGP.getSDNodeInfo(Operator).hasProperty(Property);
  }

  if (Operator->isSubClassOf("Instruction")) {
    const CodeGenInstruction &CGI = CGP.getTgtNodeInfo(Operator);
    switch (Property) {
      case SDNPHasChain:
        // Objects have chains when they change memory or control flow
        // FIXME: add iscall or isbranch/indirectbranch?
        if (CGI.mayLoad || CGI.mayStore || CGI.isBranch || CGI.isReturn
            || CGI.isCall)
          return true;
        break;
      default:
        return false;
    }
  }

  return false;
}

// Update the node type to match an instruction operand or result as specified
// in the ins or outs lists on the instruction definition. Return true if the
// type was actually changed.
bool InvTreePatternNode::UpdateNodeTypeFromInst(unsigned ResNo,
                                                Record *Operand,
                                                CodeGenTarget &Tgt) {
  // The 'unknown' operand indicates that types should be inferred from the
  // context.
  if (Operand->isSubClassOf("unknown_class"))
    return false;

  // The Operand class specifies a type directly.
  if (Operand->isSubClassOf("Operand")) {
    setType(ResNo, getValueType(Operand->getValueAsDef("Type")));
    return true;
  }

  // PointerLikeRegClass has a type that is determined at runtime.
  if (Operand->isSubClassOf("PointerLikeRegClass")) {
    setType(ResNo, MVT::iPTR);
    return true;
  }

  // Both RegisterClass and RegisterOperand operands derive their types from a
  // register class def.
  Record *RC = 0;
  if (Operand->isSubClassOf("RegisterClass"))
    RC = Operand;
  else if (Operand->isSubClassOf("RegisterOperand"))
    RC = Operand->getValueAsDef("RegClass");

  if (RC != NULL) {
    const std::vector<MVT::SimpleValueType> VTs =
      Tgt.getRegisterClass(RC).getValueTypes();
    if (VTs.size() != 0) {
      setType(ResNo, VTs[0]);
      return true;
    }
  }

  if (Operand->isSubClassOf("Register")) {
    const CodeGenRegister *CGR = Tgt.getRegisterByName(Operand->getName());
    if (CGR) {
      std::vector<MVT::SimpleValueType> CGRVTs =
        Tgt.getRegisterVTs(CGR->TheDef);
      if (CGRVTs.size() != 0) {
        setType(ResNo, CGRVTs[0]);
        return true;
      }
    }
    // NOTE: EFLAGS Register Overload for x86 this may break on other archs.
    // Not clear why EFLAGS td does not have a discernable type.
    setType(ResNo, MVT::i32);
    return true;
  }

  return false;
}



InvTreePatternNode& InvTreePatternNode::operator=(const InvTreePatternNode &RHS) {
  TheRec = RHS.TheRec;
  Val = RHS.Val;
  Name = RHS.Name;
  Children = RHS.Children;
  // FIXME: It may be appropriate to compare Children to make sure the right options 
  // are available.
  return *this;
}

void InvTreePattern::print(raw_ostream &OS) const {
  OS << Rec->getName();
  // if (!Args.empty()) {
  //   OS << "(" << Args[0];
  //   for (unsigned i = 1, e = Args.size(); i != e; ++i)
  //     OS << ", " << Args[i];
  //   OS << ")";
  // }
  OS << ": ";

  if (Trees.size() > 1)
    OS << "[\n";
  for (unsigned i = 0, e = Trees.size(); i != e; ++i) {
    OS << "\t";
    Trees[i]->print(OS);
    OS << "\n";
  }

  if (Trees.size() > 1)
    OS << "]\n";
}

//===----------------------------------------------------------------------===//
// Instruction Analysis
//===----------------------------------------------------------------------===//

class InstAnalyzer {
  const CodeInvDAGPatterns &CDP;
public:
  bool hasSideEffects;
  bool mayStore;
  bool mayLoad;
  bool isBitcast;
  bool isVariadic;

  InstAnalyzer(const CodeInvDAGPatterns &cdp)
    : CDP(cdp), hasSideEffects(false), mayStore(false), mayLoad(false),
      isBitcast(false), isVariadic(false) {}

  void Analyze(const InvTreePattern *Pat) {
    // Assume only the first tree is the pattern. The others are clobber nodes.
    AnalyzeNode(Pat->getTree(0));
  }

  void Analyze(const InvPatternToMatch *Pat) {
    AnalyzeNode(Pat->getDstPattern());
  }

private:
  bool IsNodeBitcast(const InvTreePatternNode *N) const {
    if (hasSideEffects || mayLoad || mayStore || isVariadic)
      return false;

    if (N->getNumChildren() != 2)
      return false;

    const InvTreePatternNode *N0 = N->getChild(0);
    if (!N0->isLeaf() || !isa<DefInit>(N0->getLeafVal()))
      return false;

    const InvTreePatternNode *N1 = N->getChild(1);
    if (N1->isLeaf())
      return false;
    if (N1->getNumChildren() != 1 || !N1->getChild(0)->isLeaf())
      return false;

    // FIXME: Figure out why we need this if statement
    if (N1->getRecord()->isSubClassOf("SDNode")) {
      const SDNodeInfo &OpInfo = CDP.getSDNodeInfo(N1->getRecord());
      if (OpInfo.getNumResults() != 1 || OpInfo.getNumOperands() != 1)
        return false; 
      return OpInfo.getEnumName() == "ISD::BITCAST";
    }
    
    return false;
  }

public:
  void AnalyzeNode(const InvTreePatternNode *N) {
    if (N->isLeaf()) {
      if (DefInit *DI = dyn_cast<DefInit>(N->getLeafVal())) {
        Record *LeafRec = DI->getDef();
        // Handle ComplexPattern leaves.
        if (LeafRec->isSubClassOf("ComplexPattern")) {
          const ComplexPattern &CP = CDP.getComplexPattern(LeafRec);
          if (CP.hasProperty(SDNPMayStore)) mayStore = true;
          if (CP.hasProperty(SDNPMayLoad)) mayLoad = true;
          if (CP.hasProperty(SDNPSideEffect)) hasSideEffects = true;
        }
      }
      return;
    }

    // Analyze children.
    for (unsigned i = 0, e = N->getNumChildren(); i != e; ++i)
      AnalyzeNode(N->getChild(i));

    // Ignore set nodes, which are not SDNodes.
    if (N->getRecord()->getName() == "set") {
      isBitcast = IsNodeBitcast(N);
      return;
    }

    errs() << N->getRecord()->getName() << "\n";
    std::vector<Record*> classes = N->getRecord()->getSuperClasses();
    errs() << "Superclasses: " << classes.size() << "\n";
    for (unsigned i = 0, e = classes.size(); i != e; ++i) {
      errs() << classes[i]->getName() << "\n";
    }

    //FIXME: Figure out why this if statement is needed.
    if (N->getRecord()->isSubClassOf("SDNode")) {
      // Get information about the SDNode for the operator.
      const SDNodeInfo &OpInfo = CDP.getSDNodeInfo(N->getRecord());

      // Notice properties of the node.
      if (OpInfo.hasProperty(SDNPMayStore)) mayStore = true;
      if (OpInfo.hasProperty(SDNPMayLoad)) mayLoad = true;
      if (OpInfo.hasProperty(SDNPSideEffect)) hasSideEffects = true;
      if (OpInfo.hasProperty(SDNPVariadic)) isVariadic = true;
    }

    if (const CodeGenIntrinsic *IntInfo = N->getIntrinsicInfo(CDP)) {
      // If this is an intrinsic, analyze it.
      if (IntInfo->ModRef >= CodeGenIntrinsic::ReadArgMem)
        mayLoad = true;// These may load memory.

      if (IntInfo->ModRef >= CodeGenIntrinsic::ReadWriteArgMem)
        mayStore = true;// Intrinsics that can write to memory are 'mayStore'.

      if (IntInfo->ModRef >= CodeGenIntrinsic::ReadWriteMem)
        // WriteMem intrinsics can have other strange effects.
        hasSideEffects = true;
    }
  }

};

static bool InferFromPattern(CodeGenInstruction &InstInfo,
                             const InstAnalyzer &PatInfo,
                             Record *PatDef) {
  bool Error = false;

  // Remember where InstInfo got its flags.
  if (InstInfo.hasUndefFlags())
      InstInfo.InferredFrom = PatDef;

  // Check explicitly set flags for consistency.
  if (InstInfo.hasSideEffects != PatInfo.hasSideEffects &&
      !InstInfo.hasSideEffects_Unset) {
    // Allow explicitly setting hasSideEffects = 1 on instructions, even when
    // the pattern has no side effects. That could be useful for div/rem
    // instructions that may trap.
    if (!InstInfo.hasSideEffects) {
      Error = true;
      PrintError(PatDef->getLoc(), "Pattern doesn't match hasSideEffects = " +
                 Twine(InstInfo.hasSideEffects));
    }
  }

  if (InstInfo.mayStore != PatInfo.mayStore && !InstInfo.mayStore_Unset) {
    Error = true;
    PrintError(PatDef->getLoc(), "Pattern doesn't match mayStore = " +
               Twine(InstInfo.mayStore));
  }

  if (InstInfo.mayLoad != PatInfo.mayLoad && !InstInfo.mayLoad_Unset) {
    // Allow explicitly setting mayLoad = 1, even when the pattern has no loads.
    // Some targets translate imediates to loads.
    if (!InstInfo.mayLoad) {
      Error = true;
      PrintError(PatDef->getLoc(), "Pattern doesn't match mayLoad = " +
                 Twine(InstInfo.mayLoad));
    }
  }

  // Transfer inferred flags.
  InstInfo.hasSideEffects |= PatInfo.hasSideEffects;
  InstInfo.mayStore |= PatInfo.mayStore;
  InstInfo.mayLoad |= PatInfo.mayLoad;

  // These flags are silently added without any verification.
  InstInfo.isBitcast |= PatInfo.isBitcast;

  // Don't infer isVariadic. This flag means something different on SDNodes and
  // instructions. For example, a CALL SDNode is variadic because it has the
  // call arguments as operands, but a CALL instruction is not variadic - it
  // has argument registers as implicit, not explicit uses.

  return Error;
}

/// Get all the instructions in a tree.
static void
getInstructionsInTree(const InvTreePatternNode *Tree, SmallVectorImpl<Record*> &Instrs) {
  if (Tree->isLeaf())
    return;
  if (Tree->getRecord()->isSubClassOf("Instruction"))
    Instrs.push_back(Tree->getRecord());
  for (unsigned i = 0, e = Tree->getNumChildren(); i != e; ++i)
    getInstructionsInTree(Tree->getChild(i), Instrs);
}


CodeInvDAGPatterns::CodeInvDAGPatterns(RecordKeeper &R) : Records(R), Target(R){
  Intrinsics = LoadIntrinsics(Records, false);
  TgtIntrinsics = LoadIntrinsics(Records, true);

  ParseInstructions();
  ParseSDNodes();
  ParseComplexPatterns();

  // Infer instruction flags.  For example, we can detect loads,
  // stores, and side effects in many cases by examining an
  // instruction's pattern.
  InferInstructionFlags();
}

CodeInvDAGPatterns::~CodeInvDAGPatterns() {
  // Do nothing right now.
}

void CodeInvDAGPatterns::ParseInstructions() {
  std::vector<Record*> Instrs = Records.getAllDerivedDefinitions("Instruction");
  Record* setRec = Records.getDef("set");

  for (unsigned i = 0, e = Instrs.size(); i != e; ++i) {
    // Generate instruction details
    Record* CurInst = Instrs[i]; 
    // CodeGenInstruction &InstrInfo = Target.getInstruction(CurInst);

    std::vector<InvTreePatternNode*> InstrChildren;

    // Build the InstrPat backwards, starting at the ins operand list.
    InvTreePattern *InstrPat = 
      new InvTreePattern(CurInst, CurInst->getValueAsDag("InOperandList"));
    InvTreePatternNode *IPRoot = InstrPat->Trees.at(0);

    // Change the "ins" record to the CurInst record.
    IPRoot->setRecord(CurInst);

    // If there are outputs, change the outs to a set, then add the outputs to
    // the output tree. Otherwise don't change the instruction pattern.
    InvTreePatternNode *IPOuts = InstrPat->ParseTreePattern(
      Instrs[i]->getValueAsDag("OutOperandList"), "");
    if (IPOuts->getNumChildren() != 0) {
      IPOuts->setRecord(setRec);
      IPOuts->addChild(IPRoot);
      InstrPat->Trees.at(0) = IPOuts;
    }
    InlinePatternFragments(*InstrPat);

    // outs() << "IN:  ";
    // InstrPat.print(outs());

    // Next is to generate Results patterns
    // Steps:
    // 1. Parse the pattern
    // 2. Link Results to the instruction outs list
    // 3. Link Inputs/Operands to the instruction ins list
    // 4. ?
    // 5. Profit

    // Every instruction has an inverse pattern, even if it's just an ops/res list
    // and the instruction itself (these would need to be handled explicitly)
    ListInit *InstResPat = 0;

    if (isa<ListInit>(Instrs[i]->getValueInit("Pattern")))
      InstResPat = Instrs[i]->getValueAsListInit("Pattern");
 

    if (InstResPat && InstResPat->getSize() != 0) {
      // Extract useful pattern info -- Remember we already know what the
      // ops/res are, so we need the corresponding opcode(s) and the mapping
      // of the ops/res that would have been matched during compiliation.
      InvTreePattern *ResultPat = new InvTreePattern(Instrs[i], InstResPat);
      InlinePatternFragments(*ResultPat);
      // outs() << "OUT: ";
      // test.print(outs());
      InvTreePatternNode *Src, *Dst; 
      Src = InstrPat->Trees.at(0);
      Dst = ResultPat->Trees.at(0);
      if (Src->getRecord()->getName() == "set") {
        // Set types for the instruction
        // FIXME: This won't work for register classes with more than one type
        // and the types should probably match between src and dst. Also, 
        InvTreePatternNode *Inst = Src->getChild(Src->getNumChildren() - 1);
        for (unsigned i = 0, e = Src->getNumChildren() - 1; i != e; ++i) {
          InvTreePatternNode *tmp = Src->getChild(i);
          Record *VTOp = NULL;
          if (tmp->isLeaf()) {
            DefInit *DI = dyn_cast<DefInit>(tmp->getLeafVal());
            if (DI == NULL) {
              errs() << "Could not convert init to definit for determining"
                     << " the type!!!\n";
              continue;
            }
            VTOp = DI->getDef();
          }
          else {
            VTOp = tmp->getRecord();
          }

          if(!Inst->UpdateNodeTypeFromInst(i, VTOp, Target)) {
            errs() << "Couldn't get type!\n";
          }
        }
        Src = Inst;
      }
      if (Dst->getRecord()->getName() == "set") {
        // Set types for the destination
        InvTreePatternNode *Inst = Dst->getChild(Dst->getNumChildren() - 1);
        for (unsigned i = 0, e = Dst->getNumChildren() - 1; i != e; ++i) {
          InvTreePatternNode *tmp = Dst->getChild(i);
          Record *VTOp = NULL;
          if (tmp->isLeaf()) {
            DefInit *DI = dyn_cast<DefInit>(tmp->getLeafVal());
            if (DI == NULL) {
              errs() << "Could not convert init to definit for determining"
                     << " the type!!!\n";
              continue;
            }
            VTOp = DI->getDef();
          }
          else {
            VTOp = tmp->getRecord();
          }

          if(!Inst->UpdateNodeTypeFromInst(i, VTOp, Target)) {
            errs() << "Couldn't get type!\n";
          }
        }
        Dst = Inst;
      }
      Patterns.push_back(new InvPatternToMatch(CurInst,
          Src, Dst, i));
      ResultPatterns.push_back(ResultPat);
    }
    // else {
    //   outs() << "OUT: NONE\n";
    // }

  }
}

Record *CodeInvDAGPatterns::getSDNodeNamed(const std::string &Name) const {
  Record *N = Records.getDef(Name);
  if (!N || !N->isSubClassOf("SDNode")) {
    errs() << "Error getting SDNode '" << Name << "'!\n";
    exit(1);
  }
  return N;
}

void CodeInvDAGPatterns::ParseSDNodes() {
  std::vector<Record*> Nodes = Records.getAllDerivedDefinitions("SDNode");

  while (!Nodes.empty()) {
    outs() << Nodes.back()->getName() << "\n";
    // Create a Record* and SDNodeInfo pair (second param calls SDNodeInfo constr)
    SDNodes.insert(std::make_pair(Nodes.back(), Nodes.back()));
    Nodes.pop_back();
  }

  // Get the builtin intrinsic nodes.
  intrinsic_void_sdnode     = getSDNodeNamed("intrinsic_void");
  intrinsic_w_chain_sdnode  = getSDNodeNamed("intrinsic_w_chain");
  intrinsic_wo_chain_sdnode = getSDNodeNamed("intrinsic_wo_chain");
}

void CodeInvDAGPatterns::ParseComplexPatterns() {
  std::vector<Record*> AMs = Records.getAllDerivedDefinitions("ComplexPattern");
  while (!AMs.empty()) {
    ComplexPatterns.insert(std::make_pair(AMs.back(), AMs.back()));
    AMs.pop_back();
  }
}

void CodeInvDAGPatterns::InferInstructionFlags() {
  const std::vector<const CodeGenInstruction*> &Instructions =
    Target.getInstructionsByEnumValue();

  // First try to infer flags from the primary instruction pattern, if any.
  SmallVector<CodeGenInstruction*, 8> Revisit;
  unsigned Errors = 0;
  for (unsigned i = 0, e = Instructions.size(); i != e; ++i) {
    CodeGenInstruction &InstInfo =
      const_cast<CodeGenInstruction &>(*Instructions[i]);

    // Treat neverHasSideEffects = 1 as the equivalent of hasSideEffects = 0.
    // This flag is obsolete and will be removed.
    if (InstInfo.neverHasSideEffects) {
      assert(!InstInfo.hasSideEffects);
      InstInfo.hasSideEffects_Unset = false;
    }

    // Get the primary instruction pattern.
    const InvTreePattern *Pattern = getResultPattern(InstInfo.TheDef);
    if (!Pattern) {
      if (InstInfo.hasUndefFlags())
        Revisit.push_back(&InstInfo);
      continue;
    }
    InstAnalyzer PatInfo(*this);
    Pattern->print(outs());
    // CURRENTLY DIES WHEN HITTING INTRINSICS!!!
    PatInfo.Analyze(Pattern);
    Errors += InferFromPattern(InstInfo, PatInfo, InstInfo.TheDef);
  }

  // Second, look for single-instruction patterns defined outside the
  // instruction.
  for (unsigned i = 0, e = Patterns.size(); i != e; ++i) {
    const InvPatternToMatch* PTM = Patterns[i];

    // We can only infer from single-instruction patterns, otherwise we won't
    // know which instruction should get the flags.
    SmallVector<Record*, 8> PatInstrs;
    getInstructionsInTree(PTM->getSrcPattern(), PatInstrs);
    if (PatInstrs.size() != 1)
      continue;

    // Get the single instruction.
    CodeGenInstruction &InstInfo = Target.getInstruction(PatInstrs.front());

    // Only infer properties from the first pattern. We'll verify the others.
    if (InstInfo.InferredFrom)
      continue;

    InstAnalyzer PatInfo(*this);
    PatInfo.Analyze(PTM);
    Errors += InferFromPattern(InstInfo, PatInfo, PTM->SrcRecord);
  }

  if (Errors) 
    PrintFatalError("pattern conflicts");

  // Revisit instructions with undefined flags and no pattern.
  if (Target.guessInstructionProperties()) {
    for (unsigned i = 0, e = Revisit.size(); i != e; ++i) {
      CodeGenInstruction &InstInfo = *Revisit[i];
      if (InstInfo.InferredFrom)
        continue;
      // The mayLoad and mayStore flags default to false.
      // Conservatively assume hasSideEffects if it wasn't explicit.
      if (InstInfo.hasSideEffects_Unset)
        InstInfo.hasSideEffects = true;
    }
    return;
  }

  // Complain about any flags that are still undefined.
  for (unsigned i = 0, e = Revisit.size(); i != e; ++i) {
    CodeGenInstruction &InstInfo = *Revisit[i];
    if (InstInfo.InferredFrom)
      continue;
    if (InstInfo.hasSideEffects_Unset)
      PrintError(InstInfo.TheDef->getLoc(),
                 "Can't infer hasSideEffects from patterns");
    if (InstInfo.mayStore_Unset)
      PrintError(InstInfo.TheDef->getLoc(),
                 "Can't infer mayStore from patterns");
    if (InstInfo.mayLoad_Unset)
      PrintError(InstInfo.TheDef->getLoc(),
                 "Can't infer mayLoad from patterns");
  }
}

/// Verify instruction flags against pattern node properties.
void CodeInvDAGPatterns::VerifyInstructionFlags() {
  unsigned Errors = 0;
  for (unsigned i = 0, e = Patterns.size(); i != e; ++i) {
    const InvPatternToMatch *PTM = Patterns[i];
    SmallVector<Record*, 8> Instrs;
    getInstructionsInTree(PTM->getDstPattern(), Instrs);
    if (Instrs.empty())
      continue;

    // Count the number of instructions with each flag set.
    unsigned NumSideEffects = 0;
    unsigned NumStores = 0;
    unsigned NumLoads = 0;
    for (unsigned i = 0, e = Instrs.size(); i != e; ++i) {
      const CodeGenInstruction &InstInfo = Target.getInstruction(Instrs[i]);
      NumSideEffects += InstInfo.hasSideEffects;
      NumStores += InstInfo.mayStore;
      NumLoads += InstInfo.mayLoad;
    }

    // Analyze the source pattern.
    InstAnalyzer PatInfo(*this);
    PatInfo.Analyze(PTM);

    // Collect error messages.
    SmallVector<std::string, 4> Msgs;

    // Check for missing flags in the output.
    // Permit extra flags for now at least.
    if (PatInfo.hasSideEffects && !NumSideEffects)
      Msgs.push_back("pattern has side effects, but hasSideEffects isn't set");

    // Don't verify store flags on instructions with side effects. At least for
    // intrinsics, side effects implies mayStore.
    if (!PatInfo.hasSideEffects && PatInfo.mayStore && !NumStores)
      Msgs.push_back("pattern may store, but mayStore isn't set");

    // Similarly, mayStore implies mayLoad on intrinsics.
    if (!PatInfo.mayStore && PatInfo.mayLoad && !NumLoads)
      Msgs.push_back("pattern may load, but mayLoad isn't set");

    // Print error messages.
    if (Msgs.empty())
      continue;
    ++Errors;

    for (unsigned i = 0, e = Msgs.size(); i != e; ++i)
      PrintError(PTM->SrcRecord->getLoc(), Twine(Msgs[i]) + " on the " +
                 (Instrs.size() == 1 ?
                  "instruction" : "output instructions"));
    // Provide the location of the relevant instruction definitions.
    for (unsigned i = 0, e = Instrs.size(); i != e; ++i) {
      if (Instrs[i] != PTM->SrcRecord)
        PrintError(Instrs[i]->getLoc(), "defined here");
      const CodeGenInstruction &InstInfo = Target.getInstruction(Instrs[i]);
      if (InstInfo.InferredFrom &&
          InstInfo.InferredFrom != InstInfo.TheDef &&
          InstInfo.InferredFrom != PTM->SrcRecord)
        PrintError(InstInfo.InferredFrom->getLoc(), "inferred from patttern");
    }
  }
  if (Errors)
    PrintFatalError("Errors in DAG patterns");
}

// InlinePatternFragments - Scans a tree pattern for fragments and inlines them 
// into place in the tree.
void CodeInvDAGPatterns::InlinePatternFragments(InvTreePattern &TP) {
  // if (TP.hasError())
  //   return 0;

  // DFS through the tree -- For each child
  //  - If PatFrag, inline it
  //  - Else recurse through it


  std::stack<InvTreePatternNode*> TPS;

  // Push each TP Tree on to the stack in reverse order (so the first tree is 
  // evaluated first)
  unsigned i = TP.Trees.size();
  while (i != 0) {
    --i;
    TPS.push(TP.Trees.at(i));
  }

  while (!TPS.empty()) {
    InvTreePatternNode* CurNode = TPS.top();
    TPS.pop();

    // Nothing to do if we are a leaf.
    if (CurNode->isLeaf()) continue;

    Record *NodeRec = CurNode->getRecord();
    assert(NodeRec && "NPE inside of InvTreePatternNode!");

    // Inline patfrag (if it is one)
    if (NodeRec->isSubClassOf("PatFrag")) {
      InvTreePattern *FragPat = getPatternFragment(NodeRec);
      // NOTE: LLVM Tablegen checks numargs are the same here...
      InvTreePatternNode *FragTree = FragPat->Trees.at(0);

      // Map the tree fragment operands to the nodes from CurNode.
      // Note that frags must have at least as many children as the curnode.
      unsigned NumOps = CurNode->getNumChildren();
      // NOTE: Removed for compat. with latest LLVM trunk.
      // assert (NumOps <= FragTree->getNumChildren() &&
      //   "Not enough children in fragment node!");
      // NOTE: There's no checking here...might need to fix in future.
      for (unsigned i = 0; i != NumOps && i != FragTree->getNumChildren();
           ++i) {
        // if the Fragment child has a name but the curnode child doesn't, then
        // use the fragments name
        InvTreePatternNode *CurChild = CurNode->getChild(i);
        if (CurChild->getName().empty()) {
          CurChild->setName(FragTree->getChild(i)->getName());
        }
        FragTree->setChild(i, CurChild);
      }

      *CurNode = *FragTree;
      // Evaluate this node again
      TPS.push(CurNode);
      continue;
    }

    // Add all children to the stack in reverse order.
    unsigned i = CurNode->getNumChildren();
    while (i != 0) {
      --i;
      TPS.push(CurNode->getChild(i));
    }
  }
}

// Check for the pattern fragment in the map, if it's not there then parse it
// using the rec object.
// This memoization strategy is used because we don't need to do extra stuff
// with patfrags.
InvTreePattern* CodeInvDAGPatterns::getPatternFragment(Record *Rec) {
  std::map<Record*, InvTreePattern*, LessRecordByID>::iterator
    Res = PatFrags.find(Rec);

  if (Res != PatFrags.end()) return Res->second;

  // Parse the Pattern Fragment, add to list, and return it
  DagInit *Tree = Rec->getValueAsDag("Fragment");
  InvTreePattern *Frag = new InvTreePattern(Rec, Tree);
  PatFrags[Rec] = Frag;

  // NOTE: TableGen does some argument sanity checks here that we are skipping
  // That might not be a good idea...

  return Frag;
}


} // end namespace llvm
