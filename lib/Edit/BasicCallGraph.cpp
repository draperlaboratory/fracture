//===--- BasicCallGraph.cpp - BasicCallGraph ----------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Wrapper for a call graph representation of a .ll file.
//
// Author: lqb4061
// Date: May 9, 2013
//
//===----------------------------------------------------------------------===//

#include "Edit/BasicCallGraph.h"

BasicCallGraph::BasicCallGraph(Module &M) : CallGraph(M),
  Root(0), ExternalCallingNode(0), CallsExternalNode(0) {

  ExternalCallingNode = getOrInsertFunction(0);
  CallsExternalNode = new CallGraphNode(0);
  Root = 0;
}

void BasicCallGraph::viewFunctionInDotty(const Function *F) {
  F->viewCFG();
}

void BasicCallGraph::addToCallGraph(Function *F) {
  CallGraphNode *Node = getOrInsertFunction(F);

  // If this function has external linkage, anything could call it.
  if (!F->hasLocalLinkage()) {
    ExternalCallingNode->addCalledFunction(CallSite(), Node);

    // Found the entry point?
    if (F->getName() == "main") {
      if (Root)    // Found multiple external mains?  Don't pick one.
        Root = ExternalCallingNode;
      else
        Root = Node;          // Found a main, keep track of it!
    }
  }

  // If this function has its address taken, anything could call it.
  if (F->hasAddressTaken())
    ExternalCallingNode->addCalledFunction(CallSite(), Node);

  // If this function is not defined in this translation unit, it could call
  // anything.
  if (F->isDeclaration() && !F->isIntrinsic())
    Node->addCalledFunction(CallSite(), CallsExternalNode);

  // Look for calls by this function.
  for (Function::iterator BB = F->begin(), BBE = F->end(); BB != BBE; ++BB)
    for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE;
        ++II) {
      CallSite CS(cast<Value>(II));
      if (CS) {
        const Function *Callee = CS.getCalledFunction();
        if (!Callee)
          // Indirect calls of intrinsics are not allowed so no need to check.
          Node->addCalledFunction(CS, CallsExternalNode);
        else if (!Callee->isIntrinsic())
          Node->addCalledFunction(CS, getOrInsertFunction(Callee));
      }
    }
}

void BasicCallGraph::destroy() {
  /// CallsExternalNode is not in the function map, delete it explicitly.
  if (CallsExternalNode) {
    // CallsExternalNode->allReferencesDropped();
    delete CallsExternalNode;
    CallsExternalNode = 0;
  }
  // CallGraph::destroy();
}

Instruction* getInstructionByName(StringRef InstName, BasicBlock *BB) {
  for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (I->getName() == InstName)
      return I;
  }

  return BB->end();
}

BasicBlock* getBasicBlockByName(StringRef BBName, Function *Func) {
  for (Function::iterator I = Func->begin(), E = Func->end(); I != E; ++I) {
    if (I->getName() == BBName)
      return I;
  }

  return Func->end();
}

void BasicCallGraph::view() {
  Module &M = getModule();

  // View every function in the call graph.
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    viewFunctionInDotty(I);
}

void BasicCallGraph::print(raw_ostream &OS) const {
  CallGraph::print(OS);
}
