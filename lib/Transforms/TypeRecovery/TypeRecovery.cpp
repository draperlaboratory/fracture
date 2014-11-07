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

struct I2PInfo {
  std::vector<BinaryOperator*> BinOps;
  LoadInst* StartLoad;
  LoadInst* EndLoad;



  // Generally, the pattern we are looking for is:
  //  Load
  //   BinOp
  //    BinOp
  //     Binop
  //      ...
  //       IntToPtr
  //        Load
  //         Misc. ...
  //          Store
  // We want to turn the BinOps to 2nd load into a GEP instruction.
  // We have to verify:
  //   - Nothing but BinOps between the Load and IntToPtr instruction.
  //   - All BinOps have to result in an aligned offset and divides cleanly by
  //     1, 2, 4, or 8 based on variable size (e.g., i32 load indicates an int)
  //   - Every IntToPtr has to have the same size, otherwise we are dealing with
  //     a struct/class or stack passed variables
  // Note that there may be ops in between which use all or part of the binop
  // e.g., usually when loading from the stack or allocating space the stack
  // pointer gets modified with a binop (and reversed at the end).
  void AnalyzeIntToPtr(IntToPtrInst *IP) {
    // Build up the use chain
    return;
  }

};

void AnalyzeVar(Instruction* Alloca, int depth = 0) {
  if (depth == 0) outs() << ">" << *Alloca << "\n";
  for (Value::use_iterator UI = Alloca->use_begin(), E = Alloca->use_end();
       UI != E; ++UI) {
    Instruction *User = cast<Instruction>(*UI);
    for (unsigned int i = 0, e = depth; i != e; ++i) outs() << "=";
    outs() << "=>" << *User << "\n";
    AnalyzeVar(User, depth+1);

  }
}

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

  BasicBlock *BBEntry = &F.getEntryBlock();
  std::list<Instruction*> WorkList;
  // Note that the last inst in the basic block is a jump
  for (BasicBlock::iterator I = BBEntry->begin(), E = --BBEntry->end();
       I != E; ++I) {
    if (isa<AllocaInst>(I)) {
      // WorkList.push_back(I);
      //AnalyzeVar(I);
    }
  }

  // if (WorkList.empty()) {
  //   break;
  // }

  
  // First, do a pass that converts
  //  (store var1, inttoptr (add var2, *))
  // to:
  //  (store var1, var2[whatever])
  // And also does the same for load patterns.
  // This requires us to convert var2 into a pointer of appropriate size (int*)
  // if 32 bits, (char*) if 8, and so on.
  // That requires some constraint solving, to make sure the offset is a mult.
  // of the bitsize, or it could be a struct pointer with different types in it.

  // Walk the function and record:
  //   - find any vars loaded/used before def, these are parameters of some form
  //   - find the last store/def of all vars

  uint64_t NumStores = 0;
  // For starters, take anything that's offset an add/sub from a register
  // and make it a GEP.
  // TODO: Figure out if this really belongs in the IREmitter.
  for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      IntToPtrInst *I2P = dyn_cast<IntToPtrInst>(I);

      if (I2P == NULL) {
        continue;
      }
      Value* Op1 = I2P->getOperand(0);
      if (isa<LoadInst>(Op1) || isa<BinaryOperator>(Op1)) {
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
