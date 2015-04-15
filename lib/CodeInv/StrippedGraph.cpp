//===--------------------  StrippedGraph.cpp  --------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class attempts to hold a tree data structure for graphing the
// disassembly of a binary section in order to find all functions in
// stripped binaries.
//
// Author: Jackson Korba(jqk1032) <jqk1032@draper.com>
// Date: March 3, 2015
//===----------------------------------------------------------------------===//

#include "CodeInv/Disassembler.h"
#include "CodeInv/StrippedGraph.h"


namespace fracture {

void StrippedGraph::addGraphNode(GraphNode *Node) {
  /*outs() << "Initial NeedsLink: ";
  const char *Fmt;
  Fmt = "%08" PRIx64;
  for (auto &it : NeedsLink)
    outs() << format(Fmt, it->Address) << " ";
  outs() << "\n";
  */
  // If the graph is empty, start a new node with the first basic block.
  if (HeadNodes.empty()) {
    PrevNode = Node;
    HeadNodes.push_back(Node);
    return;
  }
  // If the previous basic block ends with a return instruction that is
  // not a conditional return, or the current basic block starts with a
  // function prologue, this must be the start of a function.
  if ((PrevNode != NULL && PrevNode->NodeBlock->instr_rbegin()->isReturn()
      && !isConditionalTerminator(PrevNode))
      || isFunctionBegin(Node)) {
    //outs() << "Prev is Terminator\n";
    HeadNodes.push_back(Node);
  }
  // If the previous basic block ends with a conditional branch, we want to
  // automatically add the current node as a successor to the previous node,
  // and push the previous node into the NeedsLink vector for later linking
  // of the other successor. 
  if (PrevNode->NodeBlock->instr_rbegin()->isConditionalBranch() &&
           !isAddressInBasicBlock(PrevNode->BranchAddress, Node)) {
    PrevNode->SuccNodes.push_back(Node);
    NeedsLink.push_back(PrevNode);
  }
  // If the current block ends with an unconditional branch, we want to
  // push it into NeedsLink for later linking.
  if (Node->NodeBlock->instr_rbegin()->isUnconditionalBranch())
    NeedsLink.push_back(Node);
  // If there are links that need to be resolved, attempt to resolve them.
  if (!NeedsLink.empty())
    resolveLinks();

  PrevNode = Node;
  /*outs() << "NeedsLink: ";
  for (auto &it : NeedsLink)
    outs() << format(Fmt, it->Address) << " ";
  outs() << "\n";
  */
  return;
}

void StrippedGraph::addToList(GraphNode *Node) {
  AllNodes.push_back(Node);
}

void StrippedGraph::resolveLinks() {
  bool NodeDeleted = false;
  for (std::vector<GraphNode *>::iterator vecit = NeedsLink.begin();
       vecit != NeedsLink.end(); ) {
    for(auto &it : AllNodes) {
      if (isAddressInBasicBlock((*vecit)->BranchAddress, it) &&
          !isAlreadySuccessor(*vecit, it)) {
        (*vecit)->SuccNodes.push_back(it);
        vecit = NeedsLink.erase(vecit);
        NodeDeleted = true;
      }
    }
    if(!NodeDeleted)
      vecit++;
    NodeDeleted = false; 
  }
}

void StrippedGraph::printGraph() {
  initializeColors();
  for (auto &it : HeadNodes) {
    outs() << "Function Begin!\n";
    printVisit(it);
  }
}

std::vector<GraphNode *> StrippedGraph::getHeadNodes() {
  return HeadNodes;
}

void StrippedGraph::printNode(GraphNode *Node) {
  const char *Fmt;
  Fmt = "%08" PRIx64;
  outs() << "Address: " << format(Fmt, Node->Address) << "\n"
         << "Branch: " << format(Fmt, Node->BranchAddress) << "\n"
         << "Successors: ";
  for (auto &it : Node->SuccNodes)
    outs() << format(Fmt, it->Address) << " ";
  outs() << "\n\n";
  Node->NodeColor = Color::BLACK;
}

void StrippedGraph::printVisit(GraphNode *Node) {
  if (Node->NodeColor == Color::GRAY) {
    printNode(Node);
    for (auto &it : Node->SuccNodes) {
      if(it->Address == it->BranchAddress)
        continue;
      if(isSuccessorLoop(it))
        continue;
      printVisit(it);
    }
    //Node->NodeColor = Color::BLACK;
  }
}

bool StrippedGraph::isAddressInBasicBlock(uint64_t Address, GraphNode *Node) {
  if (Address >= Node->Address && Address <= Node->End)
    return true;
  return false;
}

bool StrippedGraph::isJumpToCompleteFunction(GraphNode *Node) {
  for (auto &it : HeadNodes)
    if (isAddressInBasicBlock(Node->BranchAddress, it))
      return true;
  return false;
}

void StrippedGraph::initializeColors() {
  for (auto &it : AllNodes)
    it->NodeColor = Color::GRAY;
}

bool StrippedGraph::isAlreadySuccessor(GraphNode *Node, GraphNode *Succ) {
  for (auto &it : Node->SuccNodes)
    if (it->Address == Succ->Address)
      return true;
  return false;
}

bool StrippedGraph::isSuccessorLoop(GraphNode *Node) {
  for (auto &it : Node->SuccNodes)
    for (auto &it2 : it->SuccNodes)
      if (it2->Address == Node->Address)
        return true;
  return false;
}

MachineInstr *StrippedGraph::bypassNops(GraphNode *Node) {
  for (MachineBasicBlock::iterator MI = Node->NodeBlock->instr_begin();
       MI != Node->NodeBlock->instr_end(); MI++) {
    unsigned opcode = MI->getOpcode();
    if (Triple.find("i386") != std::string::npos ||
        Triple.find("x86_64") != std::string::npos) {

      if (opcode >= 1817 && opcode <= 1819) // NOOP
        continue;
      if (opcode >= 8451 && opcode <= 8463) // XCH
        continue;
      if (opcode >= 1213 && opcode <= 1216) // LEA
        continue;
    }
    if (Triple.find("arm") != std::string::npos) {
      if (opcode >= 42 && opcode <= 45 &&  // ANDEQ
          MI->getOperand(MI->findFirstPredOperandIdx()).getImm() == 0)
        continue;
      if (opcode >= 244 && opcode <= 245 &&  // MULEQ
          MI->getOperand(MI->findFirstPredOperandIdx()).getImm() == 0)
        continue;
      if (opcode >= 155 && opcode <= 198 &&  // LDREQ

          MI->getOperand(MI->findFirstPredOperandIdx()).getImm() == 0)
        continue;
    }
    return &(*MI);
  }
  return NULL;
}

void StrippedGraph::correctHeadNodes() {
  for (auto &it : HeadNodes) {
    MachineInstr *temp = bypassNops(it);
    if (temp != NULL)
      it->Address = DAS->getDebugOffset(temp->getDebugLoc());
  }
}

bool StrippedGraph::isConditionalTerminator(GraphNode *Node) {
  if (Node->NodeBlock->instr_rbegin()->isPredicable()) {
    int predOpIndex = Node->NodeBlock->instr_rbegin()->findFirstPredOperandIdx();
    if (predOpIndex != -1) {
      int condCode = Node->NodeBlock->instr_rbegin()->getOperand(predOpIndex).getImm();
      if (condCode >= 0 && condCode < 14) {
        //outs() << "Prev Conditional Terminator\n";
        return true;
      }
    }
  }
  return false;
}

bool StrippedGraph::isFunctionBegin(GraphNode *Node) {
  MachineInstr *begin = bypassNops(Node);
  if (Triple.find("arm") != std::string::npos)
    if (begin->getOpcode() >= 412 && begin->getOpcode() <= 417)
      for (MachineInstr::mop_iterator mop = begin->operands_begin();
           mop != begin->operands_end(); ++mop)
        if (mop->isReg() && mop->getReg() == 10)
          return true;
  if (Triple.find("i386") != std::string::npos ||
      Triple.find("x86_64") != std::string::npos)
    if (begin->getOpcode() >= 2227 && begin->getOpcode() <= 2261)
      for (MachineInstr::mop_iterator mop = begin->operands_begin();
           mop != begin->operands_end(); ++mop)
        if (mop->isReg() && (mop->getReg() == 20 || mop->getReg() == 36)
            && !PrevNode->NodeBlock->instr_rbegin()->isConditionalBranch())
          return true;
  return false;
}
} // end namespace fracture
