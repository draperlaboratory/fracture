//===--- TypeRecovery - recovers function parameters and locals -*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This function pass looks at LLVM IR from the Decompiler and tries to
// recover the function parameter types and return type of the function.
//
// Works only at the function level. It may be necessary to make this global
// across the entire decompiled program output.
//
// NOTE: Does not do anything useful yet. Placeholder for now.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: January 15, 2014
//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "Transforms/TypeRecovery.h"

#include <list>

using namespace llvm;

namespace {

struct TypeRecovery : public FunctionPass {
  static char ID;
  TypeRecovery() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F);

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
}; // end of struct TypeRecovery

} // end anonymous namespace

char TypeRecovery::ID = 0;

static RegisterPass<TypeRecovery> X("TypeRecovery", "Type recovery pass",
                                    false /* Only looks at CFG */,
                                    false /* Analysis Pass */);

FunctionPass* llvm::createTypeRecoveryPass() {
  return new TypeRecovery();
}

bool TypeRecovery::runOnFunction(Function &F) {
  if (F.isDeclaration()) {
    return false;
  }

  uint64_t NumStores = 0;
  for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      if (isa<LoadInst>(I) || isa<BinaryOperator>(I)) {
        NumStores++;
      }
    }
  }

  outs() << "Num Loads and Stores: " << NumStores << "\n";


  return false;
}

//===----------------------------------------------------------------------===//
// BEGIN MEMLIST IMPL
//===----------------------------------------------------------------------===//

MemList::MemList(const Value* SrcRegister) : Register(SrcRegister) {
  // Do nothing
}
