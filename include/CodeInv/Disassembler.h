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

#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/StringRefMemoryObject.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <sstream>

#include "CodeInv/MCDirector.h"

using namespace llvm;

namespace fracture {

class Disassembler {
public:
  /// \brief Construct a new Disassembler object, which is tied to a specific
  /// executable. The constructor creates a module if none is found, and may
  /// share the MCDirector with other Disassemblers (attached to other
  /// executables).
  ///
  /// \param TheMC - The MCDirector object used to access the relevant LLVM API.
  ///                The Director is required to be a valid object to
  ///                disassemble executable bytes.
  /// \param TheExe - The Executable object to disassemble.
  /// \param TheModule - Optional parameter to specify an LLVM Module to use. If
  ///                    it's not provided, the disassembler will create one and
  ///                    you can use the accessor to get it.
  Disassembler(MCDirector *NewMC, object::ObjectFile *NewExecutable,
    Module *NewModule = NULL, raw_ostream &InfoOut = nulls(),
    raw_ostream &ErrOut = nulls());
  ~Disassembler();

  /// \brief Decodes the instructions at the specified address and returns them
  /// in a vector.
  ///
  /// When possible, this function attempts to pull decoded instructions from
  /// previously recorded basic blocks. It also creates basic blocks from the
  /// instructions (and a function, if necessary) as they are decoded.
  ///
  /// Note: to get the address of an instruction, use the DebugLoc associated
  ///       with the instruction. It is encoded into the line number.
  ///
  /// \param Address - the address of the offset for the current section.
  /// \param Size - the number of instructions to decode. If 0, decodes until
  ///               the end of the function. Always stops at end of function.
  ///
  /// \returns A vector of MachineInstr* with valid MC Instruction Descriptors
  MachineFunction* disassemble(unsigned Address);

  /// \brief Prints size instructions on the given output stream at the given
  /// address. Uses current section for base offset.
  ///
  /// \param Out - output stream
  /// \param Address - starting address
  /// \param Size - number of functions (or 0 to print until end of function)
  unsigned printInstructions(formatted_raw_ostream &Out, unsigned Address,
    unsigned Size, bool PrintTypes = true);
  void printInstruction(formatted_raw_ostream &Out, MachineInstr *Inst,
    bool PrintTypes = false);
  void printInstruction(raw_ostream &Out, MachineInstr *Inst,
    bool PrintTypes = false) {
    formatted_raw_ostream OutF(Out);
    printInstruction(OutF, Inst, PrintTypes);
  }


  MachineBasicBlock* decodeBasicBlock(unsigned Address, MachineFunction* MF,
    unsigned &Size);

  unsigned decodeInstruction(unsigned Address, MachineBasicBlock *Block);
  /// \brief Create a function object at the specified address.
  MachineFunction* getOrCreateFunction(unsigned Address);

  MachineFunction* getNearestFunction(unsigned Address);

  /// \brief Disassembles a specific instruction given the specific object file
  /// offset.
  ///
  /// Note that this function is just an accessor to the underlying LLVM API,
  /// and doesn't affect state of the object.
  ///
  /// \param Address - an unsigned integer representing the offset for the
  /// current section of the object file.
  ///
  /// \returns MCInst object pointer.
  // MCInst* getInst(unsigned Address) const { return Instructions[Address]; }

  /// Getters and Setters
  void setExecutable(object::ObjectFile* NewExecutable);
  object::ObjectFile* getExecutable() { return Executable; };

  static std::string rawBytesToString(StringRef Bytes);


  /// \brief Symbol accessors
  std::string getSymbolName(unsigned Address);
  const StringRef getFunctionName(unsigned Address) const;

  /// \brief Set the current section reference in the Disassembler
  ///
  /// \param SectionName a string representing the name, e.g. ".text"
  /// \param Section a pointer to the desired SectionRef
  void setSection(std::string SectionName);
  void setSection(const object::SectionRef Section);
  const object::SectionRef getCurrentSection() const {
    return CurSection;
  }
  const object::SectionRef getSectionByName(StringRef SectionName) const;
  const object::SectionRef getSectionByAddress(unsigned Address) const;

  object::ObjectFile* getExecutable() const { return Executable; }
  MCDirector* getMCDirector() const { return MC; }
  Module* getModule() const { return TheModule; }

  const MachineInstr* getMachineInstr(unsigned Address) const {
     if (MachineInstructions.find(Address) != MachineInstructions.end()) {
        return MachineInstructions.at(Address);
     }
     return NULL;
  }

  MCInst* getMCInst(unsigned Address) const {
    if (Instructions.find(Address) != Instructions.end()) {
      return Instructions.at(Address);
    }
    return NULL;
  }

  uint64_t getDebugOffset(const DebugLoc &Loc) const;
  DebugLoc* setDebugLoc(uint64_t Address);
  void deleteFunction(MachineFunction* MF);
private:
  object::SectionRef CurSection;
  object::ObjectFile *Executable;
  StringRefMemoryObject* CurSectionMemory;
  uint64_t CurSectionEnd;
  std::map<unsigned, MachineBasicBlock*> BasicBlocks;
  std::map<unsigned, MachineFunction*> Functions;
  std::map<unsigned, MCInst*> Instructions;
  std::map<unsigned, const MachineInstr*> MachineInstructions;

  MachineModuleInfo *MMI;
  GCModuleInfo *GMI;
  Module *TheModule;

  MCDirector *MC;

  /// Error printing
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const { 
    Infos << "Disassembler: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "Disassembler: " << Msg << "\n";
    Errs.flush();
  }
};

} // end namespace fracture

#endif /* DISASSEMBLER_H */
