
//===---  StrippedDisassembler.cpp -----*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class attempts to locate and then populate symbols into
// various types of stripped binaries.
//
// Author: Dillon Forzese(dmf1032) <dmf1032@draper.com>
// Date: February 5, 2015
//===----------------------------------------------------------------------===//

#include "CodeInv/StrippedDisassembler.h"
#include "CodeInv/StrippedGraph.h"

using namespace llvm;

namespace fracture {

//Returns address of section
uint64_t StrippedDisassembler::getStrippedSection(std::string section) {

  StringRef SectionNameOrAddress = section;
  const object::ObjectFile* Executable = DAS->getExecutable();
  std::error_code ec;
  uint64_t Address = 0;
  object::SectionRef Section = *Executable->section_end();

  if (SectionNameOrAddress.getAsInteger(0, Address) && Address != 0)
      Section = DAS->getSectionByAddress(Address);

  if (Section == *Executable->section_end())
      Section = DAS->getSectionByName(SectionNameOrAddress);

  if (Section == *Executable->section_end()) {
    errs() << "Could not find section!\n";
    return 0;
  }

  Address = Section.getAddress();
  return Address;
}

//Returns address in hex
uint64_t StrippedDisassembler::getHexAddress(MachineBasicBlock::iterator II) {

  std::string mn;
  uint64_t address;
  std::stringstream sin;
  unsigned Address = DAS->getDebugOffset(II->getDebugLoc());
  unsigned Size = II->getDesc().getSize();
  uint8_t *Bytes = new uint8_t(Size);
  DAS->getCurSectionMemory()->readBytes(Bytes, Address, Size);
  for (unsigned i = (Size/2); i >= 1; --i) {
    sin << std::uppercase << std::hex << static_cast<int>(Bytes[i-1]);
    mn.append(sin.str());
    sin.str("");
  }
  std::stringstream ss;
  ss << std::hex << mn;
  ss >> address;

  delete Bytes;
  return address;

}
//Iterate through basic blocks of a section and push them to the graph
void StrippedDisassembler::functionsIterator(uint64_t Address) {

  MachineFunction *MF = DAS->disassemble(Address);
  object::SectionRef Section = DAS->getSectionByAddress(Address);
  DAS->setSection(Section);
  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  const char *Fmt;
  Fmt = "%08" PRIx64;

  while (BI != BE
    && DAS->getDebugOffset(BI->instr_begin()->getDebugLoc()) < Address) {
    ++BI;
  }
  if (BI == BE) {
    //outs() << "End of file\n";
    return;
  }
  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();

//Skip to first instruction
  while (DAS->getDebugOffset(II->getDebugLoc()) < Address) {
    if (II == IE) {
      outs() << "Unreachable: reached end of basic block when looking for first"
        " instruction.";
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
    ++II;
  }
  if (Address != DAS->getDebugOffset(II->getDebugLoc())) {
    outs() << "Warning: starting at " << DAS->getDebugOffset(II->getDebugLoc())
        << " instead of " << Address << ".\n";
  }

  for (; BI != BE; ++BI) {
//Inside a new basic block
    uint64_t ad = DAS->getDebugOffset(BI->instr_begin()->getDebugLoc());
    GraphNode *tempNode = new GraphNode;
    tempNode->NodeBlock = BI;
    tempNode->Address = DAS->getDebugOffset(BI->instr_begin()->getDebugLoc());
    tempNode->End = DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc());

    if (BI->instr_rbegin()->isBranch()) {
      uint64_t InstAddr = DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc());
      uint64_t InstSize = (TripleName.find("arm") != std::string::npos) ? 8
                          : BI->instr_rbegin()->getDesc().getSize();
      uint64_t JumpAddr = 0;

      if (BI->instr_rbegin()->getOperand(0).isImm())
        JumpAddr = BI->instr_rbegin()->getOperand(0).getImm();
      tempNode->BranchAddress = JumpAddr + InstSize + InstAddr;
    }
    Graph->addGraphNode(tempNode);
    Graph->addToList(tempNode);
    ad = DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc());
    if(DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc())
      + BI->instr_rbegin()->getDesc().getSize() >= Section.getAddress() 
      + Section.getSize() - BI->instr_rbegin()->getDesc().getSize())
      return;
  }
  functionsIterator(DAS->getDebugOffset((--BI)->instr_rbegin()->getDebugLoc())
                    + BI->instr_rbegin()->getDesc().getSize());
  return;
}

// Sets mainAddr to the address of main in a stripped file.
//   Iterates through the start of .text and finds the call to main using the
//   standard gcc and clang file preamble.
void StrippedDisassembler::findStrippedMain() {


  uint64_t Address = 0, pre = 0;
  //uint64_t symbAddr = getStrippedSection(".text");
  object::SectionRef curSection = DAS->getSectionByName("text");
  uint64_t symbAddr = curSection.getAddress();
  DAS->setSection(curSection);
  std::ostringstream sin;
  sin << std::hex << symbAddr;
  std::string dis(sin.str());
  dis.insert(0,"0x");
  sin.str("");

  // duplicate using MC
  formatted_raw_ostream Out(outs(), false);
  MachineFunction *MF = DAS->disassemble(symbAddr);
  //object::SectionRef Section = DAS->getSectionByAddress(symbAddr);
  //DAS->setSection(Section);


  MachineFunction::iterator BI = MF->begin(), BE = MF->end();

  while (BI != BE
  && DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < symbAddr) {
    ++BI;
  }
  if (BI == BE) {
    outs() << "Could not disassemble, reached end of function's basic blocks"
             " when looking for first instruction.";
    return;
  }


  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();
  // skip to first instruction
  while (DAS->getDebugOffset(II->getDebugLoc()) < symbAddr) {
    if (II == IE) {
      outs() << "Unreachable: reached end of basic block when looking for first"
                " instruction.";
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
    ++II;
  }
  if (symbAddr != DAS->getDebugOffset(II->getDebugLoc())) {
    outs() << "Warning: starting at " << DAS->getDebugOffset(II->getDebugLoc())
           << " instead of " << symbAddr << ".\n";
  }
  if(TripleName.find("arm") != std::string::npos){
    while (BI != BE) {
      if(II->getOpcode() == 192 || II->getOpcode() == 152){
        ++II;
        ++II;
        if(II->getOpcode() == 449 || II->getOpcode() == 415) {
          while (BI != BE) {
            if(II->getOpcode() == 197 && pre == 197){
              for(int x = 0; x < 4; x++)
                ++II;
              Address = getHexAddress(II);
              outs() << "ARM main address is: " << Address << "\n";
              mainAddr = Address;
              return;
            }

            pre = II->getOpcode();
            ++II;

          }
        }
      }

      ++II;
      if (II == IE) {
        ++BI;
        II = BI->instr_begin();
        IE = BI->instr_end();
      }
    }
  }

  else if((TripleName.find("i386") != std::string::npos) || (TripleName.find("x86") != std::string::npos)) {
    while(BI != BE) {
      if(II->getOpcode() >= 8472 && II->getOpcode() <= 8511)
        break;
      ++II;
      if (II == IE) {
        ++BI;
        II = BI->instr_begin();
        IE = BI->instr_end();
      }
    }
    while(BI != BE) {
      if(II->getOpcode() >= 353 && II->getOpcode() <= 361) {
          outs() << "Intel main address is: " << pre << "\n";
          mainAddr = pre;
          return;
        }
//32b systems have the address in op0 64b in op1
      if(II->getOperand(0).isImm())
        pre = II->getOperand(0).getImm();
      else if(II->getOperand(1).isImm())
        pre = II->getOperand(1).getImm();
      ++II;
    }
  }
  outs() << "No success finding main RETURNING\n";
   return;
}

void StrippedDisassembler::addSymbol(FractureSymbol tempSym) {
  Symbols.push_back(tempSym);
}
std::vector<FractureSymbol> StrippedDisassembler::getSymbolVector() {
  return Symbols;
}

StrippedGraph *StrippedDisassembler::getStrippedGraph() {
  return Graph;
}
uint64_t StrippedDisassembler::getMain(){
return mainAddr;
}

} // end namespace fracture
