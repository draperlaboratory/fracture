//===--- BasicCallGraph.h - Disassembler ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief Wrapper for a call graph. Represents the call graph representation
/// of a .ll file.
///
/// Author: lqb4061
/// Date: Aug 20, 2013
///
//===----------------------------------------------------------------------===//

#ifndef BASICCALLGRAPH_H_
#define BASICCALLGRAPH_H_

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

/// \brief An abstraction for a CFG.
class BasicCallGraph: public CallGraph {
  // Root is root of the call graph, or the external node if a 'main' function
  // couldn't be found.
  CallGraphNode *Root;

  // ExternalCallingNode - This node has edges to all external functions and
  // those internal functions that have their address taken.
  CallGraphNode *ExternalCallingNode;

  // CallsExternalNode - This node has edges to it from all functions making
  // indirect calls or calling an external function.
  CallGraphNode *CallsExternalNode;
private:
  //===---------------------------------------------------------------------
  // Implementation of CallGraph construction
  //

  // addToCallGraph - Add a function to the call graph, and link the node to all
  // of the functions that it calls.
  //
  void addToCallGraph(Function *F);

  //
  // destroy - Release memory for the call graph
  virtual void destroy();

  void viewFunctionInDotty(const Function *F);

public:
  BasicCallGraph(Module &M);

  void view();

  bool containsFunc(const Function *F);

  virtual void print(raw_ostream &OS) const;

  virtual void releaseMemory() {
    destroy();
  }

  CallGraphNode* getExternalCallingNode() const {
    return ExternalCallingNode;
  }
  CallGraphNode* getCallsExternalNode() const {
    return CallsExternalNode;
  }

// getRoot - Return the root of the call graph, which is either main, or if
// main cannot be found, the external node.
//
  CallGraphNode *getRoot() {
    return Root;
  }
  const CallGraphNode *getRoot() const {
    return Root;
  }
};

#endif /* CFGEDITOR_H_ */
