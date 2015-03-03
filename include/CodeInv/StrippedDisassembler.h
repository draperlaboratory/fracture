
//===---  StrippedDisassembler.h -----*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class attempts to locate and then populate symbols into
// various types of stripped binaries. Currently it searches for main
// and other functions called by it.
//
// Author: Dillon Forzese(dmf1032) <dmf1032@draper.com>
// Date: February 5, 2015
//===----------------------------------------------------------------------===//

#ifndef STRIPPEDDISASSEMBLER_H_
#define STRIPPEDDISASSEMBLER_H_


#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "CodeInv/MCDirector.h"
#include "CodeInv/FractureSymbol.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"



#include <sstream>
#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <stdio.h>
#include <algorithm>
#include <map>
#include <inttypes.h>
#include <signal.h>
#include <sstream>
#include <unistd.h>
#include <cstdlib>

using namespace llvm;

namespace fracture{

  class StrippedDisassembler{
  public:
    StrippedDisassembler() {}
    StrippedDisassembler(Disassembler *D, std::string T) {

      TripleName = T;
      DAS = D;
    }

    bool opcodeCheck(int opc);
    uint64_t getHexAddress(MachineBasicBlock::iterator II);
    uint64_t getStrippedSection(std::string section);
    void findStrippedFunctions(uint64_t Address);
    uint64_t findStrippedMain();
    void addSymbol(FractureSymbol S);
    std::vector<FractureSymbol> getSymbolVector();

  private:
    Disassembler *DAS;
    std::string TripleName;
    std::vector<FractureSymbol> Symbols;
    struct inst {
      enum instructonType {call, branch, ret, jump};
      uint64_t Addr;
    };

  };
}


#endif /* STRIPPEDDISASSEMBLER_H_ */
