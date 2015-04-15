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

#ifndef STRIPPEDGRAPH_H
#define STRIPPEDGRAPH_H


#include "llvm/Object/ObjectFile.h"
#include "llvm/Target/TargetInstrInfo.h"
#include <list>
#include <vector>

using namespace llvm;

namespace fracture {

enum class Color { GRAY, BLACK };

struct GraphNode {
    MachineBasicBlock *NodeBlock;
    uint64_t Address, End, BranchAddress = 0;
    Color NodeColor;
    std::vector<GraphNode *> SuccNodes;
};

class StrippedGraph {
  public:
    StrippedGraph(Disassembler *D, std::string T) {
      DAS = D;
      Triple = T;
    }
    ~StrippedGraph() {
      for (auto &it : AllNodes)
        delete it;
    }
    void addGraphNode(GraphNode *Node);
    void addToList(GraphNode *Node);
    void printGraph();
    std::vector<GraphNode *> getHeadNodes();
    void correctHeadNodes();

  private:
    Disassembler *DAS;
    std::string Triple;
    std::vector<GraphNode *> HeadNodes;
    std::vector<GraphNode *> NeedsLink;
    std::list<GraphNode *> AllNodes;
    GraphNode *PrevNode;
    bool AlreadyASuccessor;
    void resolveLinks();
    void nodeVisit(std::vector<GraphNode *>::iterator NodeIt, GraphNode *ToAdd);
    void printNode(GraphNode *Node);
    void printVisit(GraphNode *Node);
    bool isAddressInBasicBlock(uint64_t Address, GraphNode *Node);
    bool isJumpToCompleteFunction(GraphNode *Node);
    void initializeColors();
    bool isAlreadySuccessor(GraphNode *Node, GraphNode *Succ);
    bool isSuccessorLoop(GraphNode *Node);
    MachineInstr *bypassNops(GraphNode *Node);
    bool isConditionalTerminator(GraphNode *Node);
    bool isFunctionBegin(GraphNode *Node);

};
}






#endif /* STRIPPEDGRAPH_H */
