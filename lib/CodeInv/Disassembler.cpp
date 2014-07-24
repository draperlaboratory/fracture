//===--- Disassembler - Interface to MCDisassembler -------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class interfaces with the MCDisassembler, and holds state relevant to
// machine functions and instructions that can be tracked by the dish tool.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: August 28, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/Disassembler.h"

using namespace llvm;

namespace fracture {

Disassembler::Disassembler(MCDirector *NewMC, object::ObjectFile *NewExecutable,
  Module *NewModule, raw_ostream &InfoOut, raw_ostream &ErrOut)
  : Infos(InfoOut), Errs(ErrOut) {
  MC = NewMC;
  setExecutable(NewExecutable);
  // If the module is null then create a new one
  if (NewModule == NULL) {
    // TODO: getloadName may fail, how to resolve?
    TheModule = new Module(Executable->getLoadName(), *MC->getContext());
  } else {
    TheModule = NewModule;
  }
  // Set current section to ".text"
  setSection(".text");
  // Initialize the MMI
  MMI = new MachineModuleInfo(*MC->getMCAsmInfo(), *MC->getMCRegisterInfo(),
    MC->getMCObjectFileInfo());
  // Initialize the GCMI
  GMI = new GCModuleInfo();
}

Disassembler::~Disassembler() {
  // Note: BasicBlocks and Functions are also a part of TheModule, but we
  // still check to make sure they get deleted anyway.
  delete MC;
  delete TheModule;
  delete GMI;
  delete MMI;

  // NOTE: Consider using OwningPtr interface instead of this.
  for (std::map<unsigned, MCInst*>::iterator I = Instructions.begin(),
         E = Instructions.end(); I != E; ++I) {
    if (I->second) delete I->second;
  }

  for (std::map<unsigned, MachineFunction*>::iterator I = Functions.begin(),
         E = Functions.end(); I != E; ++I) {
    if (I->second) {
      delete I->second;
      errs() << "Disassembler: MachineFunction not deleted in module!\n";
    }
  }

  // BasicBlock destructor is private.
  for (std::map<unsigned, MachineBasicBlock*>::iterator I = BasicBlocks.begin(),
         E = BasicBlocks.end(); I != E; ++I) {
    // if (I->second) delete I->second;
    if (I->second) errs() << "Disassembler: BasicBlock not deleted!\n";
  }


  delete CurSectionMemory;
  delete Executable;
}

MachineFunction* Disassembler::disassemble(unsigned Address) {
  MachineFunction *MF = getOrCreateFunction(Address);

  if (MF->size() == 0) {
    // Create basic blocks until end of function
    unsigned Size = 0;
    MachineBasicBlock *MBB;
    do {
      unsigned MBBSize = 0;
      MBB = decodeBasicBlock(Address+Size, MF, MBBSize);
      Size += MBBSize;
    } while (Address+Size < CurSectionEnd && MBB->size() > 0
      && !(MBB->instr_rbegin()->isReturn()));
    if (Address+Size < CurSectionEnd && MBB->size() > 0) {
      // FIXME: This can be shoved into the loop above to improve performance
      MachineFunction *NextMF =
        getNearestFunction(getDebugOffset(MBB->instr_rbegin()->getDebugLoc()));
      if (NextMF != NULL) {
        Functions.erase(
          getDebugOffset(NextMF->begin()->instr_begin()->getDebugLoc()));
      }
    }
  }

  Functions[Address] = MF;
  return MF;
}

MachineBasicBlock* Disassembler::decodeBasicBlock(unsigned Address,
  MachineFunction* MF, unsigned &Size) {
  assert(MF && "Unable to decode basic block without Machine Function!");

  uint64_t MFLoc = MF->getFunctionNumber(); // FIXME: Horrible, horrible hack
  uint64_t Off = Address-MFLoc;
  std::stringstream MBBName;
  MBBName << MF->getName().str() << "+" << Off;

  // Dummy holds the name.
  BasicBlock *Dummy = BasicBlock::Create(*MC->getContext(), MBBName.str());
  MachineBasicBlock *MBB = MF->CreateMachineBasicBlock(Dummy);
  MF->push_back(MBB);

  // NOTE: Might also need SectAddr...
  Size = 0;
  while (Address+Size < (unsigned) CurSectionEnd) {
    unsigned CurAddr = Address+Size;
    Size += std::max(unsigned(1), decodeInstruction(CurAddr, MBB));
    MachineInstr* MI = NULL;
    if (MBB->size() != 0) {
      MI = &(*MBB->instr_rbegin());
      MachineInstructions[CurAddr] = MI;
    }
    if (MI != NULL && MI->isTerminator()) {
      break;
    }
  }

  if (Address >= CurSectionEnd) {
    printInfo("Reached end of current section!");
  }

  return MBB;
}

unsigned Disassembler::decodeInstruction(unsigned Address,
  MachineBasicBlock *Block) {
  // Disassemble instruction
  const MCDisassembler *DA = MC->getMCDisassembler();
  uint64_t InstSize;
  MCInst *Inst = new MCInst();
  StringRef Bytes;

  if (!(DA->getInstruction(*Inst, InstSize, *CurSectionMemory, Address,
        nulls(), nulls()))) {
    printError("Unknown instruction encountered, instruction decode failed!");
    return 1;
    // Instructions[Address] = NULL;
    // Block->push_back(NULL);	
    // TODO: Replace with default size for each target.
    // return 1;
    // outs() << format("%8" PRIx64 ":\t", SectAddr + Index);
    // Dism->rawBytesToString(StringRef(Bytes.data() + Index, Size));
    // outs() << "   unkn\n";
  }
  Instructions[Address] = Inst;

  // Recover Instruction information
  const MCInstrInfo *MII = MC->getMCInstrInfo();
  MCInstrDesc *MCID = new MCInstrDesc(MII->get(Inst->getOpcode()));
  MCID->Size = InstSize;

  // Recover MachineInstr representation
  DebugLoc *Location = setDebugLoc(Address);
  MachineInstrBuilder MIB = BuildMI(Block, *Location, *MCID);
  unsigned int numDefs = MCID->getNumDefs();
  for (unsigned int i = 0; i < Inst->getNumOperands(); i++) {
    MCOperand MCO = Inst->getOperand(i);
    // FIXME: This hack is a workaround for the assert in MachineInstr.cpp:653,
    // where OpNo >= MCID->getNumOperands()...
    if (i >= MCID->getNumOperands() && !(MCID->isVariadic()))
      break;

    if (MCO.isReg()) {
      unsigned flags = 0;
      // Defs always start at the beginning of the operands list,
      // unfortunately BuildMI doesn't set default define flags so we have
      // to do it manually here.
      // NOTE: This should always be true, but might not be if operands list
      //       is not populated correctly by the MC Backend for the target.
      if (i < numDefs) {
        flags |= RegState::Define;
      }

      // NOTE: No need to worry about imp defs and uses, as these are already
      //       specificed in the MCID attached to the MachineInst object.
      MIB.addReg(MCO.getReg(), flags);
      continue;
    }
    if (MCO.isImm()) {
      MIB.addImm(MCO.getImm());
        continue;
    }
    //else if (MCO.isFPImm()) MIB.addFPImm(MCO.getFPImm());
    if (MCO.isExpr()) {
      MCOperandInfo MCOpInfo = MCID->OpInfo[i];
      switch (MCOpInfo.OperandType) {
        case MCOI::OPERAND_MEMORY:
        case MCOI::OPERAND_PCREL:
        case MCOI::OPERAND_UNKNOWN:
        default:
          printError("Unknown how to handle this Expression at this time.");
      }
    }
    printError("Unknown how to handle Operand!");
  }

  // NOTE: I tried MCOpInfo here, and it appearst o be NULL
  // ... at least for ARM.
  unsigned flags = 0;
  if (MCID->mayLoad())
  	flags |= MachineMemOperand::MOLoad;
  if (MCID->mayStore())
  	flags |= MachineMemOperand::MOStore;
  if (flags != 0) {
  	// Constant* cInt = ConstantInt::get(Type::getInt64Ty(ctx), MCO.getImm());
  	// Value *Val = ConstantExpr::getIntToPtr(cInt,
  	// PointerType::getUnqual(Type::getInt32Ty(ctx)));
  	// FIXME: note size of 4 is known to be bad for
  	// some targets

  	//Copy & paste set getImm to zero
  	MachineMemOperand* MMO = new MachineMemOperand(
  			MachinePointerInfo(0, 0), flags, 4, 0);	//MCO.getImm()
		 	 MIB.addMemOperand(MMO);
		 	 //outs() << "Name: " << MII->getName(Inst->getOpcode()) << " Flags: " << flags << "\n";
	 }

  // Note: I don't know why they decided instruction size needed to be 64 bits,
  // but the following conversion shouldn't be an issue.
  return ((unsigned)InstSize);
}

DebugLoc* Disassembler::setDebugLoc(uint64_t Address) {
  // Note: Location stores offset of instruction, which is really a perverse
  //       misuse of this field.
  Type *Int64 = Type::getInt64Ty(*MC->getContext());
  // The following sets the "scope" variable which actually holds the address.
  uint64_t AddrMask = dwarf::DW_TAG_lexical_block;
  std::vector<Value*> *Elts = new std::vector<Value*>();
  Elts->push_back(ConstantInt::get(Int64, AddrMask));
  Elts->push_back(ConstantInt::get(Int64, Address));
  MDNode *Scope = MDNode::get(*MC->getContext(), *Elts);
  // The following is here to fill in the value and not to be used to get
  // offsets
  unsigned ColVal = (Address & 0xFF000000) >> 24;
  unsigned LineVal = Address & 0xFFFFFF;
  DebugLoc *Location = new DebugLoc(DebugLoc::get(LineVal, ColVal,
      Scope, NULL));

  return Location;
}

MachineFunction* Disassembler::getOrCreateFunction(unsigned Address) {
  MachineFunction *MF = getNearestFunction(Address);
  if (MF == NULL) {
    StringRef FNameRef = getFunctionName(Address);
    // Note: this fixes breakage in the constructor below DO NOT REMOVE
    std::string FN = FNameRef.str();
    FunctionType *FTy = FunctionType::get(
      Type::getPrimitiveType(TheModule->getContext(), Type::VoidTyID), false);
    Function *F = cast<Function>(TheModule->getOrInsertFunction(FN, FTy));
    MF = new MachineFunction(F, *MC->getTargetMachine(), Address, *MMI, GMI);
    Functions[Address] = MF;
  }
  return MF;
}

MachineFunction* Disassembler::getNearestFunction(unsigned Address) {
  if (Functions.size() == 0) {
    return NULL;
  }
  std::map<unsigned, MachineFunction*>::reverse_iterator FuncItr =
    Functions.rbegin();
  while (FuncItr != Functions.rend()) {
    if (FuncItr->second == NULL || FuncItr->second->size() == 0
	|| FuncItr->second->rbegin()->size() == 0) {
      FuncItr++;
      continue;
    }
    if (FuncItr->first <= Address && Address < CurSectionEnd) {
      // Does this address fit there?
      MachineInstr* LastInstr =
        &(*((FuncItr->second->rbegin())->instr_rbegin()));
      if (Address <= getDebugOffset(LastInstr->getDebugLoc())) {
        return FuncItr->second;
      }
    }
    FuncItr++;
  }
  return NULL;
}

unsigned Disassembler::printInstructions(formatted_raw_ostream &Out,
  unsigned Address, unsigned Size, bool PrintTypes) {
  MachineFunction *MF = disassemble(Address);

  MachineFunction::iterator BI = MF->begin(), BE = MF->end();
  // Skip to first basic block with instruction in desired address
  // Out << BI->instr_rbegin()->getDebugLoc().getLine() << "\n";
  while (BI != BE
    && getDebugOffset(BI->instr_rbegin()->getDebugLoc()) < Address) {
    ++BI;
  }
  if (BI == BE) {
    printError("Could not disassemble, reached end of function's basic blocks"
      " when looking for first instruction.");
    return 0;
  }


  MachineBasicBlock::iterator II = BI->instr_begin(), IE = BI->instr_end();
  // skip to first instruction
  while (getDebugOffset(II->getDebugLoc()) < Address) {
    if (II == IE) {
      printError("Unreachable: reached end of basic block whe looking for first"
        " instruction.");
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
    ++II;
  }
  if (Address != getDebugOffset(II->getDebugLoc())) {
    Out << "Warning: starting at " << getDebugOffset(II->getDebugLoc())
        << " instead of " << Address << ".\n";
  }

  // Function Name and Offset
  Out << "<" << MF->getName();
  if (getDebugOffset(MF->begin()->instr_begin()->getDebugLoc()) != Address) {
    Out << "+"
        << (Address
          - getDebugOffset(MF->begin()->instr_begin()->getDebugLoc()));
  }
  Out << ">:\n";

  // Print each instruction
  unsigned InstrCount = 0;
  while (BI != BE && (Size == 0 || InstrCount < Size)) {
    printInstruction(Out, II, PrintTypes);
    ++InstrCount;
    ++II;
    if (II == IE) {
      ++BI;
      II = BI->instr_begin();
      IE = BI->instr_end();
    }
  }

  return InstrCount;
}

void Disassembler::printInstruction(formatted_raw_ostream &Out,
  MachineInstr *Inst, bool PrintTypes) {
  unsigned Address = getDebugOffset(Inst->getDebugLoc());
  unsigned Size = Inst->getDesc().getSize();
  // TODO: replace the Bytes with something memory safe (StringRef??)
  uint8_t *Bytes = new uint8_t(Size);
  int NumRead = CurSectionMemory->readBytes(Address, Size, Bytes);
  if (NumRead < 0) {
    printError("Unable to read current section memory!");
    return;
  }
  // Print Address
  Out << format("%08" PRIX64 ":", Address);
  Out.PadToColumn(12);        // 12345678: <- 9 chars + 1 space

  // Print Instruction Bytes
  for (unsigned i = 0, e = ((Size > 8) ? 8 : Size); i != e; ++i)
    Out << format("%02" PRIX8 " ", Bytes[i]);
  Out.PadToColumn(40);        // 8 bytes (2 char) + 1 space each + 2 spaces

  // Print instruction
  // NOTE: We could print the "Full" machine instruction version here instead
  // of down-converting to MCInst...
  if (PrintTypes) {
    Inst->print(Out, MC->getTargetMachine(), false);
  } else {
    MC->getMCInstPrinter()->printInst(Instructions[Address], Out, "");
    Out << "\n";
  }

  // Print the rest of the instruction bytes
  unsigned ColCnt = 8;
  for (unsigned i = 8, e = Size; i < e; ++i) {
    if (ColCnt == 8) {
      Out.PadToColumn(12);        // 8 bytes (2 char) + 1 space each + 2 spaces
      Out << "\n";
      ColCnt = 0;
    } else {
      ++ColCnt;
    }
    Out << format("%02" PRIX8 " ", Bytes[i]);
  }
  delete Bytes;
}


void Disassembler::setExecutable(object::ObjectFile* NewExecutable) {
  // NOTE: We don't do any reorging with the module or the machine functions. We
  // need to evaluate if this is necessary. We should *not* change the MC API
  // settings to match those of the executable.
  Executable = NewExecutable;
}

std::string Disassembler::getSymbolName(unsigned Address) {
  uint64_t SymAddr;
  error_code ec;
  for (object::symbol_iterator I = Executable->begin_symbols(),
         E = Executable->end_symbols(); I != E; ++I) {
    if ((ec = I->getAddress(SymAddr))) {
      errs() << ec.message() << "\n";
      continue;
    }
    if ((unsigned)SymAddr == Address) {
      StringRef Name;
      if ((ec = I->getName(Name))) {
        errs() << ec.message() << "\n";
        continue;
      }
      return Name.str();
    }
  }
  return "";
}

const StringRef Disassembler::getFunctionName(unsigned Address) const {
  uint64_t SymAddr;
  error_code ec;
  StringRef NameRef;
  // Check in the regular symbol table first
  for (object::symbol_iterator I = Executable->begin_symbols(),
         E = Executable->end_symbols(); I != E; ++I) {
    object::SymbolRef::Type SymbolTy;
    if ((ec = I->getType(SymbolTy))) {
      errs() << ec.message() << "\n";
      continue;
    }
    if (SymbolTy != object::SymbolRef::ST_Function) {
      continue;
    }
    if ((ec = I->getAddress(SymAddr))) {
      errs() << ec.message() << "\n";
      continue;
    }
    if ((unsigned)SymAddr == Address) {
      if ((ec = I->getName(NameRef))) {
        errs() << ec.message() << "\n";
        continue;
      }
      break;
    }
  }
  // NOTE: Dynamic symbols accessors removed in newer version of llvm-trunk
  // Now check dynamic symbols
  // for (object::symbol_iterator I = Executable->begin_dynamic_symbols(),
  //        E = Executable->end_dynamic_symbols(); I != E; I.increment(ec)) {
  //   object::SymbolRef::Type SymbolTy;
  //   if ((ec = I->getType(SymbolTy))) {
  //     errs() << ec.message() << "\n";
  //     continue;
  //   }
  //   if (SymbolTy != object::SymbolRef::ST_Function) {
  //     continue;
  //   }
  //   if ((ec = I->getValue(SymAddr))) {
  //     errs() << ec.message() << "\n";
  //     continue;
  //   }
  //   if ((unsigned)SymAddr == Address) {
  //     if ((ec = I->getName(NameRef))) {
  //       errs() << ec.message() << "\n";
  //       continue;
  //     }
  //     break;
  //   }
  // }

  if (NameRef.empty()) {
    std::string *FName = new std::string();
    raw_string_ostream FOut(*FName);
    FOut << "func_" << format("%1" PRIx64, Address);
    return StringRef(FOut.str());
  }
  return NameRef;
}


void Disassembler::setSection(std::string SectionName) {
  setSection(getSectionByName(SectionName));
}

void Disassembler::setSection(const object::SectionRef Section) {
  StringRef Bytes;
  uint64_t SectAddr, SectSize;
  error_code ec = Section.getContents(Bytes);
  if (ec) {
    printError(ec.message());
    return;
  }
  ec = Section.getAddress(SectAddr);
  if (ec) {
    printError(ec.message());
    return;
  }
  ec = Section.getSize(SectSize);
  if (ec) {
    printError(ec.message());
    return;
  }

  CurSection = Section;
  CurSectionEnd = SectAddr + SectSize;
  CurSectionMemory = new StringRefMemoryObject(Bytes, SectAddr);
  StringRef SectionName;
  CurSection.getName(SectionName);
  printInfo("Setting Section " + std::string(SectionName.data()));
  // TODO: Add section relocations (if ncessary).
  // Make a list of all the relocations for this section.
  // error_code ec;
  // std::vector<object::RelocationRef> Rels;
  // for (relocation_iterator ri = Section.begin_relocations(), re =
  //     Section.end_relocations(); ri != re; ri.increment(ec)) {
  //   if (error(ec))
  //     break;
  //   Rels.push_back(*ri);
  // }

  // Sort relocations by address.
  // std::sort(Rels.begin(), Rels.end(), relocAddressLess);

  // std::vector<RelocationRef>::const_iterator rel_cur = Rels.begin();
  // std::vector<RelocationRef>::const_iterator rel_end = Rels.end();
}

std::string Disassembler::rawBytesToString(StringRef Bytes) {
  static const char hex_rep[] = "0123456789abcdef";

  std::string Str;

  for (StringRef::iterator i = Bytes.begin(), e = Bytes.end(); i != e; ++i) {
    Str += hex_rep[(*i & 0xF0) >> 4];
    Str += hex_rep[*i & 0xF];
    Str += ' ';
  }

  return Str;
}

const object::SectionRef Disassembler::getSectionByName(StringRef SectionName)
  const {
  error_code ec;
  for (object::section_iterator si = Executable->section_begin(), se =
         Executable->section_end(); si != se; ++si) {



    if (ec) {
      printError(ec.message());
      break;
    }

    StringRef Name;
    if (si->getName(Name)) {
      uint64_t Addr;
      si->getAddress(Addr);
      Infos << "Disassembler: Unnamed section encountered at "
            << format("%8" PRIx64 , Addr) << "\n";
      continue;
    }

    if (Name == SectionName) {
      return *si;
    }
  }

  printError("Unable to find section named \"" + std::string(SectionName.data())
    + "\"");
  return *Executable->section_end();
}

const object::SectionRef Disassembler::getSectionByAddress(unsigned Address)
  const {
  error_code ec;
  for (object::section_iterator si = Executable->section_begin(), se =
         Executable->section_end(); si != se; ++si) {

    if (ec) {
      printError(ec.message());
      break;
    }

    uint64_t SectionAddr;
    if (si->getAddress(SectionAddr))
      break;

    uint64_t SectionSize;
    if (si->getSize(SectionSize))
      break;

    if (SectionAddr <= Address && Address < SectionAddr + SectionSize) {
      return *si;
    }
  }

  return *Executable->section_end();
}

uint64_t Disassembler::getDebugOffset(const DebugLoc &Loc) const {
  MDNode *Scope = Loc.getScope(*MC->getContext());
  if (Scope == NULL || Scope->getNumOperands() != 2) {
    errs() << "Error: Scope not set properly on Debug Offset.\n";
    return 0;
  }

  if (ConstantInt *OffsetVal = dyn_cast<ConstantInt>(Scope->getOperand(1))) {
    return OffsetVal->getZExtValue();
  }

  errs() << "Could not decode DebugOffset Value as a ConstantInt!\n";
  return 0;
}

void Disassembler::deleteFunction(MachineFunction *MF) {
  std::map<unsigned, MachineFunction*>::reverse_iterator FuncItr =
    Functions.rbegin();
  while (FuncItr != Functions.rend()) {
    if (FuncItr->second == MF) {
      break;
    }
    FuncItr++;
  }
  if (FuncItr != Functions.rend()) {
    Functions.erase(FuncItr->first);
    delete MF;
  }
}

} // end namespace fracture
