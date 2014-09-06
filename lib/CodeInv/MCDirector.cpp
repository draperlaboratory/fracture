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
// Author: Richard Carback (rtc1032) <rcarback@draper.com>
// Date: September 4, 2013
//===----------------------------------------------------------------------===//

#include "CodeInv/MCDirector.h"

using namespace llvm;

namespace fracture {

MCDirector::MCDirector(std::string TripleName,
  StringRef CPUName,
  StringRef Features,
  TargetOptions TargetOpts,
  Reloc::Model RM,
  CodeModel::Model CM,
  CodeGenOpt::Level OL,
  raw_ostream &InfoOut,
  raw_ostream &ErrOut) : Infos(InfoOut), Errs(ErrOut) {

  printInfo("Using Triple: " + TripleName);
  printInfo("Using CPU: " + CPUName.str());
  printInfo("Using Features: " + Features.str());

  LLVMCtx = &getGlobalContext();

  // TargetOptions
  TOpts = new TargetOptions(TargetOpts);

  // Target
  std::string ErrMsg;
  TheTarget = TargetRegistry::lookupTarget(TripleName, ErrMsg);
  if (TheTarget == NULL && ErrMsg.empty()) {
    printError("Unnknown error. Could not init Target.");
  } else if (!ErrMsg.empty()) {
    printError(ErrMsg);
  }

  // TargetMachine
  TM = TheTarget->createTargetMachine(TripleName, CPUName, Features,
                                      TargetOpts);
  if (TM == NULL) {
    printError("Unable to create TargetMachine. "
               "Did you call InitializeAllTargets()?");
  }

  // SubtargetInfo
  STI = TheTarget->createMCSubtargetInfo(TripleName, CPUName, Features);
  if (STI == NULL) {
    printError("Unable to create SubtargetInfo.");
  }

  // MCDisassembler
  DisAsm = TheTarget->createMCDisassembler(*STI, *MCCtx);
  if (DisAsm == NULL) {
    printError("Unable to create MCDisassembler.");
  }

  // MCRegisterInfo
  MRI = TheTarget->createMCRegInfo(TripleName);
  if (MRI == NULL) {
    printError("Unable to create MCRegisterInfo.");
  }

  // MCObjectFileInfo
  MCOFI = new MCObjectFileInfo();

  // MCAsmInfo
  AsmInfo = TheTarget->createMCAsmInfo(*MRI, TripleName);
  if (AsmInfo == NULL) {
    printError("No Assembly info for Target available.");
  }

  // MCContext
  MCCtx = new MCContext(AsmInfo, MRI, MCOFI);

  // NOTE: Circular dependency is why the init AFTER MCContext
  // for MCObjectFileInfo
  MCOFI->InitMCObjectFileInfo(TripleName, RM, CM, *MCCtx);

  // MCInstrInfo
  MII = TheTarget->createMCInstrInfo();
  if (MII == NULL) {
    printError("No InstrInfo for Target available.");
  }

  // MCInstPrinter
  MIP = TheTarget->createMCInstPrinter(AsmInfo->getAssemblerDialect(), *AsmInfo,
    *MII, *MRI, *STI);
  if (MIP == NULL) {
    printError("No instruction printer for target.");
  }
  MIP->setPrintImmHex(1);
}

MCDirector::~MCDirector() {
  delete MIP;
  delete MII;
  delete AsmInfo;
  delete MCCtx;
  delete MCOFI;
  delete MRI;
  delete DisAsm;
  delete STI;
  delete TM;
  delete TheTarget;
  delete TOpts;
  delete LLVMCtx;
}

bool MCDirector::isValid() {
  return (TheTarget && TM && STI && DisAsm && MRI && MCOFI && MCCtx && AsmInfo
          && MII && MIP);
}

EVT MCDirector::getRegType(unsigned RegisterID) {
  const TargetRegisterInfo *TRI = TM->getRegisterInfo();
  if (RegTypes.size() == 0) {
    RegTypes.grow(TRI->getNumRegs());
  }
  EVT NullEVT;
  // Add to the map if it's not already there
  if (RegTypes[RegisterID] == NullEVT) {
    // Get the minimum physical subreg class
    const TargetRegisterClass* MinRC = NULL;
    for (TargetRegisterInfo::regclass_iterator TRS =
           TRI->regclass_begin(), TRE = TRI->regclass_end(); TRS != TRE;
         ++TRS) {
      const TargetRegisterClass *RC = *TRS;
      if (RC->contains(RegisterID) && (!MinRC || MinRC->hasSubClass(RC))) {
        MinRC = RC;
      }
    }

    if (MinRC == NULL) {
      MinRC = *(TRI->regclass_begin());
    }

    RegTypes[RegisterID] = *(MinRC->vt_begin());
  }
  return RegTypes[RegisterID];
}

} // end namespace fracture
