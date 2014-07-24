//===- Decompiler.h - Interface for Target Decompilers  ==========-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// The decompiler class provides a target independent base class for performing
// decompilation actions.
//
//===----------------------------------------------------------------------===//

#ifndef INVISELDAG_H
#define INVISELDAG_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

#include "CodeInv/IREmitter.h"

using namespace llvm;

namespace fracture {

class Decompiler;

/// ISelUpdater - helper class to handle updates of the instruction selection
/// graph.
class ISelUpdater : public SelectionDAG::DAGUpdateListener {
  SelectionDAG::allnodes_iterator &ISelPosition;
public:
  ISelUpdater(SelectionDAG &DAG, SelectionDAG::allnodes_iterator &isp)
    : SelectionDAG::DAGUpdateListener(DAG), ISelPosition(isp) {}

  /// NodeDeleted - Handle nodes deleted from the graph. If the node being
  /// deleted is the current ISelPosition node, update ISelPosition.
  ///
  virtual void NodeDeleted(SDNode *N, SDNode *E) {
    if (ISelPosition == SelectionDAG::allnodes_iterator(N))
      ++ISelPosition;
  }
};


struct MatchScope {
  /// FailIndex - If this match fails, this is the index to continue with.
  unsigned FailIndex;

  /// NodeStack - The node stack when the scope was formed.
  SmallVector<SDValue, 4> NodeStack;

  /// NumRecordedNodes - The number of recorded nodes when the scope was formed.
  unsigned NumRecordedNodes;

  /// NumMatchedMemRefs - The number of matched memref entries.
  unsigned NumMatchedMemRefs;

  /// InputChain/InputGlue - The current chain/glue
  SDValue InputChain, InputGlue;

  /// HasChainNodesMatched - True if the ChainNodesMatched list is non-empty.
  bool HasChainNodesMatched, HasGlueResultNodesMatched;
};

class InvISelDAG {
public:
  // Opcodes used by the DAG state machine:
  enum BuiltinOpcodes {
    OPC_Scope,
    OPC_RecordNode,
    OPC_RecordChild0, OPC_RecordChild1, OPC_RecordChild2, OPC_RecordChild3,
    OPC_RecordChild4, OPC_RecordChild5, OPC_RecordChild6, OPC_RecordChild7,
    OPC_RecordMemRef,
    OPC_CaptureGlueInput,
    OPC_MoveChild,
    OPC_MoveParent,
    OPC_CheckSame,
    OPC_CheckPatternPredicate,
    OPC_CheckPredicate,
    OPC_CheckOpcode,
    OPC_SwitchOpcode,
    OPC_CheckType,
    OPC_SwitchType,
    OPC_CheckChild0Type, OPC_CheckChild1Type, OPC_CheckChild2Type,
    OPC_CheckChild3Type, OPC_CheckChild4Type, OPC_CheckChild5Type,
    OPC_CheckChild6Type, OPC_CheckChild7Type,
    OPC_CheckInteger,
    OPC_CheckCondCode,
    OPC_CheckValueType,
    OPC_CheckComplexPat,
    OPC_CheckAndImm, OPC_CheckOrImm,
    OPC_CheckFoldableChainNode,

    OPC_EmitInteger,
    OPC_EmitRegister,
    OPC_EmitRegister2,
    OPC_EmitConvertToTarget,
    OPC_EmitMergeInputChains,
    OPC_EmitMergeInputChains1_0,
    OPC_EmitMergeInputChains1_1,
    OPC_EmitCopyToReg,
    OPC_EmitNodeXForm,
    OPC_EmitNode,
    OPC_MorphNodeTo,
    OPC_MarkGlueResults,
    OPC_CompleteMatch
  };

  enum {
    OPFL_None       = 0,  // Node has no chain or glue input and isn't variadic.
    OPFL_Chain      = 1,     // Node has a chain input.
    OPFL_GlueInput  = 2,     // Node has a glue input.
    OPFL_GlueOutput = 4,     // Node has a glue output.
    OPFL_MemRefs    = 8,     // Node gets accumulated MemRefs.
    OPFL_Variadic0  = 1<<4,  // Node is variadic, root has 0 fixed inputs.
    OPFL_Variadic1  = 2<<4,  // Node is variadic, root has 1 fixed inputs.
    OPFL_Variadic2  = 3<<4,  // Node is variadic, root has 2 fixed inputs.
    OPFL_Variadic3  = 4<<4,  // Node is variadic, root has 3 fixed inputs.
    OPFL_Variadic4  = 5<<4,  // Node is variadic, root has 4 fixed inputs.
    OPFL_Variadic5  = 6<<4,  // Node is variadic, root has 5 fixed inputs.
    OPFL_Variadic6  = 7<<4,  // Node is variadic, root has 6 fixed inputs.

    OPFL_VariadicInfo = OPFL_Variadic6
  };

  const TargetLowering *TLI;
  const TargetMachine *TM;
  SelectionDAG *CurDAG;
  MachineFunction *MF;
  const Decompiler        *Dec;

  InvISelDAG(const TargetMachine &TMC, 
    CodeGenOpt::Level OL = CodeGenOpt::Default,
    const Decompiler *TheDec = NULL);
 // {
 //    TLI = TMC.getTargetLowering();
 //    TM = &TMC;
 //  }
  virtual ~InvISelDAG() {
    // Creator is responsible for destroying TLI, TM, CurDAG, and MF.
  };

  virtual IREmitter* getEmitter(Decompiler *Dec, raw_ostream &InfoOut = nulls(), 
    raw_ostream &ErrOut = nulls())
  { return new IREmitter(Dec, InfoOut, ErrOut); }

  void UpdateChainsAndGlue(SDNode *NodeToMatch,  SDValue InputChain,
    const SmallVectorImpl<SDNode*> &ChainNodesMatched,
    SDValue InputGlue,
    const SmallVectorImpl<SDNode*> &GlueResultNodesMatched,
    bool isMorphNodeTo);

  SDNode* InvertCodeCommon(SDNode *NodeToMatch,
    const unsigned char *MatcherTable,
    unsigned TableSize);

  // NOTE: InvertCode is Implemented by tablegen
  virtual SDNode* InvertCode(SDNode *NodeToMatch) = 0;

  bool CheckAndMask(SDValue LHS, ConstantSDNode *RHS, int64_t DesiredMaskS) const;
  bool CheckOrMask(SDValue LHS, ConstantSDNode *RHS, int64_t DesiredMaskS) const;
  void SelectInlineAsmMemoryOperands(std::vector<SDValue> &Ops);

  void FixChainOp(SDNode *N);
  SDNode *MorphNode(SDNode *Node, unsigned TargetOpc, SDVTList VTList,
    const SDValue *Ops, unsigned NumOps, unsigned EmitNodeInfo);
  void CannotYetSelect(SDNode *N);
  void SetDAG(SelectionDAG *NewDAG) { 
    CurDAG = NewDAG; 
    MF = &CurDAG->getMachineFunction();
  }

  /// \brief Selects appropriate SDNode to convert Target Instruction to IR
  ///
  /// This function is implemented by each target and should be the entry point
  /// to that target's fracture-tblgen InvertCode function. It is also the 
  /// function where the target author should put hacks and special cases
  /// which cannot be expressed in the tablegen system.
  ///
  /// Typical Usage:
  /// \code
  ///   MachineBasicBlock B = ...
  ///   foreach (SDNode S in B) 
  ///     S = Transmogrify(S);
  /// \code
  /// 
  /// \param N a MachineSDNode that will be converted.
  ///
  /// \retrurns a regular SDnode which corresponded to the MachineSDNode.
  virtual SDNode* Transmogrify(SDNode *N) = 0;

  virtual bool CheckComplexPattern(SDNode *Root, SDNode *Parent, SDValue N,
    unsigned PatternNo,
    SmallVectorImpl<std::pair<SDValue, SDNode*> > &Result, 
    unsigned StartNo = 0) {
    llvm_unreachable("Tblgen should generate the implementation of this!");
  }
};

/// \brief Selects the correct InvISelDAG engine for the Target.
/// 
/// Returns null if there is no InvISelDAG for the given Target.
///
/// NOTE: This function must be updated when you add a new engine!
/// 
/// Typical usage:
/// \code
///   Target T = ...
///   ...
///   InvISelDAG *MyDec = getTargetInvISelDAG(T);
/// \endcode
///
/// \param T a pointer to the target.
///
/// \returns InvISelDAG engine for Target, or null. 
InvISelDAG* getTargetInvISelDAG(const TargetMachine *T, const Decompiler *TheDec = NULL);

} // end fracture namespace

#endif /* INVISELDAG_H */
