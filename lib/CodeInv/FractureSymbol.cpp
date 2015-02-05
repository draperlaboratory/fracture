//===--- FractureSymbol - Extended Class from LLVM SymbolRef -----*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class inherits from llvm::object::SymbolRef with the intention of adding
// data to existing symbols or creating symbols in stripped binaries. 
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/FractureSymbol.h"

using namespace llvm;

namespace fracture {



std::error_code FractureSymbol::getAddress(uint64_t &Result) const {
  if (Address == 0) {
    this->object::SymbolRef::getAddress(Result);
    if (Result == object::UnknownAddressOrSize)
      Result = 0;
  }
  else
    Result = Address;
  return object::object_error::success;
}

std::error_code FractureSymbol::getName(StringRef &Result) const {
  if (Name.empty())
    this->object::SymbolRef::getName(Result);
  else
    Result = Name;
  return object::object_error::success;
}

std::error_code FractureSymbol::getAlignment(uint32_t &Result) const {
  if (Alignment != 0)
    this->object::SymbolRef::getAlignment(Result);
  else
    Result = Alignment;
  return object::object_error::success;
}

std::error_code FractureSymbol::getType(object::SymbolRef::Type &Result) const {
  if (Type != ST_Function)
    this->object::SymbolRef::getType(Result);
  else
    Result = Type;
  return object::object_error::success;
}

std::error_code FractureSymbol::getSize(uint64_t &Result) const {
  if (Size != 0)
    this->object::SymbolRef::getSize(Result);
  else
    Result = Size;
  return object::object_error::success;
}

void FractureSymbol::matchAddress(std::map<StringRef, uint64_t> Rels) {
  for (auto const &it : Rels) {
    StringRef Name;
    this->object::SymbolRef::getName(Name);
    if (it.first == Name) {
      Address = it.second;
      return;
    }
  }
  Address = 0;
}

void FractureSymbol::setAddress(uint64_t Addr) {
  Address = Addr;
}

void FractureSymbol::setName(StringRef name){
  Name = name;
}

} // end namespace fracture
