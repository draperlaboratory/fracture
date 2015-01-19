//===--- MCDirector - Interface to LLVM MC API ------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class interfaces with the MCDisassembler and other LLVM API in order to
// hold state relevant to machine functions and instructions that can be tracked
// by the dish tool.
//
// TODO: Add symbolizer support.
//
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: September 4, 2013
//===----------------------------------------------------------------------===//

#ifndef MCDIRECTOR_H
#define MCDIRECTOR_H

#include "llvm/ADT/IndexedMap.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace fracture {

class MCDirector {
public:
  /// \brief All that is typically needed to set up the MC API is the target
  /// triple. We also ask for output streams to set warnings and errors.
  /// The main purpose of all these fields is so that the user of the Director
  /// can tweak any fields if necessary.
  ///
  /// \param TripleName - the target triple <arch>-<vendor>-<os>[-<environment>]
  ///                     (e.g., "arm-unknown-linux-gnuabi")
  /// \param CPUName - the name of the CPU (e.g., "armv6")
  /// \param Features - the features of the CPU
  /// \param TargetOps - the target options
  /// \param RM - Relocation model
  /// \param CM - Code model
  /// \param OL - Optimization level (has no effect that we know of)
  /// \param InfoOut - prints out information and warnings, defaults to null.
  /// \param ErrOut - prints out errors, defaults to null.
  MCDirector(std::string TripleName,
               StringRef CPUName = "generic",
               StringRef Features = "",
               TargetOptions TargetOpts = TargetOptions(),
               Reloc::Model RM = Reloc::Default,
               CodeModel::Model CM = CodeModel::Default,
               CodeGenOpt::Level OL = CodeGenOpt::Default,
               raw_ostream &InfoOut = nulls(),
               raw_ostream &ErrOut = nulls());

  ~MCDirector();

  /// \brief Check if this object is usable.
  bool isValid();

  /// Accessors
  LLVMContext* getContext() const { return LLVMCtx; }
  const Target* getTarget() const { return TheTarget; }
  TargetMachine* getTargetMachine() const { return TM; }
  const TargetOptions* getTargetOptions() const { return TOpts; }
  const MCContext* getMCContext() const { return MCCtx; }
  const MCObjectFileInfo* getMCObjectFileInfo() const { return MCOFI; }
  const MCAsmInfo* getMCAsmInfo() const { return AsmInfo; }
  const MCSubtargetInfo* getMCSubtargetInfo() const { return STI; }
  const MCRegisterInfo* getMCRegisterInfo() const { return MRI; }
  const MCInstrInfo* getMCInstrInfo() const { return MII; }
  const MCDisassembler* getMCDisassembler() const { return DisAsm; }
  MCInstPrinter* getMCInstPrinter() const { return MIP; }

  EVT getRegType(unsigned RegisterID);

private:
  LLVMContext *LLVMCtx;
  /// NOTE: In order of initialization in Constructor.
  const TargetOptions *TOpts;
  const Target *TheTarget;
  TargetMachine *TM;

  const MCSubtargetInfo *STI;
  const MCDisassembler *DisAsm;
  const MCRegisterInfo *MRI;
  MCObjectFileInfo *MCOFI;
  MCContext *MCCtx;
  const MCAsmInfo *AsmInfo;
  const MCInstrInfo *MII;
  MCInstPrinter *MIP;
  IndexedMap<EVT> RegTypes;

  /// Error printing.
  raw_ostream &Infos, &Errs;
  void printInfo(std::string Msg) const {
    Infos << "MCDirector: " << Msg << "\n";
  }
  void printError(std::string Msg) const {
    Errs << "MCDirector: " << Msg << "\n";
    Errs.flush();
  }
};


} // end namespace fracture

#endif /* MCDIRECTOR_H */
