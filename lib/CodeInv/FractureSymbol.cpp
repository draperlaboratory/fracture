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
  this->object::SymbolRef::getAddress(Result);
  if (Result == object::UnknownAddressOrSize)
    Result = Address;
  return object::object_error::success;
  
}

std::error_code FractureSymbol::getName(StringRef &Result) const {
  this->object::SymbolRef::getName(Result);
  if (Result == "")
    Result = Name;
  return object::object_error::success;
}

void FractureSymbol::matchAddress(std::map<StringRef, uint64_t> Rels) {
  for (auto const &it : Rels) {
    StringRef Name;
    this->object::SymbolRef::getName(Name);
    if (it.first == Name) {
      Address = it.second;
      break;
    }
  }
}

void FractureSymbol::setAddress(uint64_t Addr) {
  Address = Addr;
}

} // end namespace fracture
