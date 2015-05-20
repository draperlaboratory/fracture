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
    TheModule = new Module(Executable->getFileName(), *MC->getContext());
  } else {
    TheModule = NewModule;
  }
  // Set current section to ".text"
  // setSection(".text");
  setSection("text");
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
    // Decode basic blocks until end of function
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
  ArrayRef<uint8_t> Bytes(
      (uint8_t*)CurSectionMemory->getBytes().data(),
      (size_t)CurSectionMemory->getBytes().size());
  // Chop any bytes off before instuction address that we don't need.
  uint64_t NewAddr = Address - CurSectionMemory->getBase();
  ArrayRef<uint8_t> NewBytes((uint8_t*)(Bytes.data() + NewAddr), 
                             //Bytes.data() + Bytes.size() - NewAddr);
                             (size_t)(Bytes.size() - NewAddr));
  // Replace nulls() with outs() for stack tracing
  if (!(DA->getInstruction(*Inst, InstSize, NewBytes, Address,
        nulls(), nulls()))) {
    printError("Unknown instruction encountered, instruction decode failed! ");
    
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

  // Check if the instruction can load to program counter and mark it as a Ret
  // FIXME: Better analysis would be to see if the PC value references memory
  // sent as a parameter or set locally in the function, but that would need to
  // happen after decompilation. In either case, this is definitely a BB
  // terminator or branch!
  if (MCID->mayLoad()
    && MCID->mayAffectControlFlow(*Inst, *MC->getMCRegisterInfo())) {
    MCID->Flags |= (1 << MCID::Return);
    MCID->Flags |= (1 << MCID::Terminator);
  }


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
      MachinePointerInfo(), flags, 4, 0);	//MCO.getImm()
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
  Twine DIType = "0x" + Twine::utohexstr(AddrMask);
  std::vector<Metadata*> *Elts = new std::vector<Metadata*>();
  Elts->push_back(MDString::get(*MC->getContext(), StringRef(DIType.str())));
  Elts->push_back(ValueAsMetadata::get(ConstantInt::get(Int64, Address)));
  DIScope *Scope = new DIScope(MDNode::get(*MC->getContext(), *Elts));
  // The following is here to fill in the value and not to be used to get
  // offsets
  unsigned ColVal = (Address & 0xFF000000) >> 24;
  unsigned LineVal = Address & 0xFFFFFF;
  DebugLoc *Location = new DebugLoc(DebugLoc::get(LineVal, ColVal,
      Scope->get(), NULL));

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
    MF = new MachineFunction(F, *MC->getTargetMachine(), Address, *MMI);
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
  int NumRead = CurSectionMemory->readBytes(Bytes, Address, Size);
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

  // Calculate function address for printing function names in disassembly
  int64_t Tgt = 0, DestInt = 0;
  StringRef FuncName;
  if (Inst->isCall()) {
    Size != 5 ? Size = 8 : Size; // Instruction size is 8 for ARM
    for (MachineInstr::mop_iterator MII = Inst->operands_begin(); MII !=
         Inst->operands_end(); ++MII)
    if (MII->isImm())
      DestInt = MII->getImm();
    Tgt = Address + Size + DestInt;
    FuncName = getFunctionName(Tgt);
    if (FuncName.startswith("func")) {
      StringRef SectionName;
      object::SectionRef Section = getSectionByAddress(Tgt);
      setSection(Section);
      getRelocFunctionName(Tgt, FuncName);
      Section = getSectionByAddress(Address);
      setSection(Section);
    }
  }

  // Print instruction
  // NOTE: We could print the "Full" machine instruction version here instead
  // of down-converting to MCInst...
  if (PrintTypes) {
    Inst->print(Out, MC->getTargetMachine(), false);
  } else {
    MC->getMCInstPrinter()->printInst(Instructions[Address], Out,
    Inst->isCall() ? FuncName : "");
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
  std::error_code ec;
  for (object::symbol_iterator I = Executable->symbols().begin(),
         E = Executable->symbols().end(); I != E; ++I) {
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
// getRelocFunctionName() pairs function call addresses with dynamically relocated
// library function addresses and sets the function name to the actual name 
// rather than the function address
void Disassembler::getRelocFunctionName(unsigned Address, StringRef &NameRef) {
  MachineFunction *MF = disassemble(Address);
  MachineBasicBlock *MBB = &(MF->front());
  uint64_t JumpAddr = 0;
  StringRef RelName;
  std::error_code ec;
  bool isOffsetJump = false;

  // Iterate through the operands, checking for immediates and grabbing them
  MachineInstr *JumpInst = &*MBB->instr_rbegin();
  for (MachineInstr::mop_iterator MII = JumpInst->operands_begin();
       MII != JumpInst->operands_end(); ++MII) {
    if (MII->isImm()) {
      if ( MBB->size() > 1) {
        JumpAddr = MII->getImm();
        break;
      }
      JumpAddr = MII->getImm();
    }
  }
  // If the Jump address of the instruction is smaller than the instruction address
  // then it must be an offset from the instruction address. In this case, we
  // add the jump address to the original address plus the instruction size.
  if (JumpAddr < Address) isOffsetJump = true;
  if (MBB->size() > 1) JumpAddr += (Address + 32768 + 8);
  else if (isOffsetJump) JumpAddr += Address + JumpInst->getDesc().getSize();

  // Check if address matches relocation symbol address and if so
  // grab the symbol name
  for (object::section_iterator seci = Executable->section_begin(); seci !=
      Executable->section_end(); ++seci)
    for (object::relocation_iterator ri = seci->relocation_begin(); ri != 
         seci->relocation_end(); ++ri) {
      uint64_t RelocAddr;
      if ((ec = ri->getAddress(RelocAddr))) {
        errs() << ec.message() << "\n";
        continue;
      }
      if (JumpAddr == RelocAddr) {
        if ((ec = ri->getSymbol()->getName(RelName))) {
          errs() << ec.message() << "\n";
          continue;
        }
        RelocOrigins[RelName] = Address;
      }
    }
  // NameRef is passed by reference, so if relocation doesn't match,
  // we don't want to modify the StringRef
  if (!RelName.empty())
    NameRef = RelName;
}

const StringRef Disassembler::getFunctionName(unsigned Address) const {
  uint64_t SymAddr;
  std::error_code ec;
  StringRef NameRef;
  // Check in the regular symbol table first
  for (object::symbol_iterator I = Executable->symbols().begin(),
         E = Executable->symbols().end(); I != E; ++I) {
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
  // for (object::symbol_iterator I = elf->dynamic_symbol_begin(),
  //        E = elf->dynamic_symbol_end(); I != E; ++I) {
  //   object::SymbolRef::Type SymbolTy;
  //   if ((ec = I->getType(SymbolTy))) {
  //     errs() << ec.message() << "\n";
  //     continue;
  //   }
  //   if (SymbolTy != object::SymbolRef::ST_Function) {
  //     continue;
  //   }
  //   if ((ec = I->getAddress(SymAddr))) {
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
  std::error_code ec = Section.getContents(Bytes);
  if (ec) {
    printError(ec.message());
    return;
  }
  SectAddr = Section.getAddress();
  SectSize = Section.getSize();
  CurSection = Section;
  CurSectionEnd = SectAddr + SectSize;
  CurSectionMemory = new FractureMemoryObject(Bytes, SectAddr);
  StringRef SectionName;
  CurSection.getName(SectionName);
  //printInfo("Setting Section " + std::string(SectionName.data()));
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
  std::error_code ec;
  StringRef Name;
  uint64_t Addr;
  for (object::section_iterator si = Executable->section_begin(), se =
         Executable->section_end(); si != se; ++si) {
    if (ec) {
      printError(ec.message());
      break;
    }
    if (si->getName(Name)) {
      Addr = si->getAddress();
      Infos << "Disassembler: Unnamed section encountered at "
            << format("%8" PRIx64 , Addr) << "\n";
      continue;
    }
    if(Name == SectionName) {
      return *si;
    }
  }
  for (object::section_iterator si = Executable->section_begin(), se =
         Executable->section_end(); si != se; ++si) {
    if (si->getName(Name)) {
      Addr = si->getAddress();
      continue;
    }
    if (Name.str().find(SectionName) != std::string::npos)
      return *si;
  }

  printError("Unable to find section named \"" + std::string(SectionName.data())
    + "\"");
  return *Executable->section_end();
}

const object::SectionRef Disassembler::getSectionByAddress(unsigned Address)
  const {
  std::error_code ec;
  for (object::section_iterator si = Executable->section_begin(), se =
         Executable->section_end(); si != se; ++si) {
    
    if (ec) {
      printError(ec.message());
      break;
    }

    uint64_t SectionAddr;
    SectionAddr = si->getAddress();

    uint64_t SectionSize;
    SectionSize = si->getSize();

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

  if (ConstantInt *OffsetVal = dyn_cast<ConstantInt>(
      dyn_cast<ValueAsMetadata>(Scope->getOperand(1))->getValue())) {
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
