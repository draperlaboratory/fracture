//===- CodeInvDAGPatterns.h - Inverse Code Patterns ==============-*- C++ -*-=//
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
//===----------------------------------------------------------------------===//

#ifndef CODEINV_DAGPATTERNS_H
#define CODEINV_DAGPATTERNS_H

#include "CodeGenIntrinsics.h"
#include "CodeGenTarget.h"
#include "SDNodeInfo.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/Error.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Debug.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <stack>

namespace llvm {


class CodeInvDAGPatterns;

// May not be necessary to have this class as what we need to track is not as
// complicated, but modeling it anyway.
class InvTreePatternNode {
  Record *TheRec;
  Init *Val;
  std::string Name;
  std::vector<InvTreePatternNode*> Children;

  /// The type of each node result.  Before and during type inference, each
  /// result may be a set of possible types.  After (successful) type inference,
  /// each is a single concrete type.
  SmallVector<MVT::SimpleValueType, 1> Types;

public:
  InvTreePatternNode(Record *Rec, const std::vector<InvTreePatternNode*> &Ch)
    : TheRec(Rec), Val(0), Children(Ch) { }
  InvTreePatternNode(Init *val) : TheRec(0), Val(val) { }

  const std::string &getName() const { return Name; }
  void setName(StringRef N) { Name.assign(N.begin(), N.end()); }

  bool isLeaf() const { return (Val != 0); }
  Init* getLeafVal() const { return Val; }

  Record *getRecord() const { assert(!isLeaf()); return TheRec; }
  void setRecord(Record* NewRec) { TheRec = NewRec; }

  unsigned getNumChildren() const { return Children.size(); }
  InvTreePatternNode* getChild(unsigned N) const { return Children[N]; }
  void setChild(unsigned N, InvTreePatternNode *Val) { Children[N] = Val; }
  void addChild(InvTreePatternNode *Val) { Children.push_back(Val); }

  /// getComplexPatternInfo - If this node corresponds to a ComplexPattern,
  /// return the ComplexPattern information, otherwise return null.
  const ComplexPattern *getComplexPatternInfo(const CodeInvDAGPatterns &CGP) const;

  /// NodeHasProperty - Return true if this node has the specified property.
  bool NodeHasProperty(SDNP Property, const CodeInvDAGPatterns &CGP) const;

  /// getIntrinsicInfo - If this node corresponds to an intrinsic, return the
  /// CodeGenIntrinsic information for it, otherwise return a null pointer.
  const CodeGenIntrinsic *getIntrinsicInfo(const CodeInvDAGPatterns &CDP) const;

  // Update node type with types inferred from an instruction operand or result
  // def from the ins/outs lists.
  // Return true if the type changed.
  bool UpdateNodeTypeFromInst(unsigned ResNo, Record *Operand, CodeGenTarget &Tgt);

  // Type accessors.
  unsigned getNumTypes() const { return Types.size(); }
  MVT::SimpleValueType getType(unsigned ResNo) const {
    return Types[ResNo];
  }
  void setType(unsigned ResNo, const MVT::SimpleValueType &T) { 
    // FIXME: kind of a hack to have this if stmt here...
    if (Types.size() < (ResNo + 1)) Types.resize(ResNo+1);
    Types[ResNo] = T; 
  }

  InvTreePatternNode& operator=(const InvTreePatternNode &RHS);

  void print(raw_ostream &OS) const;
};

inline raw_ostream &operator<<(raw_ostream &OS, const InvTreePatternNode &TPN) {
  TPN.print(OS);
  return OS;
}

class InvTreePattern {

public:
  Record *Rec;
  std::vector<InvTreePatternNode*> Trees;

  InvTreePattern(Record *TheRec, ListInit *RawPat);
  InvTreePattern(Record *TheRec, DagInit *Pat);
  InvTreePattern(Record *TheRec, InvTreePatternNode *Pat);
  InvTreePatternNode *ParseTreePattern(Init *TheInit, StringRef OpName);
  InvTreePatternNode *getTree(unsigned TreeNo) const { 
    assert(TreeNo < Trees.size() && "InvTreePattern out of bounds error!");
    return Trees[TreeNo]; 
  };

  Record* getRecord() const { return Rec; }

  void error(const std::string &Msg) { 
    PrintError(Rec->getLoc(), "In " + Rec->getName() + ": " + Msg);
  };
  void print(raw_ostream &OS) const;
};



// InvPatternToMatch - Used by CodeInfDAGPatterns to keep tabs of patterns
// processed to produce inverse isel. 
//
// NOTE: We are dealing with what is on an actual Target and not trying to find 
// and optimal pattern to a specificl subtarget. This means we don't need preds,
// complexity, or destination registers.
class InvPatternToMatch {
public:
  InvPatternToMatch(Record *srcrecord, 
                    InvTreePatternNode *src, InvTreePatternNode *dst, 
                    unsigned uid) 
    : SrcRecord(srcrecord), SrcPattern(src), DstPattern(dst), ID(uid) {}

  Record          *SrcRecord; // Originating Record for this pattern
  InvTreePatternNode *SrcPattern;
  InvTreePatternNode *DstPattern;
  unsigned        ID;

  const InvTreePatternNode* getSrcPattern() const { return SrcPattern; }
  const InvTreePatternNode* getDstPattern() const { return DstPattern; }

};

class CodeInvDAGPatterns {
  RecordKeeper &Records;
  CodeGenTarget Target;

  std::map<Record*, InvTreePattern*, LessRecordByID> PatFrags;
  std::map<Record*, SDNodeInfo, LessRecordByID> SDNodes;

  std::vector<const InvPatternToMatch*> Patterns;
  std::vector<const InvTreePattern*> ResultPatterns;
  std::map<Record*, ComplexPattern, LessRecordByID> ComplexPatterns;

  std::vector<CodeGenIntrinsic> Intrinsics;
  std::vector<CodeGenIntrinsic> TgtIntrinsics;

  // Specific SDNode definitions:
  Record *intrinsic_void_sdnode;
  Record *intrinsic_w_chain_sdnode, *intrinsic_wo_chain_sdnode;
public:
  CodeInvDAGPatterns(RecordKeeper &R);
  ~CodeInvDAGPatterns();
  void ParseInstructions();
  void ParseSDNodes();
  void ParseComplexPatterns();
  void InferInstructionFlags();
  void VerifyInstructionFlags();
  Record *getSDNodeNamed(const std::string &Name) const;
  void InlinePatternFragments(InvTreePattern &TP);
  InvTreePattern* getPatternFragment(Record* Rec);
  const CodeGenTarget &getTargetInfo() const { return Target; }
  const std::vector<const InvPatternToMatch*> 
    getPatterns() const { return Patterns; }
  const SDNodeInfo& getSDNodeInfo(Record* Rec) const {
    assert(SDNodes.count(Rec) && "Unknown SDNode!");
    return SDNodes.find(Rec)->second;
  }
  std::vector<const ComplexPattern*> getComplexPatterns() const {
    std::vector<const ComplexPattern*> CP; 
    for (std::map<Record*, ComplexPattern, LessRecordByID>::const_iterator 
           I = ComplexPatterns.begin(), E = ComplexPatterns.end(); 
         I != E; ++I) {
      CP.push_back(&(I->second));
    }
    return CP;
  }
  const ComplexPattern &getComplexPattern(Record *R) const {
    assert(ComplexPatterns.count(R) && "Unknown addressing mode!");
    return ComplexPatterns.find(R)->second;
  }
  const CodeGenInstruction& getTgtNodeInfo(Record* Rec) const {
    const std::vector<const CodeGenInstruction*> &Instrs = 
      Target.getInstructionsByEnumValue();
    for (unsigned i = 0, e = Instrs.size(); i != e; ++i) {
      if (Instrs[i]->TheDef == Rec) {
        return *Instrs[i];
      }
    }
    llvm_unreachable("Unknown target node instruction!");
  }
  const InvTreePattern* getResultPattern(Record *R) const {
    for (unsigned i = 0, e = ResultPatterns.size(); i != e; ++i) {
      if (ResultPatterns[i]->getRecord() == R) 
        return ResultPatterns[i];
    }
    return NULL;
  }

  const CodeGenIntrinsic &getIntrinsic(Record *R) const {
    for (unsigned i = 0, e = Intrinsics.size(); i != e; ++i)
      if (Intrinsics[i].TheDef == R) return Intrinsics[i];
    for (unsigned i = 0, e = TgtIntrinsics.size(); i != e; ++i)
      if (TgtIntrinsics[i].TheDef == R) return TgtIntrinsics[i];
    llvm_unreachable("Unknown intrinsic!");
  }

  const CodeGenIntrinsic &getIntrinsicInfo(unsigned IID) const {
    if (IID-1 < Intrinsics.size())
      return Intrinsics[IID-1];
    if (IID-Intrinsics.size()-1 < TgtIntrinsics.size())
      return TgtIntrinsics[IID-Intrinsics.size()-1];
    llvm_unreachable("Bad intrinsic ID!");
  }

  unsigned getIntrinsicID(Record *R) const {
    for (unsigned i = 0, e = Intrinsics.size(); i != e; ++i)
      if (Intrinsics[i].TheDef == R) return i;
    for (unsigned i = 0, e = TgtIntrinsics.size(); i != e; ++i)
      if (TgtIntrinsics[i].TheDef == R) return i + Intrinsics.size();
    llvm_unreachable("Unknown intrinsic!");
  }

  Record *get_intrinsic_void_sdnode() const {
    return intrinsic_void_sdnode;
  }
  Record *get_intrinsic_w_chain_sdnode() const {
    return intrinsic_w_chain_sdnode;
  }
  Record *get_intrinsic_wo_chain_sdnode() const {
    return intrinsic_wo_chain_sdnode;
  }
};  

} // end namespace llvm

#endif
