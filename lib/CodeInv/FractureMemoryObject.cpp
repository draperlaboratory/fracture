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

#include "CodeInv/FractureMemoryObject.h"

using namespace llvm;
using namespace fracture;

uint64_t FractureMemoryObject::readByte(uint8_t *Byte, uint64_t Addr) const {
  if (Addr >= Base + getExtent() || Addr < Base)
    return -1;
  *Byte = Bytes[Addr - Base];
  return 0;
}

uint64_t FractureMemoryObject::readBytes(uint8_t *Buf,
                                         uint64_t Addr,
                                         uint64_t Size) const {
  uint64_t Offset = Addr - Base;
  if (Addr >= Base + getExtent() || Offset + Size > getExtent() || Addr < Base)
    return -1;
  memcpy(Buf, Bytes.data() + Offset, Size);
  return 0;
}
