//===- TableGen.cpp - Top-Level TableGen implementation for Fracture-------===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
// The following license applies to utils/TableGen in r173931 of clang:
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Use "diff" to determine changes made. 
//
//===----------------------------------------------------------------------===//
//
// This file contains the main function for Fracture's TableGen.
//
//===----------------------------------------------------------------------===//

#include "TableGenBackends.h" // Declares all backends.
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"

using namespace llvm;
using namespace fracture;

enum ActionType {
  GenPatternlessInstrs,
  GenInstrMap
};

namespace {
  cl::opt<ActionType>
  Action(cl::desc("Action to perform:"),
         cl::values(
           clEnumValN(GenPatternlessInstrs, "gen-patternless-instrs",
             "Generate list of patternless instructions"),
           clEnumValN(GenInstrMap, "gen-instr-map",
             "Generate map of instructions to generic instructions"),
           clEnumValEnd)
         );

bool FractureTableGenMain(raw_ostream &OS, RecordKeeper &Records) {
  switch (Action) {
  case GenPatternlessInstrs:
    EmitPatternlessInstrs(Records, OS);
    break;
  case GenInstrMap:
    EmitInstrMap(Records, OS);
    break;
  }

  return false;
}
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);

  return TableGenMain(argv[0], &FractureTableGenMain);
}
