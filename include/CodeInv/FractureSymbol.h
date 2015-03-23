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

#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"

#include <map>

using namespace llvm;

namespace fracture {

class FractureSymbol : public object::SymbolRef {
  public:
    FractureSymbol() {}
    FractureSymbol(uint64_t A,
                   StringRef N,
                   uint32_t Al,
                   SymbolRef::Type T,
                   uint64_t S) {

      Address = A;
      Name = N;
      Alignment = Al;
      Type = T;
      Size = S;
    }
    // Constructs a FractureSymbol using an existing SymbolRef
    FractureSymbol(const object::SymbolRef &f) : object::SymbolRef(f) {}

    // Method overrides
    std::error_code getAddress(uint64_t &Result) const;
    std::error_code getName(StringRef &Result) const;
    std::error_code getAlignment(uint32_t &Result) const;
    std::error_code getType(SymbolRef::Type &Result) const;
    std::error_code getSize(uint64_t &Result) const;


    // matchAddress() sets the address of a dynamic FractureSymbol by 
    // pairing the symbol name with the original call instruction offset
    void matchAddress(std::map<StringRef, uint64_t> Rels);

    // Getters and setters
    void setAddress(uint64_t Addr);

    void setName(StringRef name);

    uint64_t getadd() {
      return Address;
    }
    StringRef getN() {
      return Name;
    }

  private: 
    uint64_t Address = 0;
    StringRef Name ;
    uint32_t Alignment;
    SymbolRef::Type Type;
    uint64_t Size;
};

} // end namespace fracture
