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
#include "llvm/IR/Constants.h"
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

  // Map Src Registers to Memory lists
  std::map<Value*, MemList*> MemLists;
  std::vector<Value*> SrcRegs;
  std::vector<Value*> NonIntOffsets;
  std::vector<int64_t> IntOffsets;

  for (Function::iterator BB = F.begin(), BBE = F.end(); BB != BBE; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      // When we see an I2P instruction, we (potentially) create a new MemList,
      // then we will calculate the offset of the source register.
      if (isa<IntToPtrInst>(I)) {
        Value *SrcReg, *OffsetVal;
        Value *Cur = I;
        OffsetVal = NULL;
        int64_t IntOffset = 0;
        // Loop till we see the global, but while we are there calculate offsets
        while(Cur != NULL && !isa<GlobalVariable>(Cur)
          && isa<Instruction>(Cur)) {
          Instruction* CurInst = dyn_cast<Instruction>(Cur);
          Cur = CurInst->getOperand(0);
          if (OffsetVal != NULL) {
            continue;
          }
          if (CurInst->getNumOperands() < 2) {
            continue;
          }
          // Note: assuming 2nd op is always the constant and 1st is always
          // leading to the register.
          if (!isa<ConstantInt>(CurInst->getOperand(1))) {
            outs() << "Non-Constant Value!!: " << *CurInst << "\n";
            OffsetVal = CurInst->getOperand(1);
            continue;
          }
          int64_t Tmp = dyn_cast<ConstantInt>(
            CurInst->getOperand(1))->getSExtValue();
          switch (CurInst->getOpcode()) {
            case Instruction::Add:
              outs() << "Add!\n";
              break;
            case Instruction::Sub:
              outs() << "Sub!\n";
              Tmp *= -1;
              break;
            default:
              outs() << "Unknown opcode type! " << *CurInst << "\n";
              break;
          }
          IntOffset += Tmp;
          outs() << "IntOffset=" << IntOffset << "\n";
        }
        if (Cur != NULL && isa<GlobalVariable>(Cur)) {
          SrcReg = Cur;
          SrcRegs.push_back(Cur);
        }
        if (OffsetVal == NULL) {
          IntOffsets.push_back(IntOffset);
        } else {
          NonIntOffsets.push_back(OffsetVal);
        }
      }
    }
  }

  outs() << "SRC REGS: \n";
  for (int i = 0, e = SrcRegs.size(); i != e; ++i) {
    outs() << *SrcRegs[i] << "\n";
  }
  outs() << "Non-Int Offsets: \n";
  for (int i = 0, e = NonIntOffsets.size(); i != e; ++i) {
    outs() << *NonIntOffsets[i] << "\n";
  }
  outs() << "Int Offsets: \n";
  for (int i = 0, e = IntOffsets.size(); i != e; ++i) {
    outs() << IntOffsets[i] << "\n";
  }


  return false;
}

//===----------------------------------------------------------------------===//
// BEGIN MEMLIST IMPL
//===----------------------------------------------------------------------===//

MemList::MemList(const Value* SrcRegister) : Register(SrcRegister) {
  // Do nothing
}
