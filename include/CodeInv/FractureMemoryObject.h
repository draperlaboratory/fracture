//===--- FractureMemoryObject - Section memory holder -----------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class reimplements StringRefMemoryObject from LLVM 3.5.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: January 3rd, 2015
//===----------------------------------------------------------------------===//


#ifndef FRACTUREMEMORYOBJECT_H
#define FRACTUREMEMORYOBJECT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/MemoryObject.h"

using namespace llvm;

namespace fracture {

/// StringRefMemoryObject - Simple StringRef-backed MemoryObject
class FractureMemoryObject : public MemoryObject {
  StringRef Bytes;
  uint64_t Base;
public:
  FractureMemoryObject(StringRef Bytes, uint64_t Base = 0) :
    Bytes(Bytes), Base(Base) {};

  uint64_t getBase() const { return Base; }
  uint64_t getExtent() const { return Bytes.size(); }

  uint64_t readByte(uint8_t *Byte, uint64_t Addr) const;
  uint64_t readBytes(uint8_t *Buf, uint64_t Addr, uint64_t Size) const;

  const uint8_t *getPointer(uint64_t address, uint64_t size) const {
    if (isValidAddress(address)) {
      return ((const uint8_t*)Bytes.data())+(address-Base);
    } else {
      return nullptr;
    }
  };
  bool isValidAddress(uint64_t address) const {
    return ((address-Base) < Bytes.size());
  };

  StringRef getBytes() { return Bytes; };

};

}

#endif
