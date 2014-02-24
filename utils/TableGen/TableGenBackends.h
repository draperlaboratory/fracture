//===- TableGenBackends.h - Declarations for Fracture TableGen Backends ---===//
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
// This file contains the declarations for all of the Fracture TableGen
// backends. A "TableGen backend" is just a function. See
// "$LLVM_ROOT/utils/TableGen/TableGenBackends.h" for more info.
//
//===----------------------------------------------------------------------===//

#include <string>

namespace llvm {
  class raw_ostream;
  class RecordKeeper;
}

using llvm::raw_ostream;
using llvm::RecordKeeper;

namespace fracture {

void EmitPatternlessInstrs(RecordKeeper &RK, raw_ostream &OS);
void EmitInstrMap(RecordKeeper &RK, raw_ostream &OS);

} // end namespace clang
