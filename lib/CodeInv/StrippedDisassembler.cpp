
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
// various types of stripped binaries. Currently it searches for main
// and other functions called by it.
//
// Author: Dillon Forzese(dmf1032) <dmf1032@draper.com>
// Date: February 5, 2015
//===----------------------------------------------------------------------===//

#include "CodeInv/StrippedDisassembler.h"
#include "CodeInv/StrippedGraph.h"

using namespace llvm;

namespace fracture {

uint64_t StrippedDisassembler::getStrippedSection(std::string section) {

  StringRef SectionNameOrAddress = section;
  const object::ObjectFile* Executable = DAS->getExecutable();
  std::error_code ec;
  uint64_t Address;
  object::SectionRef Section = *Executable->section_end();

  if (SectionNameOrAddress.getAsInteger(0, Address) && Address != 0)
   Section = DAS->getSectionByAddress(Address);

  if (Section == *Executable->section_end())
    Section = DAS->getSectionByName(SectionNameOrAddress);

  if (Section == *Executable->section_end()) {
    errs() << "Could not find section!\n";
    return 0;
  }

  if (std::error_code(Section.getAddress(Address)))
    return 0;

  //StringRef SectionName;
  //std::error_code(Section.getName(SectionName));
  return Address;
}

uint64_t StrippedDisassembler::getHexAddress(MachineBasicBlock::iterator II) {
  //Returns location of iterator. converts from int > hex > int sheeesh.
  std::string mn;
  uint64_t address;
  std::stringstream sin;
  unsigned Address = DAS->getDebugOffset(II->getDebugLoc());
  unsigned Size = II->getDesc().getSize();
  uint8_t *Bytes = new uint8_t(Size);
  DAS->getCurSectionMemory()->readBytes(Address, Size, Bytes);
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

/*NodeType StrippedDisassembler::opcodeCheck(int opc, MachineBasicBlock::iterator II) {
  //ARM
  MachineBasicBlock::iterator IX = II;
  if(TripleName.find("arm") != std::string::npos) {
    if(opc >= 45 && opc <= 70)
      return NodeType::BRANCH;
  }
  //x86
  else if((TripleName.find("i386") != std::string::npos) || (TripleName.find("x86_64") !=std::string::npos)) {
    if(opc >= 1063 && opc <= 1123){
      //outs() << "BRANCH " << DAS->getDebugOffset(II->getDebugLoc()) <<"\n";
      return NodeType::BRANCH;
    }

    else if(opc >= 2313 && opc <= 2318){
      //outs() << "FUNCEND " << DAS->getDebugOffset(II->getDebugLoc()) <<"\n";
      return NodeType::FUNCEND;
    }

    else if((opc >= 2186 && opc <= 2220 ) && II->getOperand(0).isReg()){
      //outs() << "Possible FUNCBEGIN OPERAND 0 is " << II->getOperand(0).getReg()
      //       << "OPC " << opc << "\n";
      ++IX;
      opc = IX->getOpcode();
      if((opc >= 1535 && opc <= 1721) && (IX->getOperand(0).isReg() && IX->getOperand(1).isReg())){
        //outs() << "FUNCBEGIN FOUND at " << DAS->getDebugOffset(II->getDebugLoc()) << "\n";
        return NodeType::FUNCBEGIN;
      }
    }
  }
  //outs() << "UNKNOWN " << DAS->getDebugOffset(II->getDebugLoc()) <<"\n";
  return NodeType::UNKNOWN;
}*/

void StrippedDisassembler::functionsIterator(uint64_t Address) {
  //print out stripped function locations
  MachineFunction *MF = DAS->disassemble(Address);
  object::SectionRef Section = DAS->getSectionByAddress(Address);
  DAS->setSection(Section);
  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  //NodeType T;
  //uint64_t entry = 0, exit = 0;

  const char *Fmt;
  Fmt = "%08" PRIx64;

  while (BI != BE
    && DAS->getDebugOffset(BI->instr_begin()->getDebugLoc()) < Address) {
    ++BI;
  }
  if (BI == BE) {
    outs() << "Could not disassemble :( reached end of function's basic blocks"
      " when looking for first instruction.\n";
    return;
    //TODO find a way to fail gracefully
  }
  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();

  // skip to first instruction
  while (DAS->getDebugOffset(II->getDebugLoc()) < Address) {
    if (II == IE) {
      outs() << "Unreachable: reached end of basic block when looking for first"
        " instruction.";
      //broke
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

  //loop through the BB to find the first jump or branch


  for (; BI != BE; ++BI) {
  //while (BI != BE ) {
      outs() << "New BasicBlock!: \n";
      uint64_t ad = DAS->getDebugOffset(BI->instr_begin()->getDebugLoc());
      outs() << "Begin:\t";
      outs() << format(Fmt, ad);
      outs() << "\n";
      GraphNode *tempNode = new GraphNode;
      tempNode->NodeBlock = BI;
      tempNode->Address = DAS->getDebugOffset(BI->instr_begin()->getDebugLoc());
      tempNode->End = DAS->getDebugOffset(BI->instr_end()->getPrevNode()->getDebugLoc());
      if (BI->instr_end()->getPrevNode()->isBranch()) {
        uint64_t InstAddr = DAS->getDebugOffset(BI->instr_end()->getPrevNode()->getDebugLoc());
        uint64_t InstSize = (TripleName.find("arm") != std::string::npos) ? 8
                          : BI->instr_end()->getPrevNode()->getDesc().getSize();
        uint64_t JumpAddr = 0;
        if (BI->instr_end()->getPrevNode()->getOperand(0).isImm())
          JumpAddr = BI->instr_end()->getPrevNode()->getOperand(0).getImm();
        tempNode->BranchAddress = JumpAddr + InstSize + InstAddr;
      }
      Graph->addGraphNode(tempNode);
      Graph->addToList(tempNode);
    //for(II = BI->instr_begin(), IE = BI->instr_end(); II != IE; ++II) {
      //Wanted to use II->isCall() but this failed often
      // 45-70 arm   1063-1123 x86
      //outs() <<"Calling GetOpcode\n";
      //int opc = II->getOpcode();
      //outs() <<"Post GetOpcode\n";
      //T = opcodeCheck(opc, II);
      //if(T == NodeType::BRANCH && entry == 0){
        //outs() << "Branch with no prev head\n";
        //++II;
        //continue;
      //}
      //else if (T != NodeType::UNKNOWN) {
        //outs() << "NodeType Catch\n";
      //  if(entry == 0)
      //    entry = DAS->getDebugOffset(II->getDebugLoc());
      //  else
      //    exit = DAS->getDebugOffset(II->getDebugLoc());
        /*if(entry != 0 && exit != 0) {
          temp = new GraphNode;
          temp->Type = T;
          temp->Address = entry;
          temp->End = exit;
          if(T == NodeType::BRANCH)
            temp->BranchAddress = II->getOperand(0).getImm();
          Graph->addGraphNode(temp);
          entry = 0;
          //entry = exit;
        }*/
      //}
      ad = DAS->getDebugOffset(BI->instr_end()->getPrevNode()->getDebugLoc());
      outs() << "End:\t";
      outs() << format(Fmt, ad);
      outs() << "\nOPCODE: " << BI->instr_rbegin()->getOpcode();
      if (BI->instr_rbegin()->isUnconditionalBranch())
        outs() << "\nUNCONDITIONAL\n";
      if (BI->instr_rbegin()->findFirstPredOperandIdx() != -1)
        outs() << "Condit: " << BI->instr_rbegin()->getOperand(BI->instr_rbegin()->findFirstPredOperandIdx()).getImm();
      outs() << "\n\n";
  }
  //outs() << "Calling functionsIterator\n";
  //outs() << DAS->getDebugOffset(IE->getPrevNode()->getDebugLoc()) + IE->getPrevNode()->getDesc().getSize() << "\n";
  //outs() << DAS->getDebugOffset((--BI)->instr_end()->getPrevNode()->getDebugLoc())
   //         + BI->instr_end()->getPrevNode()->getDesc().getSize();
  functionsIterator(DAS->getDebugOffset((--BI)->instr_end()->getPrevNode()->getDebugLoc())
                    + BI->instr_end()->getPrevNode()->getDesc().getSize());
  return;
}



void StrippedDisassembler::findStrippedFunctions(uint64_t Address) {
  //print out stripped function locations
  formatted_raw_ostream Out(outs(), false);
  MachineFunction *MF = DAS->disassemble(Address);
  object::SectionRef Section = DAS->getSectionByAddress(Address);
  DAS->setSection(Section);
  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  int offset = 0;

  while (BI != BE
    && DAS->getDebugOffset(BI->instr_begin()->getDebugLoc()) < Address) {
    ++BI;
  }
  if (BI == BE) {
    outs() << "Could not disassemble :( reached end of function's basic blocks"
      " when looking for first instruction.";
      //DIE
  }
  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();

  // skip to first instruction
  while (DAS->getDebugOffset(II->getDebugLoc()) < Address) {
    if (II == IE) {
      outs() << "Unreachable: reached end of basic block when looking for first"
        " instruction.";
      //broke
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

  //Here we have two loops that will find the first selection block which
  //points to the next selection block and the first function and so on.

  while (II != IE ){
    //Wanted to use II->isCall() but this failed often
    if(II->getOpcode() == 70)
      break;
    else if(II->getOpcode() == 65 || II->getOpcode() == 68){
        outs() << "END OF Functions\n";
            return;
    }

    ++II;
    }
    outs() << "First Catch in loop " << II->getOperand(0) << " : " << II->getOperand(1) << " : "
           << DAS->getDebugOffset(II->getDebugLoc()) << "\n";

  //Take the first operand and add it to the address.
  if (II->getOperand(0).isImm()) {
    //adding offset + 8 to address.
    offset = II->getOperand(0).getImm();
    offset += 8;
    Address = DAS->getDebugOffset(II->getDebugLoc());
    offset += Address;
  }
  else {
    outs() << "Wrong Opcode found\n";
    return;
      //DIE
  }
  //into the selection block
  outs() << "Beginning of the selection block " << offset << "\n";
  MF = DAS->disassemble((unsigned(offset)));
  BI = MF->begin(), BE = MF->end();

  while (BI != BE
    && DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < (unsigned(offset))) {
    ++BI;
  }
  if (BI == BE) {
    outs() << "Could not disassemble selection block, reached end of function's basic blocks"
        " when looking for first instruction.\n";
    return;
        //DIE
  }

  II = BI->instr_begin();
  IE = BI->instr_end();
  //Find first branch in block, (bls) this leads to next block before function
  while(II != IE){
    if(II->getOpcode() == 70)
    break;
    ++II;
  }
  if(Address == DAS->getDebugOffset(II->getDebugLoc())){
      outs()<< "Branch is calling its own block. Exiting\n";
      return;
  }
  outs() << "Branch function Operands " << II->getOperand(0) << " : " << II->getOperand(1) << " : "
             << DAS->getDebugOffset(II->getDebugLoc()) << "\n";
  offset = II->getOperand(0).getImm();
  offset += 8;
  Address = DAS->getDebugOffset(II->getDebugLoc());
  uint64_t nextCall = DAS->getDebugOffset(II->getDebugLoc());
  offset += Address ;
  outs() << "Final loc = " << offset << "\n";
  //First branch here leads to the function
  MF = DAS->disassemble((unsigned)offset);
  BI = MF->begin(), BE = MF->end();

    while (BI != BE
      && DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < (unsigned(offset))) {
      ++BI;
    }
    if (BI == BE) {
      outs() << "Could not disassemble selection block, reached end of function's basic blocks"
          " when looking for first instruction.\n";
      return;
          //DIE
    }

    II = BI->instr_begin();
    IE = BI->instr_end();
    //Find first branch in block, (bls) this leads to function
    while(II != IE){
      if(II->getOpcode() == 70 || II->getOpcode() == 55)
      break;
      outs() << "addr= " << DAS->getDebugOffset(II->getDebugLoc()) << " : " << II->getOpcode() << "\n";
      ++II;
    }

  outs() << "Actual Function Operands " << II->getOperand(0) << " : " << II->getOperand(1) << " : "
         << DAS->getDebugOffset(II->getDebugLoc()) << "\n";
  offset = II->getOperand(0).getImm();
  offset += 8;
  Address = DAS->getDebugOffset(II->getDebugLoc());
  offset += Address ;
  Address = (unsigned(offset));
  outs() << "Function address is " << Address << "\n\n";
  FractureSymbol tempSym(Address, DAS->getFunctionName(Address), 0, object::SymbolRef::Type::ST_Function, 0);
  addSymbol(tempSym);

  //While we are not yet at the end the file, or have any possibility of finding new functions

  findStrippedFunctions(nextCall);




}

uint64_t StrippedDisassembler::findStrippedMain() {

          //int offset = 0x14;
          int pre;
          //char tArr[100];
          uint64_t Address;
          unsigned Size = 0;
          std::string word, prev, line, tmpAddress, bits, bitA, bitB;

          //For all files, this retrieves the start location of .text
      uint64_t symbAddr = getStrippedSection(".text");
          std::ostringstream sin;
          sin << std::hex << symbAddr;
          std::string dis(sin.str());
      dis.insert(0,"0x");
      sin.str("");

      // duplicate using MC
      formatted_raw_ostream Out(outs(), false);
      MachineFunction *MF = DAS->disassemble(symbAddr);
      object::SectionRef Section = DAS->getSectionByAddress(symbAddr);
      DAS->setSection(Section);


      MachineFunction::iterator BI = MF->begin(), BE = MF->end();

      while (BI != BE
        && DAS->getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < symbAddr) {
        ++BI;
      }
      if (BI == BE) {
        outs() << "Could not disassemble, reached end of function's basic blocks"
          " when looking for first instruction.";
        return 0;
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
      unsigned InstrCount = 0;
      //set arch

      if(TripleName.find("arm") != std::string::npos){
       //outs() << "FOUND ARM \n";
        //loop to find compiler. then call specific solution
        while (BI != BE && (Size == 0 || InstrCount < Size)) {
          //looping through this basic block
          outs() << "Loop 1 to find ldr " << II->getOpcode() << " : "
              << DAS->getDebugOffset(II->getDebugLoc()) << "\n";
          if(II->getOpcode() == 187 || II->getOpcode() == 147){
            ++II;
            ++II;
            outs() << "check for st " << II->getOpcode() << "\n";
            if(II->getOpcode() == 441 || II->getOpcode() == 407) {
              while (BI != BE && (Size == 0 || InstrCount < Size)) {
                outs() << "to find addr " << II->getOpcode() << "\n";
                if(II->getOpcode() == 192 && pre == 192){
                //some magic here, prev = main
                //outs() << "FOUND main call\n";
                // FIX THIS:  add 0x14 (jump 4 instructions) then access the data
                ++II;
                ++II;
                ++II;
                ++II;

                Address = getHexAddress(II);
                FractureSymbol tempSym(Address, "main", 0, object::SymbolRef::Type::ST_Function, 0);
                addSymbol(tempSym);
                return Address;
                }

              pre = II->getOpcode();
              ++II;
              ++InstrCount;
              }
            }
            else if(II->getOpcode() == 187 ){
                outs() << "CLANG\n";
            }
          }
          ++InstrCount;
          ++II;
        if (II == IE) {
          ++BI;
          II = BI->instr_begin();
          IE = BI->instr_end();
        }
      }
    }

      outs() << "No Success finding main. RETURNING\n";
       return 0;

}

void StrippedDisassembler::addSymbol(FractureSymbol tempSym){
  Symbols.push_back(tempSym);
}
std::vector<FractureSymbol> StrippedDisassembler::getSymbolVector(){
  return Symbols;
}

StrippedGraph *StrippedDisassembler::getStrippedGraph() {
  return Graph;
}

} // end namespace fracture
