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

StrippedGraph::StrippedGraph(GraphNode *Head) {
  GraphNode *temp = new GraphNode;
  *temp = *Head;
  HeadNodes.push_back(temp);
}

void StrippedGraph::addGraphNode(GraphNode *Node) {
  outs() << "Initial NeedsLink: ";
  const char *Fmt;
  Fmt = "%08" PRIx64;
  for (auto &it : NeedsLink)
    outs() << format(Fmt, it->Address) << " ";
  outs() << "\n";
  // If the graph is empty, start a new node with the first basic block.
  if (HeadNodes.empty()) {
    PrevNode = Node;
    HeadNodes.push_back(Node);
    return;
  }
  // If there are links that need to be resolved, check the link branch address
  // with the current node to see if they match up, and if so, link them.
  if (!NeedsLink.empty()) {
    for (std::vector<GraphNode *>::iterator it = NeedsLink.begin();
         it != NeedsLink.end(); ++it) {
      if ((*it)->BranchAddress == Node->Address) {
        (*it)->SuccNodes.push_back(Node);
        NeedsLink.erase(it);
        break;
      }
    }
  }
  // If the previous basic block ends with a return instruction, and we've already
  // checked that the current basic block isn't jumped to from somewhere, then
  // the current basic block must be the start of a new function.
  else if ((PrevNode != NULL && PrevNode->NodeBlock->instr_rbegin()->isReturn()) ||
           isJumpToCompleteFunction(PrevNode)) {
    outs() << "Prev is Terminator\n";
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
  if (Node->NodeBlock->instr_rbegin()->isUnconditionalBranch())
    NeedsLink.push_back(Node);
  if (!NeedsLink.empty()) {
    resolveLinks();
  }
  PrevNode = Node;
  outs() << "NeedsLink: ";
  for (auto &it : NeedsLink)
    outs() << format(Fmt, it->Address) << " ";
  outs() << "\n";
  return;
}

void StrippedGraph::addToList(GraphNode *Node) {
  AllNodes.push_back(Node);
}

/*void StrippedGraph::resolveLinks() {
  for (std::vector<GraphNode *>::iterator it = NeedsLink.begin();
       it != NeedsLink.end(); ++it) {
    initializeColors();
    for (auto &git : HeadNodes) {
      //if (AlreadyASuccessor)
      //  return;
       it = nodeVisit(it, git);
    }
  }
}*/

void StrippedGraph::resolveLinks() {
  bool NodeDeleted = false;
  for (std::vector<GraphNode *>::iterator vecit = NeedsLink.begin();
       vecit != NeedsLink.end(); ) {
    outs() << "TESTING NEEDSLINK NODE\n";
    for(auto &it : AllNodes) {
      if (isAddressInBasicBlock((*vecit)->BranchAddress, it) &&
          !isAlreadySuccessor(*vecit, it)) {
        (*vecit)->SuccNodes.push_back(it);
        outs() <<"Link Resolved!!!! AHHHH\n";
        vecit = NeedsLink.erase(vecit);
        NodeDeleted = true;
      }
    }
    if(!NodeDeleted)
      vecit++;
    NodeDeleted = false; 
  }
}
       

/*std::vector<GraphNode *>::iterator StrippedGraph::nodeVisit(std::vector<GraphNode *>::iterator NodeIt, GraphNode *ToAdd) {
  const char *Fmt;
  Fmt = "%08" PRIx64;
  AlreadyASuccessor = false;

  if (ToAdd->NodeColor == Color::GRAY &&
      isAddressInBasicBlock((*NodeIt)->BranchAddress, ToAdd) &&
      !isAlreadySuccessor(*NodeIt, ToAdd)) {
    (*NodeIt)->SuccNodes.push_back(ToAdd);
    NodeIt = NeedsLink.erase(NodeIt);
  }
  for (auto &it : ToAdd->SuccNodes) {
    //if (AlreadyASuccessor)
    //  return;
    nodeVisit(NodeIt, it);
    
  }
  ToAdd->NodeColor = Color::BLACK;
  return NodeIt;
}*/

void StrippedGraph::printGraph() {
  initializeColors();
  for (auto &it : HeadNodes) {
    outs() << "Function Begin!\n";
    printVisit(it);
  }
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
}

void StrippedGraph::printVisit(GraphNode *Node) {
  if(Node->NodeColor == Color::GRAY) {
    printNode(Node);
    for (auto &it : Node->SuccNodes)
      printVisit(it);
    Node->NodeColor = Color::BLACK;
  }
}

bool StrippedGraph::isAddressInBasicBlock(uint64_t Address, GraphNode *Node) {
  if (Address >= Node->Address && Address <= Node->End)
    return true;
  return false;
}

bool StrippedGraph::isJumpToCompleteFunction(GraphNode *Node) {
  for(auto &it : HeadNodes)
    if(isAddressInBasicBlock(Node->BranchAddress, it))
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






} // end namespace fracture
