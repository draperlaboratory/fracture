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
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: January 15, 2014
//===----------------------------------------------------------------------===//

#ifndef TYPERECOVERY_H
#define TYPERECOVERY_H

namespace llvm {

class FunctionPass;
class Pass;

FunctionPass* createTypeRecoveryPass();

///===---------------------------------------------------------------------===//
/// MemList - Represents activity happening around a pointer in memory.
///
/// MemList is tied to a parent source register (global variable) from the
/// fracture IR Emitter.
///
/// If that register gets overwritten or never used as a memory location, it is
/// safe to delete the memlist for that register.
///===---------------------------------------------------------------------===//
class MemListEntry;
class MemList {
private:
  const Value* Register;
  // Note: Entry and Begin are not guaranteed to be the same.
  MemListEntry *Entry, *Begin, *End;
public:
  MemList(const Value* SrcRegister);
};

///===---------------------------------------------------------------------===//
/// MemListEntry - Represents a memory offset and notional entry value.
///
/// This is currently a simple linked list, but should probably get proper
/// accessors/operators/etc if this strategy works to do what we want.
///===---------------------------------------------------------------------===//
class MemListEntry {
private:
  // Pointers to previous/next entries in the memory space. These need not be
  // contiguous.
  // TODO: Entries can overlap, but we assume they don't right now.
  MemListEntry *Prev, *Next;
  // Offset is either a constant or the result of an instruction.
  Value* Offset;
  // A pointer to the Register or Temporary which represents the value.
  Value* Val;
public:
  MemListEntry(Value *MemOffset, Value* MemValue)
    : Offset(MemOffset), Val(MemValue) { };
  MemListEntry* getPrev() { return Prev; };
  MemListEntry* getNext() { return Next; };
  void setPrev(MemListEntry* PrevPtr) { Prev = PrevPtr; };
  void setNext(MemListEntry* NextPtr) { Next = NextPtr; };
};

} // End namespace llvm


#endif
