//===- Decompiler.cpp - Interface for Target Inv Instr Selectors --*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// This file modifies a file originally found in LLVM.
//
//===----------------------------------------------------------------------===//
//
// This class provides a target independent base class for performing
// inverse instruction selection actions.
//
//===----------------------------------------------------------------------===//

#include "CodeInv/InvISelDAG.h"
#include "Target/ARM/ARMInvISelDAG.h"
#include "Target/X86/X86InvISelDAG.h"
#include "Target/PowerPC/PPCInvISelDAG.h"
#include "llvm/IR/DataLayout.h"

//#include "StringRef.h"

#define DEBUG_TYPE "inviseldag"

namespace fracture {

// NOTE: This needs to be generalized to select InvISelDAG's based on
// TargetMachine Types.
InvISelDAG* getTargetInvISelDAG(const TargetMachine *T, const Decompiler *TheDec) {
  //Prob needs to be conditional...
	StringRef triple = T->getTargetTriple();
	outs() << "Triple: " << triple.str().c_str() << "\n";
	StringRef cpu = T->getTargetCPU();
	outs() << "CPU: " << cpu.str().c_str() << "\n";

	InvISelDAG *res = NULL;
	if(triple.str().find("arm") == 0){
		res = new ARMInvISelDAG(*T, CodeGenOpt::Default, TheDec);
	} else if(triple.str().find("i386") == 0 || triple.str().find("x86_64") == 0) {
		res = new X86InvISelDAG(*T, CodeGenOpt::Default, TheDec);
	} else if(triple.str().find("powerpc64") == 0) {
	  res = new PPCInvISelDAG(*T, CodeGenOpt::Default, TheDec);
	} else {
		outs() << "Decompiler doesn't support: " << triple.str().c_str() << cpu.str().c_str() << "\n";
	}
  return res;
}

InvISelDAG::InvISelDAG(const TargetMachine &TMC,
  CodeGenOpt::Level OL, const Decompiler *TheDec) {
  TLI = TMC.getSubtargetImpl()->getTargetLowering();
  TM = &TMC;
  Dec = TheDec;
}


// STATISTIC(NumFastIselFailures, "Number of instructions fast isel failed on");
// STATISTIC(NumFastIselSuccess, "Number of instructions fast isel selected");
// STATISTIC(NumFastIselBlocks, "Number of blocks selected entirely by fast isel");
// STATISTIC(NumDAGBlocks, "Number of blocks selected using DAG");
STATISTIC(NumDAGIselRetries,"Number of times dag isel has to try another path");


/// GetVBR - decode a vbr encoding whose top bit is set.
LLVM_ATTRIBUTE_ALWAYS_INLINE static uint64_t
GetVBR(uint64_t Val, const unsigned char *MatcherTable, unsigned &Idx) {
  assert(Val >= 128 && "Not a VBR");
  Val &= 127;  // Remove first vbr bit.

  unsigned Shift = 7;
  uint64_t NextBits;
  do {
    NextBits = MatcherTable[Idx++];
    Val |= (NextBits&127) << Shift;
    Shift += 7;
  } while (NextBits & 128);

  return Val;
}

/// CheckSame - Implements OP_CheckSame.
LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckSame(const unsigned char *MatcherTable, unsigned &MatcherIndex,
          SDValue N,
          const SmallVectorImpl<std::pair<SDValue, SDNode*> > &RecordedNodes) {
  // Accept if it is exactly the same as a previously recorded node.
  unsigned RecNo = MatcherTable[MatcherIndex++];
  assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
  if (RecNo >= RecordedNodes.size()) return false;
  return N == RecordedNodes[RecNo].first;
}

/// CheckPatternPredicate - Implements OP_CheckPatternPredicate.
LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckPatternPredicate(const unsigned char *MatcherTable, unsigned &MatcherIndex,
                      const InvISelDAG &SDISel) {
  // return SDISel.CheckPatternPredicate(MatcherTable[MatcherIndex++]);
  return true;
}

/// CheckNodePredicate - Implements OP_CheckNodePredicate.
LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckNodePredicate(const unsigned char *MatcherTable, unsigned &MatcherIndex,
                   const InvISelDAG &SDISel, SDNode *N) {
  // return SDISel.CheckNodePredicate(N, MatcherTable[MatcherIndex++]);
  return true;
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckOpcode(const unsigned char *MatcherTable, unsigned &MatcherIndex,
            SDNode *N) {
  uint16_t Opc = MatcherTable[MatcherIndex++];
  Opc |= (unsigned short)MatcherTable[MatcherIndex++] << 8;
  uint16_t TgtOpc = N->getOpcode();
  if (N->isMachineOpcode()) {
    TgtOpc = ~TgtOpc;
  }
  if (TgtOpc == Opc) {
    DEBUG(errs() << "Opcode MATCH: " << Opc << ", " << TgtOpc << "\n");
  } else {
    DEBUG(errs() << "Opcode MISMATCH: " << Opc << ", " << TgtOpc << "\n");
  }
  return TgtOpc == Opc;
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckType(const unsigned char *MatcherTable, unsigned &MatcherIndex,
          SDValue N, const TargetLowering &TLI) {
  MVT::SimpleValueType VT = (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
  if (N.getValueType() == VT) return true;

  // Handle the case when VT is iPTR.
  return VT == MVT::iPTR && N.getValueType() == TLI.getPointerTy();
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckChildType(const unsigned char *MatcherTable, unsigned &MatcherIndex,
               SDValue N, const TargetLowering &TLI,
               unsigned ChildNo) {
  if (ChildNo >= N.getNumOperands())
    return false;  // Match fails if out of range child #.
  return CheckType(MatcherTable, MatcherIndex, N.getOperand(ChildNo), TLI);
}


LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckCondCode(const unsigned char *MatcherTable, unsigned &MatcherIndex,
              SDValue N) {
  return cast<CondCodeSDNode>(N)->get() ==
      (ISD::CondCode)MatcherTable[MatcherIndex++];
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckValueType(const unsigned char *MatcherTable, unsigned &MatcherIndex,
               SDValue N, const TargetLowering &TLI) {
  MVT::SimpleValueType VT = (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
  if (cast<VTSDNode>(N)->getVT() == VT)
    return true;

  // Handle the case when VT is iPTR.
  return VT == MVT::iPTR && cast<VTSDNode>(N)->getVT() == TLI.getPointerTy();
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckInteger(const unsigned char *MatcherTable, unsigned &MatcherIndex,
             SDValue N) {
  int64_t Val = MatcherTable[MatcherIndex++];
  if (Val & 128)
    Val = GetVBR(Val, MatcherTable, MatcherIndex);

  ConstantSDNode *C = dyn_cast<ConstantSDNode>(N);
  return C != 0 && C->getSExtValue() == Val;
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckAndImm(const unsigned char *MatcherTable, unsigned &MatcherIndex,
            SDValue N, const InvISelDAG &SDISel) {
  int64_t Val = MatcherTable[MatcherIndex++];
  if (Val & 128)
    Val = GetVBR(Val, MatcherTable, MatcherIndex);

  if (N->getOpcode() != ISD::AND) return false;

  ConstantSDNode *C = dyn_cast<ConstantSDNode>(N->getOperand(1));
  return C != 0 && SDISel.CheckAndMask(N.getOperand(0), C, Val);
}

LLVM_ATTRIBUTE_ALWAYS_INLINE static bool
CheckOrImm(const unsigned char *MatcherTable, unsigned &MatcherIndex,
           SDValue N, const InvISelDAG &SDISel) {
  int64_t Val = MatcherTable[MatcherIndex++];
  if (Val & 128)
    Val = GetVBR(Val, MatcherTable, MatcherIndex);

  if (N->getOpcode() != ISD::OR) return false;

  ConstantSDNode *C = dyn_cast<ConstantSDNode>(N->getOperand(1));
  return C != 0 && SDISel.CheckOrMask(N.getOperand(0), C, Val);
}


/// IsPredicateKnownToFail - If we know how and can do so without pushing a
/// scope, evaluate the current node.  If the current predicate is known to
/// fail, set Result=true and return anything.  If the current predicate is
/// known to pass, set Result=false and return the MatcherIndex to continue
/// with.  If the current predicate is unknown, set Result=false and return the
/// MatcherIndex to continue with.
static unsigned IsPredicateKnownToFail(const unsigned char *Table,
                                       unsigned Index, SDValue N,
                                       bool &Result,
                                       const InvISelDAG &SDISel,
                 SmallVectorImpl<std::pair<SDValue, SDNode*> > &RecordedNodes) {
  switch (Table[Index++]) {
  default:
    Result = false;
    return Index-1;  // Could not evaluate this predicate.
  case InvISelDAG::OPC_CheckSame:
    Result = !CheckSame(Table, Index, N, RecordedNodes);
    return Index;
  case InvISelDAG::OPC_CheckPatternPredicate:
    Result = !CheckPatternPredicate(Table, Index, SDISel);
    return Index;
  case InvISelDAG::OPC_CheckPredicate:
    Result = !CheckNodePredicate(Table, Index, SDISel, N.getNode());
    return Index;
  case InvISelDAG::OPC_CheckOpcode:
    Result = !CheckOpcode(Table, Index, N.getNode());
    return Index;
  case InvISelDAG::OPC_CheckType:
    Result = !fracture::CheckType(Table, Index, N, *SDISel.TLI);
    return Index;
  case InvISelDAG::OPC_CheckChild0Type:
  case InvISelDAG::OPC_CheckChild1Type:
  case InvISelDAG::OPC_CheckChild2Type:
  case InvISelDAG::OPC_CheckChild3Type:
  case InvISelDAG::OPC_CheckChild4Type:
  case InvISelDAG::OPC_CheckChild5Type:
  case InvISelDAG::OPC_CheckChild6Type:
  case InvISelDAG::OPC_CheckChild7Type:
    Result = !CheckChildType(Table, Index, N, *SDISel.TLI,
                        Table[Index-1] - InvISelDAG::OPC_CheckChild0Type);
    return Index;
  case InvISelDAG::OPC_CheckCondCode:
    Result = !CheckCondCode(Table, Index, N);
    return Index;
  case InvISelDAG::OPC_CheckValueType:
    Result = !CheckValueType(Table, Index, N, *SDISel.TLI);
    return Index;
  case InvISelDAG::OPC_CheckInteger:
    Result = !CheckInteger(Table, Index, N);
    return Index;
  case InvISelDAG::OPC_CheckAndImm:
    Result = !CheckAndImm(Table, Index, N, SDISel);
    return Index;
  case InvISelDAG::OPC_CheckOrImm:
    Result = !CheckOrImm(Table, Index, N, SDISel);
    return Index;
  }
}

/// UpdateChainsAndGlue - When a match is complete, this method updates uses of
/// interior glue and chain results to use the new glue and chain results.
void InvISelDAG::
UpdateChainsAndGlue(SDNode *NodeToMatch, SDValue InputChain,
                    const SmallVectorImpl<SDNode*> &ChainNodesMatched,
                    SDValue InputGlue,
                    const SmallVectorImpl<SDNode*> &GlueResultNodesMatched,
                    bool isMorphNodeTo) {
  SmallVector<SDNode*, 4> NowDeadNodes;

  // Now that all the normal results are replaced, we replace the chain and
  // glue results if present.
  if (!ChainNodesMatched.empty()) {
    assert(InputChain.getNode() != 0 &&
           "Matched input chains but didn't produce a chain");
    // Loop over all of the nodes we matched that produced a chain result.
    // Replace all the chain results with the final chain we ended up with.
    for (unsigned i = 0, e = ChainNodesMatched.size(); i != e; ++i) {
      SDNode *ChainNode = ChainNodesMatched[i];

      // If this node was already deleted, don't look at it.
      if (ChainNode->getOpcode() == ISD::DELETED_NODE)
        continue;

      // Don't replace the results of the root node if we're doing a
      // MorphNodeTo.
      if (ChainNode == NodeToMatch && isMorphNodeTo)
        continue;

      SDValue ChainVal = SDValue(ChainNode, ChainNode->getNumValues()-1);
      if (ChainVal.getValueType() == MVT::Glue)
        ChainVal = ChainVal.getValue(ChainVal->getNumValues()-2);
      assert(ChainVal.getValueType() == MVT::Other && "Not a chain?");
      CurDAG->ReplaceAllUsesOfValueWith(ChainVal, InputChain);

      // If the node became dead and we haven't already seen it, delete it.
      if (ChainNode->use_empty() &&
          !std::count(NowDeadNodes.begin(), NowDeadNodes.end(), ChainNode))
        NowDeadNodes.push_back(ChainNode);
    }
  }

  // If the result produces glue, update any glue results in the matched
  // pattern with the glue result.
  if (InputGlue.getNode() != 0) {
    // Handle any interior nodes explicitly marked.
    for (unsigned i = 0, e = GlueResultNodesMatched.size(); i != e; ++i) {
      SDNode *FRN = GlueResultNodesMatched[i];

      // If this node was already deleted, don't look at it.
      if (FRN->getOpcode() == ISD::DELETED_NODE)
        continue;

      assert(FRN->getValueType(FRN->getNumValues()-1) == MVT::Glue &&
             "Doesn't have a glue result");
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(FRN, FRN->getNumValues()-1),
                                        InputGlue);

      // If the node became dead and we haven't already seen it, delete it.
      if (FRN->use_empty() &&
          !std::count(NowDeadNodes.begin(), NowDeadNodes.end(), FRN))
        NowDeadNodes.push_back(FRN);
    }
  }

  if (!NowDeadNodes.empty())
    CurDAG->RemoveDeadNodes(NowDeadNodes);

  DEBUG(dbgs() << "ISEL: Match complete!\n");
}

enum ChainResult {
  CR_Simple,
  CR_InducesCycle,
  CR_LeadsToInteriorNode
};

/// WalkChainUsers - Walk down the users of the specified chained node that is
/// part of the pattern we're matching, looking at all of the users we find.
/// This determines whether something is an interior node, whether we have a
/// non-pattern node in between two pattern nodes (which prevent folding because
/// it would induce a cycle) and whether we have a TokenFactor node sandwiched
/// between pattern nodes (in which case the TF becomes part of the pattern).
///
/// The walk we do here is guaranteed to be small because we quickly get down to
/// already selected nodes "below" us.
static ChainResult
WalkChainUsers(const SDNode *ChainedNode,
               SmallVectorImpl<SDNode*> &ChainedNodesInPattern,
               SmallVectorImpl<SDNode*> &InteriorChainedNodes) {
  ChainResult Result = CR_Simple;

  for (SDNode::use_iterator UI = ChainedNode->use_begin(),
         E = ChainedNode->use_end(); UI != E; ++UI) {
    // Make sure the use is of the chain, not some other value we produce.
    if (UI.getUse().getValueType() != MVT::Other) continue;

    SDNode *User = *UI;

    // If we see an already-selected machine node, then we've gone beyond the
    // pattern that we're selecting down into the already selected chunk of the
    // DAG.
    // Fracture Fixed:
    // In our case, if the node is not a machine opcode than it has been selected!
    if (!User->isMachineOpcode() ||
        User->getOpcode() == ISD::HANDLENODE)  // Root of the graph.
      continue;

    unsigned UserOpcode = User->getOpcode();
    if (UserOpcode == ISD::CopyToReg ||
        UserOpcode == ISD::CopyFromReg ||
        UserOpcode == ISD::INLINEASM ||
        UserOpcode == ISD::EH_LABEL ||
        UserOpcode == ISD::LIFETIME_START ||
        UserOpcode == ISD::LIFETIME_END) {
      // If their node ID got reset to -1 then they've already been selected.
      // Treat them like a MachineOpcode.
      if (User->getNodeId() == -1)
        continue;
    }

    // If we have a TokenFactor, we handle it specially.
    if (User->getOpcode() != ISD::TokenFactor) {
      // If the node isn't a token factor and isn't part of our pattern, then it
      // must be a random chained node in between two nodes we're selecting.
      // This happens when we have something like:
      //   x = load ptr
      //   call
      //   y = x+4
      //   store y -> ptr
      // Because we structurally match the load/store as a read/modify/write,
      // but the call is chained between them.  We cannot fold in this case
      // because it would induce a cycle in the graph.
      if (!std::count(ChainedNodesInPattern.begin(),
                      ChainedNodesInPattern.end(), User))
        return CR_InducesCycle;

      // Otherwise we found a node that is part of our pattern.  For example in:
      //   x = load ptr
      //   y = x+4
      //   store y -> ptr
      // This would happen when we're scanning down from the load and see the
      // store as a user.  Record that there is a use of ChainedNode that is
      // part of the pattern and keep scanning uses.
      Result = CR_LeadsToInteriorNode;
      InteriorChainedNodes.push_back(User);
      continue;
    }

    // If we found a TokenFactor, there are two cases to consider: first if the
    // TokenFactor is just hanging "below" the pattern we're matching (i.e. no
    // uses of the TF are in our pattern) we just want to ignore it.  Second,
    // the TokenFactor can be sandwiched in between two chained nodes, like so:
    //     [Load chain]
    //         ^
    //         |
    //       [Load]
    //       ^    ^
    //       |    \                    DAG's like cheese
    //      /       \                       do you?
    //     /         |
    // [TokenFactor] [Op]
    //     ^          ^
    //     |          |
    //      \        /
    //       \      /
    //       [Store]
    //
    // In this case, the TokenFactor becomes part of our match and we rewrite it
    // as a new TokenFactor.
    //
    // To distinguish these two cases, do a recursive walk down the uses.
    switch (WalkChainUsers(User, ChainedNodesInPattern, InteriorChainedNodes)) {
    case CR_Simple:
      // If the uses of the TokenFactor are just already-selected nodes, ignore
      // it, it is "below" our pattern.
      continue;
    case CR_InducesCycle:
      // If the uses of the TokenFactor lead to nodes that are not part of our
      // pattern that are not selected, folding would turn this into a cycle,
      // bail out now.
      return CR_InducesCycle;
    case CR_LeadsToInteriorNode:
      break;  // Otherwise, keep processing.
    }

    // Okay, we know we're in the interesting interior case.  The TokenFactor
    // is now going to be considered part of the pattern so that we rewrite its
    // uses (it may have uses that are not part of the pattern) with the
    // ultimate chain result of the generated code.  We will also add its chain
    // inputs as inputs to the ultimate TokenFactor we create.
    Result = CR_LeadsToInteriorNode;
    ChainedNodesInPattern.push_back(User);
    InteriorChainedNodes.push_back(User);
    continue;
  }

  return Result;
}

/// HandleMergeInputChains - This implements the OPC_EmitMergeInputChains
/// operation for when the pattern matched at least one node with a chains.  The
/// input vector contains a list of all of the chained nodes that we match.  We
/// must determine if this is a valid thing to cover (i.e. matching it won't
/// induce cycles in the DAG) and if so, creating a TokenFactor node. that will
/// be used as the input node chain for the generated nodes.
static SDValue
HandleMergeInputChains(SmallVectorImpl<SDNode*> &ChainNodesMatched,
                       SelectionDAG *CurDAG) {
  // Walk all of the chained nodes we've matched, recursively scanning down the
  // users of the chain result. This adds any TokenFactor nodes that are caught
  // in between chained nodes to the chained and interior nodes list.
  SmallVector<SDNode*, 3> InteriorChainedNodes;
  for (unsigned i = 0, e = ChainNodesMatched.size(); i != e; ++i) {
    if (WalkChainUsers(ChainNodesMatched[i], ChainNodesMatched,
                       InteriorChainedNodes) == CR_InducesCycle)
      return SDValue(); // Would induce a cycle.
  }

  // Okay, we have walked all the matched nodes and collected TokenFactor nodes
  // that we are interested in.  Form our input TokenFactor node.
  SmallVector<SDValue, 3> InputChains;
  for (unsigned i = 0, e = ChainNodesMatched.size(); i != e; ++i) {
    // Add the input chain of this node to the InputChains list (which will be
    // the operands of the generated TokenFactor) if it's not an interior node.
    SDNode *N = ChainNodesMatched[i];
    if (N->getOpcode() != ISD::TokenFactor) {
      if (std::count(InteriorChainedNodes.begin(),InteriorChainedNodes.end(),N))
        continue;

      // Otherwise, add the input chain.
      SDValue InChain = ChainNodesMatched[i]->getOperand(0);
      assert(InChain.getValueType() == MVT::Other && "Not a chain");
      InputChains.push_back(InChain);
      continue;
    }

    // If we have a token factor, we want to add all inputs of the token factor
    // that are not part of the pattern we're matching.
    for (unsigned op = 0, e = N->getNumOperands(); op != e; ++op) {
      if (!std::count(ChainNodesMatched.begin(), ChainNodesMatched.end(),
                      N->getOperand(op).getNode()))
        InputChains.push_back(N->getOperand(op));
    }
  }

  SDValue Res;
  if (InputChains.size() == 1)
    return InputChains[0];
  return CurDAG->getNode(ISD::TokenFactor, SDLoc(ChainNodesMatched[0]),
                         MVT::Other, InputChains);
}

/// Moves Op[0] to Op[Op.size()-1]. This is done for certain load/store operands
/// during inverse DAG Selection.
void InvISelDAG::FixChainOp(SDNode *N) {
  SDValue Chain = N->getOperand(0);
  unsigned NumOps = N->getNumOperands();

  assert(NumOps > 1 && "Not enough operands to swap the Chain on Load/Store!");
  assert(Chain.getValueType() == MVT::Other && "Not a chain value!");

  SmallVector<SDValue, 3> Ops;
  for (unsigned i = 1; i != NumOps; ++i) {
    Ops.push_back(N->getOperand(i));
  }
  Ops.push_back(Chain);

  N = CurDAG->UpdateNodeOperands(N, Ops);
}


/// MorphNode - Handle morphing a node in place for the selector.
SDNode *InvISelDAG::
MorphNode(SDNode *Node, unsigned TargetOpc, SDVTList VTList,
          const SDValue *Ops, unsigned NumOps, unsigned EmitNodeInfo) {
  // It is possible we're using MorphNodeTo to replace a node with no
  // normal results with one that has a normal result (or we could be
  // adding a chain) and the input could have glue and chains as well.
  // In this case we need to shift the operands down.
  // FIXME: This is a horrible hack and broken in obscure cases, no worse
  // than the old isel though.
  int OldGlueResultNo = -1, OldChainResultNo = -1;

  unsigned NTMNumResults = Node->getNumValues();
  if (Node->getValueType(NTMNumResults-1) == MVT::Glue) {
    OldGlueResultNo = NTMNumResults-1;
    if (NTMNumResults != 1 &&
        Node->getValueType(NTMNumResults-2) == MVT::Other)
      OldChainResultNo = NTMNumResults-2;
  } else if (Node->getValueType(NTMNumResults-1) == MVT::Other)
    OldChainResultNo = NTMNumResults-1;

  // Call the underlying SelectionDAG routine to do the transmogrification. Note
  // that this deletes operands of the old node that become dead.
  SmallVector<SDValue, 3> OpsArray;
  for (unsigned i = 0, e = NumOps; i != e; ++i) {
    OpsArray.push_back(Ops[i]);
  }
  SDNode *Res = CurDAG->MorphNodeTo(Node, TargetOpc, VTList, OpsArray);

  // MorphNodeTo can operate in two ways: if an existing node with the
  // specified operands exists, it can just return it.  Otherwise, it
  // updates the node in place to have the requested operands.
  if (Res == Node) {
    // If we updated the node in place, reset the node ID.  To the isel,
    // this should be just like a newly allocated machine node.
    Res->setNodeId(-1);
  }

  unsigned ResNumResults = Res->getNumValues();
  // Move the glue if needed.
  if ((EmitNodeInfo & OPFL_GlueOutput) && OldGlueResultNo != -1 &&
      (unsigned)OldGlueResultNo != ResNumResults-1)
    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, OldGlueResultNo),
                                      SDValue(Res, ResNumResults-1));

  if ((EmitNodeInfo & OPFL_GlueOutput) != 0)
    --ResNumResults;

  // Move the chain reference if needed.
  if ((EmitNodeInfo & OPFL_Chain) && OldChainResultNo != -1 &&
      (unsigned)OldChainResultNo != ResNumResults-1)
    CurDAG->ReplaceAllUsesOfValueWith(SDValue(Node, OldChainResultNo),
                                      SDValue(Res, ResNumResults-1));

  // Otherwise, no replacement happened because the node already exists. Replace
  // Uses of the old node with the new one.
  if (Res != Node)
    CurDAG->ReplaceAllUsesWith(Node, Res);

  return Res;
}

SDNode* InvISelDAG::InvertCodeCommon(SDNode *NodeToMatch,
  const unsigned char *MatcherTable,
  unsigned TableSize) {
  // FIXME: Should these even be selected?  Handle these cases in the caller?
  switch (NodeToMatch->getOpcode()) {
  default:
    break;
  case ISD::EntryToken:       // These nodes remain the same.
  case ISD::BasicBlock:
  case ISD::Register:
  case ISD::RegisterMask:
  //case ISD::VALUETYPE:
  //case ISD::CONDCODE:
  case ISD::HANDLENODE:
  case ISD::MDNODE_SDNODE:
  case ISD::TargetConstant:
  case ISD::TargetConstantFP:
  case ISD::TargetConstantPool:
  case ISD::Constant:
  case ISD::TargetFrameIndex:
  case ISD::TargetExternalSymbol:
  case ISD::TargetBlockAddress:
  case ISD::TargetJumpTable:
  case ISD::TargetGlobalTLSAddress:
  case ISD::TargetGlobalAddress:
  case ISD::TokenFactor:
  case ISD::CopyFromReg:
  case ISD::CopyToReg:
  case ISD::EH_LABEL:
  case ISD::LIFETIME_START:
  case ISD::LIFETIME_END:
    NodeToMatch->setNodeId(-1); // Mark selected.
    return 0;
  case ISD::AssertSext:
  case ISD::AssertZext:
    // CurDAG->ReplaceAllUsesOfValueWith(SDValue(NodeToMatch, 0),
    //                                   NodeToMatch->getOperand(0));
    return 0;
  case ISD::INLINEASM:
    // return Select_INLINEASM(NodeToMatch);
    return 0;
  case ISD::UNDEF:
    // return Select_UNDEF(NodeToMatch);
    return 0;
  }

  assert(NodeToMatch->isMachineOpcode() && "Node already selected!");

  // Set up the node stack with NodeToMatch as the only node on the stack.
  SmallVector<SDValue, 8> NodeStack;
  SDValue N = SDValue(NodeToMatch, 0);
  NodeStack.push_back(N);

  // MatchScopes - Scopes used when matching, if a match failure happens, this
  // indicates where to continue checking.
  SmallVector<MatchScope, 8> MatchScopes;

  // RecordedNodes - This is the set of nodes that have been recorded by the
  // state machine.  The second value is the parent of the node, or null if the
  // root is recorded.
  SmallVector<std::pair<SDValue, SDNode*>, 8> RecordedNodes;

  // MatchedMemRefs - This is the set of MemRef's we've seen in the input
  // pattern.
  SmallVector<MachineMemOperand*, 2> MatchedMemRefs;

  // These are the current input chain and glue for use when generating nodes.
  // Various Emit operations change these.  For example, emitting a copytoreg
  // uses and updates these.
  SDValue InputChain, InputGlue;

  // ChainNodesMatched - If a pattern matches nodes that have input/output
  // chains, the OPC_EmitMergeInputChains operation is emitted which indicates
  // which ones they are.  The result is captured into this list so that we can
  // update the chain results when the pattern is complete.
  SmallVector<SDNode*, 3> ChainNodesMatched;
  SmallVector<SDNode*, 3> GlueResultNodesMatched;

  DEBUG(errs() << "ISEL: Starting pattern match on root node: ";
        NodeToMatch->dump();
        errs() << '\n');
  uint16_t TgtOpc = NodeToMatch->getOpcode();
  if (N->isMachineOpcode()) {
    TgtOpc = ~TgtOpc;
  }
  DEBUG(errs() << "Target Opcode is: " << TgtOpc << "\n");
        // NodeToMatch->dump(CurDAG);


  // Determine where to start the interpreter.  Normally we start at opcode #0,
  // but if the state machine starts with an OPC_SwitchOpcode, then we
  // accelerate the first lookup (which is guaranteed to be hot) with the
  // OpcodeOffset table.
  unsigned MatcherIndex = 0;

  std::vector<unsigned> OpcodeOffset; // Was global, put here to make following
                                      // code happy...

  if (!OpcodeOffset.empty()) {
    // Already computed the OpcodeOffset table, just index into it.
    if (N.getOpcode() < OpcodeOffset.size())
      MatcherIndex = OpcodeOffset[N.getOpcode()];
    DEBUG(errs() << "  Initial Opcode index to " << MatcherIndex << "\n");

  } else if (MatcherTable[0] == OPC_SwitchOpcode) {
    // Otherwise, the table isn't computed, but the state machine does start
    // with an OPC_SwitchOpcode instruction.  Populate the table now, since this
    // is the first time we're selecting an instruction.
    unsigned Idx = 1;
    while (1) {
      // Get the size of this case.
      unsigned CaseSize = MatcherTable[Idx++];
      if (CaseSize & 128)
        CaseSize = GetVBR(CaseSize, MatcherTable, Idx);
      if (CaseSize == 0) break;

      // Get the opcode, add the index to the table.
      uint16_t Opc = MatcherTable[Idx++];
      Opc |= (unsigned short)MatcherTable[Idx++] << 8;
      if (Opc >= OpcodeOffset.size())
        OpcodeOffset.resize((Opc+1)*2);
      OpcodeOffset[Opc] = Idx;
      Idx += CaseSize;
    }

    // Okay, do the lookup for the first opcode.
    if (N.getOpcode() < OpcodeOffset.size())
      MatcherIndex = OpcodeOffset[N.getOpcode()];
  }

  while (1) {
    assert(MatcherIndex < TableSize && "Invalid index");
#ifndef NDEBUG
    unsigned CurrentOpcodeIndex = MatcherIndex;
#endif
    BuiltinOpcodes Opcode = (BuiltinOpcodes)MatcherTable[MatcherIndex++];
    switch (Opcode) {
    case OPC_Scope: {
      // Okay, the semantics of this operation are that we should push a scope
      // then evaluate the first child.  However, pushing a scope only to have
      // the first check fail (which then pops it) is inefficient.  If we can
      // determine immediately that the first check (or first several) will
      // immediately fail, don't even bother pushing a scope for them.
      unsigned FailIndex;

      while (1) {
        unsigned NumToSkip = MatcherTable[MatcherIndex++];
        if (NumToSkip & 128)
          NumToSkip = GetVBR(NumToSkip, MatcherTable, MatcherIndex);
        // Found the end of the scope with no match.
        if (NumToSkip == 0) {
          FailIndex = 0;
          break;
        }

        FailIndex = MatcherIndex+NumToSkip;

        unsigned MatcherIndexOfPredicate = MatcherIndex;
        (void)MatcherIndexOfPredicate; // silence warning.

        // If we can't evaluate this predicate without pushing a scope (e.g. if
        // it is a 'MoveParent') or if the predicate succeeds on this node, we
        // push the scope and evaluate the full predicate chain.
        bool Result;
        MatcherIndex = IsPredicateKnownToFail(MatcherTable, MatcherIndex, N,
                                              Result, *this, RecordedNodes);
        if (!Result)
          break;

        DEBUG(errs() << "  Skipped scope entry (due to false predicate) at "
                     << "index " << MatcherIndexOfPredicate
                     << ", continuing at " << FailIndex << "\n");
        ++NumDAGIselRetries;

        // Otherwise, we know that this case of the Scope is guaranteed to fail,
        // move to the next case.
        MatcherIndex = FailIndex;
      }

      // If the whole scope failed to match, bail.
      if (FailIndex == 0) break;

      // Push a MatchScope which indicates where to go if the first child fails
      // to match.
      MatchScope NewEntry;
      NewEntry.FailIndex = FailIndex;
      NewEntry.NodeStack.append(NodeStack.begin(), NodeStack.end());
      NewEntry.NumRecordedNodes = RecordedNodes.size();
      NewEntry.NumMatchedMemRefs = MatchedMemRefs.size();
      NewEntry.InputChain = InputChain;
      NewEntry.InputGlue = InputGlue;
      NewEntry.HasChainNodesMatched = !ChainNodesMatched.empty();
      NewEntry.HasGlueResultNodesMatched = !GlueResultNodesMatched.empty();
      MatchScopes.push_back(NewEntry);
      continue;
    }
    case OPC_RecordNode: {
      // Remember this node, it may end up being an operand in the pattern.
      SDNode *Parent = 0;
      if (NodeStack.size() > 1)
        Parent = NodeStack[NodeStack.size()-2].getNode();
      RecordedNodes.push_back(std::make_pair(N, Parent));
      continue;
    }

    case OPC_RecordChild0: case OPC_RecordChild1:
    case OPC_RecordChild2: case OPC_RecordChild3:
    case OPC_RecordChild4: case OPC_RecordChild5:
    case OPC_RecordChild6: case OPC_RecordChild7: {
      unsigned ChildNo = Opcode-OPC_RecordChild0;
      if (ChildNo >= N.getNumOperands())
        break;  // Match fails if out of range child #.

      RecordedNodes.push_back(std::make_pair(N->getOperand(ChildNo),
                                             N.getNode()));
      continue;
    }
    case OPC_RecordMemRef:
      MatchedMemRefs.push_back(cast<MemSDNode>(N)->getMemOperand());
      continue;

    case OPC_CaptureGlueInput:
      // If the current node has an input glue, capture it in InputGlue.
      if (N->getNumOperands() != 0 &&
          N->getOperand(N->getNumOperands()-1).getValueType() == MVT::Glue)
        InputGlue = N->getOperand(N->getNumOperands()-1);
      continue;

    case OPC_MoveChild: {
      unsigned ChildNo = MatcherTable[MatcherIndex++];
      if (ChildNo >= N.getNumOperands())
        break;  // Match fails if out of range child #.
      N = N.getOperand(ChildNo);
      NodeStack.push_back(N);
      continue;
    }

    case OPC_MoveParent:
      // Pop the current node off the NodeStack.
      NodeStack.pop_back();
      assert(!NodeStack.empty() && "Node stack imbalance!");
      N = NodeStack.back();
      continue;

    case OPC_CheckSame:
      if (!fracture::CheckSame(MatcherTable, MatcherIndex, N, RecordedNodes))
        break;
      continue;
    case OPC_CheckPatternPredicate:
      if (!fracture::CheckPatternPredicate(MatcherTable, MatcherIndex, *this))
        break;
      continue;
    case OPC_CheckPredicate:
      if (!fracture::CheckNodePredicate(MatcherTable, MatcherIndex, *this,
                                N.getNode()))
        break;
      continue;
    case OPC_CheckComplexPat: {
      unsigned CPNum = MatcherTable[MatcherIndex++];
      unsigned RecNo = MatcherTable[MatcherIndex++];
      assert(RecNo < RecordedNodes.size() && "Invalid CheckComplexPat");
      if (!CheckComplexPattern(NodeToMatch, RecordedNodes[RecNo].second,
          RecordedNodes[RecNo].first, CPNum,
          RecordedNodes, RecNo))
        break;
      continue;
    }
    case OPC_CheckOpcode:
      if (!fracture::CheckOpcode(MatcherTable, MatcherIndex, N.getNode()))
        break;
      continue;

    case OPC_CheckType:
      if (!fracture::CheckType(MatcherTable, MatcherIndex, N, *TLI)) break;
      continue;

    case OPC_SwitchOpcode: {
      unsigned CurNodeOpcode = N.getOpcode();
      unsigned SwitchStart = MatcherIndex-1; (void)SwitchStart;
      unsigned CaseSize;
      while (1) {
        // Get the size of this case.
        CaseSize = MatcherTable[MatcherIndex++];
        if (CaseSize & 128)
          CaseSize = GetVBR(CaseSize, MatcherTable, MatcherIndex);
        if (CaseSize == 0) break;

        uint16_t Opc = MatcherTable[MatcherIndex++];
        Opc |= (unsigned short)MatcherTable[MatcherIndex++] << 8;

        // If the opcode matches, then we will execute this case.
        if (CurNodeOpcode == Opc)
          break;

        // Otherwise, skip over this case.
        MatcherIndex += CaseSize;
      }

      // If no cases matched, bail out.
      if (CaseSize == 0) break;

      // Otherwise, execute the case we found.
      DEBUG(errs() << "  OpcodeSwitch from " << SwitchStart
                   << " to " << MatcherIndex << "\n");
      continue;
    }

    case OPC_SwitchType: {
      MVT CurNodeVT = N.getValueType().getSimpleVT();
      unsigned SwitchStart = MatcherIndex-1; (void)SwitchStart;
      unsigned CaseSize;
      while (1) {
        // Get the size of this case.
        CaseSize = MatcherTable[MatcherIndex++];
        if (CaseSize & 128)
          CaseSize = GetVBR(CaseSize, MatcherTable, MatcherIndex);
        if (CaseSize == 0) break;

        MVT CaseVT = (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
        if (CaseVT == MVT::iPTR)
          CaseVT = TLI->getPointerTy();

        // If the VT matches, then we will execute this case.
        if (CurNodeVT == CaseVT)
          break;

        // Otherwise, skip over this case.
        MatcherIndex += CaseSize;
      }

      // If no cases matched, bail out.
      if (CaseSize == 0) break;

      // Otherwise, execute the case we found.
      DEBUG(errs() << "  TypeSwitch[" << EVT(CurNodeVT).getEVTString()
                   << "] from " << SwitchStart << " to " << MatcherIndex<<'\n');
      continue;
    }
    case OPC_CheckChild0Type: case OPC_CheckChild1Type:
    case OPC_CheckChild2Type: case OPC_CheckChild3Type:
    case OPC_CheckChild4Type: case OPC_CheckChild5Type:
    case OPC_CheckChild6Type: case OPC_CheckChild7Type:
      if (!fracture::CheckChildType(MatcherTable, MatcherIndex, N, *TLI,
                            Opcode-OPC_CheckChild0Type))
        break;
      continue;
    case OPC_CheckCondCode:
      if (!fracture::CheckCondCode(MatcherTable, MatcherIndex, N)) break;
      continue;
    case OPC_CheckValueType:
      if (!fracture::CheckValueType(MatcherTable, MatcherIndex, N, *TLI)) break;
      continue;
    case OPC_CheckInteger:
      if (!fracture::CheckInteger(MatcherTable, MatcherIndex, N)) break;
      continue;
    case OPC_CheckAndImm:
      if (!fracture::CheckAndImm(MatcherTable, MatcherIndex, N, *this)) break;
      continue;
    case OPC_CheckOrImm:
      if (!fracture::CheckOrImm(MatcherTable, MatcherIndex, N, *this)) break;
      continue;

    case OPC_CheckFoldableChainNode: {
      // assert(NodeStack.size() != 1 && "No parent node");
      // // Verify that all intermediate nodes between the root and this one
      // have
      // // a single use.
      // bool HasMultipleUses = false;
      // for (unsigned i = 1, e = NodeStack.size()-1; i != e; ++i)
      //   if (!NodeStack[i].hasOneUse()) {
      //     HasMultipleUses = true;
      //     break;
      //   }
      // if (HasMultipleUses) break;

      // // Check to see that the target thinks this is profitable to fold and
      // that
      // // we can fold it without inducing cycles in the graph.
      // if (!IsProfitableToFold(N, NodeStack[NodeStack.size()-2].getNode(),
      //                         NodeToMatch) ||
      //     !IsLegalToFold(N, NodeStack[NodeStack.size()-2].getNode(),
      //                    NodeToMatch, OptLevel,
      //                    true/*We validate our own chains*/))
      //   break;

      continue;
    }
    case OPC_EmitInteger: {
      MVT::SimpleValueType VT =
        (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
      int64_t Val = MatcherTable[MatcherIndex++];
      if (Val & 128)
        Val = GetVBR(Val, MatcherTable, MatcherIndex);
      RecordedNodes.push_back(std::pair<SDValue, SDNode*>(
                              CurDAG->getTargetConstant(Val, VT), (SDNode*)0));
      continue;
    }
    case OPC_EmitRegister: {
      MVT::SimpleValueType VT =
        (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
      unsigned RegNo = MatcherTable[MatcherIndex++];
      RecordedNodes.push_back(std::pair<SDValue, SDNode*>(
                              CurDAG->getRegister(RegNo, VT), (SDNode*)0));
      continue;
    }
    case OPC_EmitRegister2: {
      // For targets w/ more than 256 register names, the register enum
      // values are stored in two bytes in the matcher table (just like
      // opcodes).
      MVT::SimpleValueType VT =
        (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
      unsigned RegNo = MatcherTable[MatcherIndex++];
      RegNo |= MatcherTable[MatcherIndex++] << 8;
      RecordedNodes.push_back(std::pair<SDValue, SDNode*>(
                              CurDAG->getRegister(RegNo, VT), (SDNode*)0));
      continue;
    }

    case OPC_EmitConvertToTarget:  {
      // Convert from IMM/FPIMM to target version.
      unsigned RecNo = MatcherTable[MatcherIndex++];
      assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
      SDValue Imm = RecordedNodes[RecNo].first;

      if (Imm->getOpcode() == ISD::Constant) {
        int64_t Val = cast<ConstantSDNode>(Imm)->getZExtValue();
        Imm = CurDAG->getTargetConstant(Val, Imm.getValueType());
      } else if (Imm->getOpcode() == ISD::ConstantFP) {
        const ConstantFP *Val=cast<ConstantFPSDNode>(Imm)->getConstantFPValue();
        Imm = CurDAG->getTargetConstantFP(*Val, Imm.getValueType());
      }

      RecordedNodes.push_back(std::make_pair(Imm, RecordedNodes[RecNo].second));
      continue;
    }

    case OPC_EmitMergeInputChains1_0:    // OPC_EmitMergeInputChains, 1, 0
    case OPC_EmitMergeInputChains1_1: {  // OPC_EmitMergeInputChains, 1, 1
      // These are space-optimized forms of OPC_EmitMergeInputChains.
      assert(InputChain.getNode() == 0 &&
             "EmitMergeInputChains should be the first chain producing node");
      assert(ChainNodesMatched.empty() &&
             "Should only have one EmitMergeInputChains per match");

      // Read all of the chained nodes.
      unsigned RecNo = Opcode == OPC_EmitMergeInputChains1_1;
      assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
      ChainNodesMatched.push_back(RecordedNodes[RecNo].first.getNode());

      // FIXME: What if other value results of the node have uses not matched
      // by this pattern?
      if (ChainNodesMatched.back() != NodeToMatch &&
          !RecordedNodes[RecNo].first.hasOneUse()) {
        ChainNodesMatched.clear();
        break;
      }

      // Merge the input chains if they are not intra-pattern references.
      InputChain = HandleMergeInputChains(ChainNodesMatched, CurDAG);

      if (InputChain.getNode() == 0)
        break;  // Failed to merge.
      continue;
    }

    case OPC_EmitMergeInputChains: {
      assert(InputChain.getNode() == 0 &&
             "EmitMergeInputChains should be the first chain producing node");
      // This node gets a list of nodes we matched in the input that have
      // chains.  We want to token factor all of the input chains to these nodes
      // together.  However, if any of the input chains is actually one of the
      // nodes matched in this pattern, then we have an intra-match reference.
      // Ignore these because the newly token factored chain should not refer to
      // the old nodes.
      unsigned NumChains = MatcherTable[MatcherIndex++];
      assert(NumChains != 0 && "Can't TF zero chains");

      assert(ChainNodesMatched.empty() &&
             "Should only have one EmitMergeInputChains per match");

      // Read all of the chained nodes.
      for (unsigned i = 0; i != NumChains; ++i) {
        unsigned RecNo = MatcherTable[MatcherIndex++];
        assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
        ChainNodesMatched.push_back(RecordedNodes[RecNo].first.getNode());

        // FIXME: What if other value results of the node have uses not matched
        // by this pattern?
        if (ChainNodesMatched.back() != NodeToMatch &&
            !RecordedNodes[RecNo].first.hasOneUse()) {
          ChainNodesMatched.clear();
          break;
        }
      }

      // If the inner loop broke out, the match fails.
      if (ChainNodesMatched.empty())
        break;

      // Merge the input chains if they are not intra-pattern references.
      InputChain = HandleMergeInputChains(ChainNodesMatched, CurDAG);

      if (InputChain.getNode() == 0)
        break;  // Failed to merge.

      continue;
    }

    case OPC_EmitCopyToReg: {
      unsigned RecNo = MatcherTable[MatcherIndex++];
      assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
      unsigned DestPhysReg = MatcherTable[MatcherIndex++];

      if (InputChain.getNode() == 0)
        InputChain = CurDAG->getEntryNode();

      InputChain = CurDAG->getCopyToReg(InputChain, SDLoc(NodeToMatch),
                                        DestPhysReg, RecordedNodes[RecNo].first,
                                        InputGlue);

      InputGlue = InputChain.getValue(1);
      continue;
    }

    case OPC_EmitNodeXForm: {
      // unsigned XFormNo = MatcherTable[MatcherIndex++];
      // unsigned RecNo = MatcherTable[MatcherIndex++];
      // assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
      // SDValue Res = RunSDNodeXForm(RecordedNodes[RecNo].first, XFormNo);
      // RecordedNodes.push_back(std::pair<SDValue,SDNode*>(Res, (SDNode*) 0));
      continue;
    }

    case OPC_EmitNode:
    case OPC_MorphNodeTo: {
      uint16_t TargetOpc = MatcherTable[MatcherIndex++];
      TargetOpc |= (unsigned short)MatcherTable[MatcherIndex++] << 8;
      unsigned EmitNodeInfo = MatcherTable[MatcherIndex++];
      // Get the result VT list.
      unsigned NumVTs = MatcherTable[MatcherIndex++];
      SmallVector<EVT, 4> VTs;
      for (unsigned i = 0; i != NumVTs; ++i) {
        MVT::SimpleValueType VT =
          (MVT::SimpleValueType)MatcherTable[MatcherIndex++];
        if (VT == MVT::iPTR) VT = TLI->getPointerTy().SimpleTy;
        VTs.push_back(VT);
      }

      if (EmitNodeInfo & OPFL_Chain)
        VTs.push_back(MVT::Other);
      if (EmitNodeInfo & OPFL_GlueOutput)
        VTs.push_back(MVT::Glue);

      // This is hot code, so optimize the two most common cases of 1 and 2
      // results.
      SDVTList VTList;
      if (VTs.size() == 1)
        VTList = CurDAG->getVTList(VTs[0]);
      else if (VTs.size() == 2)
        VTList = CurDAG->getVTList(VTs[0], VTs[1]);
      else
        VTList = CurDAG->getVTList(VTs);

      // Get the operand list.
      unsigned NumOps = MatcherTable[MatcherIndex++];
      SmallVector<SDValue, 8> Ops;
      for (unsigned i = 0; i != NumOps; ++i) {
        unsigned RecNo = MatcherTable[MatcherIndex++];
        if (RecNo & 128)
          RecNo = GetVBR(RecNo, MatcherTable, MatcherIndex);

        assert(RecNo < RecordedNodes.size() && "Invalid EmitNode");
        Ops.push_back(RecordedNodes[RecNo].first);
      }

      // If there are variadic operands to add, handle them now.
      if (EmitNodeInfo & OPFL_VariadicInfo) {
        // Determine the start index to copy from.
        // unsigned FirstOpToCopy = getNumFixedFromVariadicInfo(EmitNodeInfo);
        // FirstOpToCopy += (EmitNodeInfo & OPFL_Chain) ? 1 : 0;
        // assert(NodeToMatch->getNumOperands() >= FirstOpToCopy &&
        //        "Invalid variadic node");
        // // Copy all of the variadic operands, not including a potential glue
        // // input.
        // for (unsigned i = FirstOpToCopy, e = NodeToMatch->getNumOperands();
        //      i != e; ++i) {
        //   SDValue V = NodeToMatch->getOperand(i);
        //   if (V.getValueType() == MVT::Glue) break;
        //   Ops.push_back(V);
        // }
      }

      // If this has chain/glue inputs, add them.
      if (EmitNodeInfo & OPFL_Chain) {
        if (InputChain.getNode() == 0)
          InputChain = CurDAG->getEntryNode();
        Ops.push_back(InputChain);
      }
      if ((EmitNodeInfo & OPFL_GlueInput) && InputGlue.getNode() != 0)
        Ops.push_back(InputGlue);

      // Create the node.
      SDNode *Res = 0;

      if (Opcode != OPC_MorphNodeTo && EmitNodeInfo & OPFL_MemRefs) {
        // could be a load or a store
        const MachineSDNode *SrcNode = dyn_cast<MachineSDNode>(NodeToMatch);
        MachineMemOperand *MMO = NULL;
        if (SrcNode->memoperands_empty()) {
          errs() << "NO MACHINE OPS!\n";
        } else {
          MMO = *(SrcNode->memoperands_begin());
        }
        if (TargetOpc == ISD::STORE) {
          Res = (CurDAG->getStore(InputChain, SDLoc(NodeToMatch), Ops[0], 
              Ops[1], MMO)).getNode();
        }
        if (TargetOpc == ISD::LOAD) {
          // Ops[0] - chain, Ops[1] - src register, Ops[2] offImm
          EVT LdType = NodeToMatch->getValueType(0);
          // unsigned Alignment = TLI->getDataLayout()->getABITypeAlignment(
          //   NodeToMatch->getType());
          // NOTE: Selection/DAG handles Alignment = 0;.
          unsigned Alignment = 0;
          Res = (CurDAG->getLoad(LdType, SDLoc(NodeToMatch), InputChain, Ops[0],
              MachinePointerInfo::getConstantPool(), 
              false, false, true, //FIXME: Just guessing on these.
              Alignment)).getNode();
        }
        RecordedNodes.clear();
        for (unsigned i = 0, e = VTs.size(); i != e; ++i) {
          if (VTs[i] == MVT::Other || VTs[i] == MVT::Glue) break;
          RecordedNodes.push_back(std::pair<SDValue,SDNode*>(SDValue(Res, i),
              (SDNode*) 0));
        }
        FixChainOp(Res);
      }
      else if (Opcode != OPC_MorphNodeTo 
        && !NodeToMatch->isTargetMemoryOpcode()) {
        // If this is a normal EmitNode command, just create the new node and
        // add the results to the RecordedNodes list.
        Res = (CurDAG->getNode(TargetOpc, SDLoc(NodeToMatch),
            VTList, Ops)).getNode();

        // Add all the non-glue/non-chain results to the RecordedNodes list.
        // Note: The following line was necessary to make replacealluses work
        //       in OPC_CompleteMatch.
        //       It should be safe as our recorded nodes should not be needed 
        //       after we generate the new node. It might break other 
        //       functionality, however, as ResSlot is supposed to be the 
        //       slot to replace for each value type produced by the instruction.
        RecordedNodes.clear();
        for (unsigned i = 0, e = VTs.size(); i != e; ++i) {
          if (VTs[i] == MVT::Other || VTs[i] == MVT::Glue) break;
          RecordedNodes.push_back(std::pair<SDValue,SDNode*>(SDValue(Res, i),
                                                             (SDNode*) 0));
        }

      } else if (NodeToMatch->getOpcode() != ISD::DELETED_NODE) {
        for (unsigned i = 0, e = Ops.size(); i != e; ++i) {
          outs() << "Ops: ";
          Ops[i]->dump();
          outs() << "\n";
        }
        Res = MorphNode(NodeToMatch, TargetOpc, VTList, Ops.data(), Ops.size(),
          EmitNodeInfo);
      } else {
        // NodeToMatch was eliminated by CSE when the target changed the DAG.
        // We will visit the equivalent node later.
        DEBUG(dbgs() << "Node was eliminated by CSE\n");
        return 0;
      }

      // If the node had chain/glue results, update our notion of the current
      // chain and glue.
      if (EmitNodeInfo & OPFL_GlueOutput) {
        InputGlue = SDValue(Res, VTs.size()-1);
        if (EmitNodeInfo & OPFL_Chain)
          InputChain = SDValue(Res, VTs.size()-2);
      } else if (EmitNodeInfo & OPFL_Chain)
        InputChain = SDValue(Res, VTs.size()-1);

      // If the OPFL_MemRefs glue is set on this node, slap all of the
      // accumulated memrefs onto it.
      //
      // FIXME: This is vastly incorrect for patterns with multiple outputs
      // instructions that access memory and for ComplexPatterns that match
      // loads.
      // if (EmitNodeInfo & OPFL_MemRefs) {
      //   // Only attach load or store memory operands if the generated
      //   // instruction may load or store.
      //   const MCInstrDesc &MCID = TM->getInstrInfo()->get(TargetOpc);
      //   bool mayLoad = MCID.mayLoad();
      //   bool mayStore = MCID.mayStore();

      //   unsigned NumMemRefs = 0;
      //   for (SmallVector<MachineMemOperand*, 2>::const_iterator I =
      //        MatchedMemRefs.begin(), E = MatchedMemRefs.end(); I != E; ++I) {
      //     if ((*I)->isLoad()) {
      //       if (mayLoad)
      //         ++NumMemRefs;
      //     } else if ((*I)->isStore()) {
      //       if (mayStore)
      //         ++NumMemRefs;
      //     } else {
      //       ++NumMemRefs;
      //     }
      //   }

      //   MachineSDNode::mmo_iterator MemRefs =
      //     MF->allocateMemRefsArray(NumMemRefs);

      //   MachineSDNode::mmo_iterator MemRefsPos = MemRefs;
      //   for (SmallVector<MachineMemOperand*, 2>::const_iterator I =
      //        MatchedMemRefs.begin(), E = MatchedMemRefs.end(); I != E; ++I) {
      //     if ((*I)->isLoad()) {
      //       if (mayLoad)
      //         *MemRefsPos++ = *I;
      //     } else if ((*I)->isStore()) {
      //       if (mayStore)
      //         *MemRefsPos++ = *I;
      //     } else {
      //       *MemRefsPos++ = *I;
      //     }
      //   }

      //   // cast<MemSDNode>(Res)
      //   //   ->setMemRefs(MemRefs, MemRefs + NumMemRefs);
      // }

      DEBUG(errs() << "  "
                   << (Opcode == OPC_MorphNodeTo ? "Morphed" : "Created")
                   << " node: "; Res->dump(CurDAG); errs() << "\n");

      // If this was a MorphNodeTo then we're completely done!
      if (Opcode == OPC_MorphNodeTo) {
        // Update chain and glue uses.
        UpdateChainsAndGlue(NodeToMatch, InputChain, ChainNodesMatched,
                            InputGlue, GlueResultNodesMatched, true);
        return Res;
      }

      continue;
    }

    case OPC_MarkGlueResults: {
      unsigned NumNodes = MatcherTable[MatcherIndex++];

      // Read and remember all the glue-result nodes.
      for (unsigned i = 0; i != NumNodes; ++i) {
        unsigned RecNo = MatcherTable[MatcherIndex++];
        if (RecNo & 128)
          RecNo = GetVBR(RecNo, MatcherTable, MatcherIndex);

        assert(RecNo < RecordedNodes.size() && "Invalid CheckSame");
        GlueResultNodesMatched.push_back(RecordedNodes[RecNo].first.getNode());
      }
      continue;
    }

    case OPC_CompleteMatch: {
      // The match has been completed, and any new nodes (if any) have been
      // created.  Patch up references to the matched dag to use the newly
      // created nodes.
      unsigned NumResults = MatcherTable[MatcherIndex++];

      for (unsigned i = 0; i != NumResults; ++i) {
        unsigned ResSlot = MatcherTable[MatcherIndex++];
        if (ResSlot & 128)
          ResSlot = GetVBR(ResSlot, MatcherTable, MatcherIndex);

        assert(ResSlot < RecordedNodes.size() && "Invalid CheckSame");
        SDValue Res = RecordedNodes[ResSlot].first;
 
        assert(i < NodeToMatch->getNumValues() &&
               NodeToMatch->getValueType(i) != MVT::Other &&
               NodeToMatch->getValueType(i) != MVT::Glue &&
               "Invalid number of results to complete!");
        assert((NodeToMatch->getValueType(i) == Res.getValueType() ||
                NodeToMatch->getValueType(i) == MVT::iPTR ||
                Res.getValueType() == MVT::iPTR ||
                NodeToMatch->getValueType(i).getSizeInBits() ==
                    Res.getValueType().getSizeInBits()) &&
               "invalid replacement");
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(NodeToMatch, i), Res);
      }

      // If the root node defines glue, add it to the glue nodes to update list.
      if (NodeToMatch->getValueType(NodeToMatch->getNumValues()-1) == MVT::Glue)
        GlueResultNodesMatched.push_back(NodeToMatch);

      // Update chain and glue uses.
      UpdateChainsAndGlue(NodeToMatch, InputChain, ChainNodesMatched,
                          InputGlue, GlueResultNodesMatched, false);

      DEBUG(errs() << NodeToMatch->use_size() << " uses left.\n");
      for (SDNode::use_iterator i = NodeToMatch->use_begin(),
             e = NodeToMatch->use_end(); i != e; ++i) {
        DEBUG(errs() << "-->");
        DEBUG(i->dump());
        DEBUG(errs() << "\n");
      }

      assert(NodeToMatch->use_empty() &&
             "Didn't replace all uses of the node?");

      // FIXME: We just return here, which interacts correctly with SelectRoot
      // above.  We should fix this to not return an SDNode* anymore.
      return 0;
    }
    }

    // If the code reached this point, then the match failed.  See if there is
    // another child to try in the current 'Scope', otherwise pop it until we
    // find a case to check.
    DEBUG(errs() << "  Match failed at index " << CurrentOpcodeIndex << "\n");
    ++NumDAGIselRetries;
    while (1) {
      if (MatchScopes.empty()) {
        CannotYetSelect(NodeToMatch);
        return 0;
      }

      // Restore the interpreter state back to the point where the scope was
      // formed.
      MatchScope &LastScope = MatchScopes.back();
      RecordedNodes.resize(LastScope.NumRecordedNodes);
      NodeStack.clear();
      NodeStack.append(LastScope.NodeStack.begin(), LastScope.NodeStack.end());
      N = NodeStack.back();

      if (LastScope.NumMatchedMemRefs != MatchedMemRefs.size())
        MatchedMemRefs.resize(LastScope.NumMatchedMemRefs);
      MatcherIndex = LastScope.FailIndex;

      DEBUG(errs() << "  Continuing at " << MatcherIndex << "\n");

      InputChain = LastScope.InputChain;
      InputGlue = LastScope.InputGlue;
      if (!LastScope.HasChainNodesMatched)
        ChainNodesMatched.clear();
      if (!LastScope.HasGlueResultNodesMatched)
        GlueResultNodesMatched.clear();

      // Check to see what the offset is at the new MatcherIndex.  If it is zero
      // we have reached the end of this scope, otherwise we have another child
      // in the current scope to try.
      unsigned NumToSkip = MatcherTable[MatcherIndex++];
      if (NumToSkip & 128)
        NumToSkip = GetVBR(NumToSkip, MatcherTable, MatcherIndex);

      // If we have another child in this scope to match, update FailIndex and
      // try it.
      if (NumToSkip != 0) {
        LastScope.FailIndex = MatcherIndex+NumToSkip;
        break;
      }

      // End of this scope, pop it and try the next child in the containing
      // scope.
      MatchScopes.pop_back();
    }
  }
}

//===----------------------------------------------------------------------===//
// Helper functions used by the generated instruction selector.
//===----------------------------------------------------------------------===//
// Calls to these methods are generated by tblgen.

/// CheckAndMask - The isel is trying to match something like (and X, 255).  If
/// the dag combiner simplified the 255, we still want to match.  RHS is the
/// actual value in the DAG on the RHS of an AND, and DesiredMaskS is the value
/// specified in the .td file (e.g. 255).
bool InvISelDAG::CheckAndMask(SDValue LHS, ConstantSDNode *RHS,
                                    int64_t DesiredMaskS) const {
  const APInt &ActualMask = RHS->getAPIntValue();
  const APInt &DesiredMask = APInt(LHS.getValueSizeInBits(), DesiredMaskS);

  // If the actual mask exactly matches, success!
  if (ActualMask == DesiredMask)
    return true;

  // If the actual AND mask is allowing unallowed bits, this doesn't match.
  if (ActualMask.intersects(~DesiredMask))
    return false;

  // Otherwise, the DAG Combiner may have proven that the value coming in is
  // either already zero or is not demanded.  Check for known zero input bits.
  APInt NeededMask = DesiredMask & ~ActualMask;
  if (CurDAG->MaskedValueIsZero(LHS, NeededMask))
    return true;

  // TODO: check to see if missing bits are just not demanded.

  // Otherwise, this pattern doesn't match.
  return false;
}

/// CheckOrMask - The isel is trying to match something like (or X, 255).  If
/// the dag combiner simplified the 255, we still want to match.  RHS is the
/// actual value in the DAG on the RHS of an OR, and DesiredMaskS is the value
/// specified in the .td file (e.g. 255).
bool InvISelDAG::CheckOrMask(SDValue LHS, ConstantSDNode *RHS,
                                   int64_t DesiredMaskS) const {
  const APInt &ActualMask = RHS->getAPIntValue();
  const APInt &DesiredMask = APInt(LHS.getValueSizeInBits(), DesiredMaskS);

  // If the actual mask exactly matches, success!
  if (ActualMask == DesiredMask)
    return true;

  // If the actual AND mask is allowing unallowed bits, this doesn't match.
  if (ActualMask.intersects(~DesiredMask))
    return false;

  // Otherwise, the DAG Combiner may have proven that the value coming in is
  // either already zero or is not demanded.  Check for known zero input bits.
  APInt NeededMask = DesiredMask & ~ActualMask;

  APInt KnownZero, KnownOne;
  CurDAG->computeKnownBits(LHS, KnownZero, KnownOne);

  // If all the missing bits in the or are already known to be set, match!
  if ((NeededMask & KnownOne) == NeededMask)
    return true;

  // TODO: check to see if missing bits are just not demanded.

  // Otherwise, this pattern doesn't match.
  return false;
}


/// SelectInlineAsmMemoryOperands - Calls to this are automatically generated
/// by tblgen.  Others should not call it.
void InvISelDAG::SelectInlineAsmMemoryOperands(std::vector<SDValue> &Ops) {
  std::vector<SDValue> InOps;
  std::swap(InOps, Ops);

  Ops.push_back(InOps[InlineAsm::Op_InputChain]); // 0
  Ops.push_back(InOps[InlineAsm::Op_AsmString]);  // 1
  Ops.push_back(InOps[InlineAsm::Op_MDNode]);     // 2, !srcloc
  Ops.push_back(InOps[InlineAsm::Op_ExtraInfo]);  // 3 (SideEffect, AlignStack)

  unsigned i = InlineAsm::Op_FirstOperand, e = InOps.size();
  if (InOps[e-1].getValueType() == MVT::Glue)
    --e;  // Don't process a glue operand if it is here.

  while (i != e) {
    unsigned Flags = cast<ConstantSDNode>(InOps[i])->getZExtValue();
    if (!InlineAsm::isMemKind(Flags)) {
      // Just skip over this operand, copying the operands verbatim.
      Ops.insert(Ops.end(), InOps.begin()+i,
                 InOps.begin()+i+InlineAsm::getNumOperandRegisters(Flags) + 1);
      i += InlineAsm::getNumOperandRegisters(Flags) + 1;
    } else {
      assert(InlineAsm::getNumOperandRegisters(Flags) == 1 &&
             "Memory operand with multiple values?");
      // Otherwise, this is a memory operand.  Ask the target to select it.
      std::vector<SDValue> SelOps;
      // if (SelectInlineAsmMemoryOperand(InOps[i+1], 'm', SelOps))
      //   report_fatal_error("Could not match memory address.  Inline asm"
      //                      " failure!");

      // Add this to the output node.
      unsigned NewFlags =
        InlineAsm::getFlagWord(InlineAsm::Kind_Mem, SelOps.size());
      Ops.push_back(CurDAG->getTargetConstant(NewFlags, MVT::i32));
      Ops.insert(Ops.end(), SelOps.begin(), SelOps.end());
      i += 2;
    }
  }

  // Add the glue input back if present.
  if (e != InOps.size())
    Ops.push_back(InOps.back());
}

void InvISelDAG::CannotYetSelect(SDNode *N) {
  std::string msg;
  raw_string_ostream Msg(msg);
  Msg << "Cannot select: ";

  if (N->getOpcode() != ISD::INTRINSIC_W_CHAIN &&
      N->getOpcode() != ISD::INTRINSIC_WO_CHAIN &&
      N->getOpcode() != ISD::INTRINSIC_VOID) {
    N->printrFull(Msg, CurDAG);
    Msg << "\nIn function: " << MF->getName();
  } else {
    // bool HasInputChain = N->getOperand(0).getValueType() == MVT::Other;
    // unsigned iid =
    //   cast<ConstantSDNode>(N->getOperand(HasInputChain))->getZExtValue();
    // if (iid < Intrinsic::num_intrinsics)
    //   Msg << "intrinsic %" << Intrinsic::getName((Intrinsic::ID)iid);
    // else if (const TargetIntrinsicInfo *TII = TM.getIntrinsicInfo())
    //   Msg << "target intrinsic %" << TII->getName(iid);
    // else
    //   Msg << "unknown intrinsic #" << iid;
  }
  report_fatal_error(Msg.str());
}

} // end namespace fracture
