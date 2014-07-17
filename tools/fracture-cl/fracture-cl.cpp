//===--- fracture-cl.cpp - Fracture Shell -----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The fracture shell provides a command line interface to permit the user
// to interactively disassemble and output a binary executable in LLVM-IR.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include <string>
#include <algorithm>
#include <map>
#include <inttypes.h>
#include <signal.h>
#include <sstream>
		#include <unistd.h>  //new
		#include <cstdlib>  //new



// iostream is frowned upon in LLVM, but
// we are doing console I/O here.
#include <iostream>
#include <iomanip>

#include "BinFun.h"
#include "DummyObjectFile.h"
#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
//#include "CodeInv/InvISelDAG.h"
//#include "CodeInv/MCDirector.h"
#include "Commands/Commands.h"

// #define DEMANGLE  // Do name demangling
// #ifdef DEMANGLE
// #include <cxxabi.h>
// #include <malloc.h>
// #endif

using namespace llvm;
using namespace fracture;
		using std::string;  //new
//===----------------------------------------------------------------------===//
// Global Variables and Parameters
//===----------------------------------------------------------------------===//
static std::string ProgramName;
static Commands CommandParser;

MCDirector *MCD = 0;
Disassembler *DAS = 0;
Decompiler *DEC = 0;
OwningPtr<object::ObjectFile> TempExecutable;

//Command Line Options
cl::opt<std::string> TripleName("triple",
    cl::desc("Target triple to disassemble for, "
        "see -version for available targets"));
cl::opt<std::string> ArchName("arch",
    cl::desc("Target arch to disassemble for, "
        "see -version for available targets"));
cl::opt<std::string> InputFileName(cl::Positional, cl::desc("<input file>"),
    cl::init("-"));

cl::list<std::string> MAttrs("mattr", cl::CommaSeparated,
    cl::desc("Target specific attributes"), cl::value_desc("a1,+a2,-a3,..."));

static cl::opt<bool> ViewMachineDAGs("view-machine-dags", cl::Hidden,
    cl::desc("Pop up a window to show dags before Inverse DAG Select."));

static cl::opt<bool> ViewIRDAGs("view-ir-dags", cl::Hidden,
    cl::desc("Pop up a window to show dags after Inverse DAG Select."));

static bool error(error_code ec) {
  if (!ec)
    return false;

  errs() << ProgramName << ": error reading file: " << ec.message() << ".\n";
  return true;
}

///===---------------------------------------------------------------------===//
/// loadBinary      - Tries to open the file and set the ObjectFile.
/// NOTE: Binary is a subclass of ObjectFile, but Binary multiply inherits
/// from Archive as well, and we want objects in a format with sections.
///
/// @param FileName - The name of the file to open.
///
static error_code loadBinary(StringRef FileName) {
  // File should be stdin or it should exist.
  if (FileName != "-" && !sys::fs::exists(FileName)) {
    errs() << ProgramName << ": No such file or directory: '" << FileName.data()
        << "'.\n";
    return make_error_code(errc::no_such_file_or_directory);
  }

  ErrorOr<object::Binary*> Binary = object::createBinary(FileName);
  if (error_code err = Binary.getError()) {
    errs() << ProgramName << ": Unknown file format: '" << FileName.data()
        << "'.\n Error Msg: " << err.message() << "\n";

    OwningPtr<MemoryBuffer> MemBuf;
    if (error_code err = MemoryBuffer::getFile(FileName, MemBuf)) {
      errs() << ProgramName << ": BLAH!: '" << FileName.data() << "'.\n";
      return err;
    }

    OwningPtr<object::ObjectFile> ret(
        object::DummyObjectFile::createDummyObjectFile(MemBuf.take()));
    TempExecutable.swap(ret);
  } else {
    if (Binary.get()->isObject()) {
      OwningPtr<object::ObjectFile> ret(
        dyn_cast<object::ObjectFile>(Binary.get()));
      TempExecutable.swap(ret);
    }
  }

  // Initialize the Disassembler
  std::string FeaturesStr;
  if (MAttrs.size()) { 
    SubtargetFeatures Features;
    for (unsigned int i = 0; i < MAttrs.size(); ++i) {
      Features.AddFeature(MAttrs[i]);
    }
    FeaturesStr = Features.getString();
  }

  // Arch-Vendor-OS[-Env]
  // Figure out the target triple.
  Triple TT("unknown-unknown-unknown");
  if (TripleName.empty()) {
    TT.setArch(Triple::ArchType(TempExecutable->getArch()));
  } else {
    TT.setTriple(Triple::normalize(TripleName));
  }
  if (!ArchName.empty())
    TT.setArchName(ArchName);

  TripleName = TT.str();

  delete DEC;
  delete DAS;
  delete MCD;

  MCD = new MCDirector(TripleName, "generic", FeaturesStr,
    TargetOptions(), Reloc::Default, CodeModel::Default, CodeGenOpt::Default,
    outs(), errs());
  DAS = new Disassembler(MCD, TempExecutable.get(), NULL, outs(), outs());
  DEC = new Decompiler(DAS, NULL, outs(), outs());

  if (!MCD->isValid()) {
    errs() << "Warning: Unable to initialized LLVM MC API!\n";
    return make_error_code(errc::not_supported);
  }

  return error_code::success();
}

///===---------------------------------------------------------------------===//
/// printHelp       - Prints the possible commands
/// TODO: Expand this to print descriptions of the commands.
/// TODO: Expand to print help for subsections and all sections.
///
static void printHelp(std::vector<std::string> &CommandLine) {
  std::map<std::string, void (*)(std::vector<std::string> &)> Commands =
      CommandParser.getCmdMap();
  for (std::map<std::string, void (*)(std::vector<std::string> &)>::iterator
      CmdIt = Commands.begin(), CmdEnd = Commands.end(); CmdIt != CmdEnd;
      ++CmdIt) {
    if (CmdIt != Commands.begin())
      outs() << ",";
    outs() << CmdIt->first;
  }
  outs() << "\n";
}

///===---------------------------------------------------------------------===//
/// runLoadCommand   - Loads an executable by reading a file
///
/// @param Executable - The executable under analysis.
///
static void runLoadCommand(std::vector<std::string> &CommandLine) {
  StringRef FileName;
  if (CommandLine.size() >= 2)
    FileName = CommandLine[1];
  if (error_code Err = loadBinary(FileName)) {
    errs() << ProgramName << ": Could not open the file '" << FileName.data()
        << "'. " << Err.message() << ".\n";
  }
}

///===---------------------------------------------------------------------===//
/// lookupELFName   - With an ELF file, lookup a function address based on its name.
///
/// @param Executable - The executable under analysis.
///
template <class ELFT>
static bool lookupELFName(const object::ELFObjectFile<ELFT>* elf,
  StringRef funcName, uint64_t &Address ) {
  bool retVal = false;
  error_code ec;
  std::vector<object::SymbolRef> Syms;
  Address = 0;
  for (object::symbol_iterator si = elf->begin_symbols(), se =
         elf->end_symbols(); si != se; ++si) {
    Syms.push_back(*si);
  }
  // for (object::symbol_iterator si = elf->begin_dynamic_symbols(), se =
  //        elf->end_dynamic_symbols(); si != se; ++si) {
  //   Syms.push_back(*si);
  // }

  for (std::vector<object::SymbolRef>::iterator si = Syms.begin(),
      se = Syms.end();
      si != se; ++si) {

    if (error(ec))
      return retVal;

    StringRef Name;

    if (error(si->getName(Name)))
      continue;
    if (error(si->getAddress(Address)))
      continue;

    if (Address == object::UnknownAddressOrSize){
      retVal = false;
      Address = 0;
    }

    if(funcName.str() == Name.str()){
      retVal = true;
      return retVal;
    }
  }
  return retVal;
}

///===---------------------------------------------------------------------===//
/// nameLookupAddr - lookup a function address based on its name.
/// @note: COFF support function has not been written yet...
///
/// @param Executable - The executable under analysis.
///
static bool nameLookupAddr(StringRef funcName, uint64_t &Address) {
  bool retVal = false;
  const object::ObjectFile* Executable = DAS->getExecutable();

  Address = 0;

  if (//const object::COFFObjectFile *coff =
    dyn_cast<const object::COFFObjectFile>(Executable)) {
    // dumpCOFFSymbols(coff, Address);
    errs() << "COFF is Unsupported section type.\n";
  } else if (const object::ELF32LEObjectFile *elf =
    dyn_cast<const object::ELF32LEObjectFile>(Executable)) {
    retVal = lookupELFName(elf, funcName, Address );
  } else if (const object::ELF32BEObjectFile *elf =
    dyn_cast<const object::ELF32BEObjectFile>(Executable)) {
    retVal = lookupELFName(elf, funcName, Address );
  } else if (const object::ELF64BEObjectFile *elf =
    dyn_cast<const object::ELF64BEObjectFile>(Executable)) {
    retVal = lookupELFName(elf, funcName, Address );
  } else if (const object::ELF64LEObjectFile *elf =
    dyn_cast<const object::ELF64LEObjectFile>(Executable)) {
    retVal = lookupELFName(elf, funcName, Address );
  } else {
    errs() << "Unsupported section type.\n";
  }
  return retVal;
}

///===---------------------------------------------------------------------===//
/// runDecompileCommand - Decompile a basic block at a given memory address.
///
/// @param Executable - The executable under analysis.
///
static void runDecompileCommand(std::vector<std::string> &CommandLine) {
  uint64_t Address;
  StringRef FunctionName;

  if (CommandLine.size() != 2) {
    errs() << "runDecompileCommand: invalid command"
           << "format: decompile <address or function> \n";
    return;
  }

  // Get function name or address and print them
  if (StringRef(CommandLine[1]).getAsInteger(0, Address)) {
    FunctionName = CommandLine[1];
    if(nameLookupAddr(FunctionName, Address) == false){
      errs() << "Error retrieving address based on function name.\n";
      return;
    }
  }
//  Commenting this out so raw binaries can be decompiled at 0x0
//  if (Address == 0) {
//    errs() << "runDecompileCommand: invalid address or function name.\n";
//    return;
//  }

  DEC->setViewMCDAGs(ViewMachineDAGs);
  DEC->setViewIRDAGs(ViewIRDAGs);

  formatted_raw_ostream Out(outs(), false);
  DEC->decompile(Address);
  DEC->printInstructions(Out, Address);
}

///===---------------------------------------------------------------------===//
/// runDisassembleCommand - Disassemble a given memory address.
///
/// @param Executable - The executable under analysis.
///
static void runDisassembleCommand(std::vector<std::string> &CommandLine) {
  uint64_t NumInstrs, Address, NumInstrsPrinted;
  StringRef FunctionName;

  if (CommandLine.size() < 2 || CommandLine.size() > 3) {
    errs() << "runDisassemblerCommand: invalid command"
           << "format: disassemble <address or function name> "
           << "[num of instructions] \n";
    return;
  }

  NumInstrs = 0;
  // Parse Num instructions (if it is given)
  if (CommandLine.size() == 3) {
    if (StringRef(CommandLine[2]).getAsInteger(0, NumInstrs)) {
      NumInstrs = 0;
    }
  }

  // Get function name or address and print them
  if (StringRef(CommandLine[1]).getAsInteger(0, Address)) {
    FunctionName = CommandLine[1];
    if(nameLookupAddr(FunctionName, Address) == false){
      errs() << "Error retrieving address based on function name.\n";
      return;
    }
  }

//  Commenting this out so raw binaries can be disassembled at 0x0
//  if (Address == 0) {
//    errs() << "runDisassemblerCommand: invalid address or function name.\n";
//    return;
//  }

  formatted_raw_ostream Out(outs(), false);
  Out << "Address: " << Address << "\nNumInstrs: " << NumInstrs << "\n";
  NumInstrsPrinted = DAS->printInstructions(Out, Address, NumInstrs, false);
  if (NumInstrs != 0 && NumInstrsPrinted != NumInstrs) {
    outs() << "runDisassemblerCommand Warning: " << NumInstrsPrinted << " of "
           << NumInstrs << " printed.\n";
  }
}

static void runSectionsCommand(std::vector<std::string> &CommandLine) {
  outs() << "Sections:\n"
         << "Idx Name          Size      Address          Type\n";
  error_code ec;
  unsigned i = 1;
  for (object::section_iterator si = DAS->getExecutable()->section_begin(),
         se = DAS->getExecutable()->section_end(); si != se; ++si) {
    if (error(ec))
      return;
    StringRef Name;
    if (error(si->getName(Name)))
      return;
    uint64_t Address;
    if (error(si->getAddress(Address)))
      return;
    uint64_t Size;
    if (error(si->getSize(Size)))
      return;
    bool Text, Data, BSS;
    if (error(si->isText(Text)))
      return;
    if (error(si->isData(Data)))
      return;
    if (error(si->isBSS(BSS)))
      return;
    std::string Type =
      (std::string(Text ? "TEXT " : "") + (Data ? "DATA " : "")
        + (BSS ? "BSS" : ""));
    outs() << format("%3d %-13s %08" PRIx64 " %016" PRIx64 " %s\n",
      i, Name.str().c_str(), Size, Address, Type.c_str());
    ++i;
  }
}

template <class ELFT>
static void dumpELFSymbols(const object::ELFObjectFile<ELFT>* elf,
  unsigned Address) {
  error_code ec;
  std::vector<object::SymbolRef> Syms;
  for (object::symbol_iterator si = elf->begin_symbols(), se =
         elf->end_symbols(); si != se; ++si) {
    Syms.push_back(*si);
  }
  for (std::vector<object::SymbolRef>::iterator si = Syms.begin(),
         se = Syms.end();
       si != se; ++si) {
    if (error(ec))
      return;
    StringRef Name;
    uint64_t Address;
    object::SymbolRef::Type Type;
    uint64_t Size;
    uint32_t Flags = 0;
    uint64_t SectAddr;
    uint64_t Value;
    object::section_iterator Section = elf->section_end();
    if (error(si->getName(Name)))
      continue;
    if (error(si->getAddress(Address)))
      continue;
    if (error(si->getValue(Value)))
      continue;
    if (error(si->getSection(Section)))
      continue;
    if (error(Section->getAddress(SectAddr)))
      continue;
    if (error(si->getType(Type)))
      continue;
    if (error(si->getSize(Size)))
      continue;

    // Doesn't print symbol information for symbols which aren't in the section
    // specified by the function parameter
    if (SectAddr == Address)
      continue;

    bool Global = Flags & object::SymbolRef::SF_Global;
    bool Weak = Flags & object::SymbolRef::SF_Weak;

    if (Address == object::UnknownAddressOrSize)
      Address = 0;
    if (Size == object::UnknownAddressOrSize)
      Size = 0;
    char GlobLoc = ' ';
    if (Type != object::SymbolRef::ST_Unknown)
      GlobLoc = Global ? 'g' : 'l';
    char Debug =
      (Type == object::SymbolRef::ST_Debug
        || Type == object::SymbolRef::ST_File) ?
      'd' : ' ';
    char FileFunc = ' ';
    if (Type == object::SymbolRef::ST_File)
      FileFunc = 'f';
    else if (Type == object::SymbolRef::ST_Function)
      FileFunc = 'F';

    const char *Fmt;

    Fmt = elf->getBytesInAddress() > 4 ? "%016" PRIx64 :
      "%08" PRIx64;

    outs() << format(Fmt, Address) << " "
           << GlobLoc  // Local -> 'l', Global -> 'g', Neither -> ' '
           << (Weak ? 'w' : ' ')      // Weak?
           << ' '      // Constructor. Not supported yet.
           << ' '      // Warning. Not supported yet.
           << ' '      // Indirect reference to another symbol.
           << Debug    // Debugging (d) or dynamic (D) symbol.
           << FileFunc // Name of function (F), file (f) or object (O).
           << ' ';
    outs() << '\t'
           << format("%08" PRIx64 " ", Size)
           << format("%08" PRIx64 " ", Value)
           << Name
           << '\n';
  }
}

static void dumpCOFFSymbols(const object::COFFObjectFile *coff,
  uint64_t Address) {

	const object::pe32_header *peh;
	coff->getPE32Header(peh);

	outs() << "Start Address: " << peh->AddressOfEntryPoint + peh->ImageBase << "\n";
	outs() << "BaseOfCode: " << peh->BaseOfCode << "\n";
	outs() << "BaseOfData: " << peh->BaseOfData << "\n";
	outs() << "ImageBase: " << peh->ImageBase << "\n";

	/* AJG: I think this could read the complete section (I have not proven it to myself yet
	 error_code ec = coff->getRvaPtr(Address, Res);
	 if(ec!=object::object_error::success)
		 outs() << "Have an error?\n";
	  */


  // Find the section index (referenced by symbol)
  int SectionIndex = -1;
  int Index = 1;
  error_code ec;
  for (object::section_iterator si = coff->section_begin(), se =
         coff->section_end(); si != se; ++si, ++Index) {
    uint64_t SectionAddr;
    if (error(si->getAddress(SectionAddr)))
      break;
    uint64_t SectionSize;
    if (error(si->getSize(SectionSize)))
      break;
    if (SectionAddr <= Address && Address < SectionAddr + SectionSize) {
      SectionIndex = Index;
      break;
    }
  }
  if (SectionIndex == -1) {
    outs() << "No section found with that name or containing that address\n";
    return;
  }

  const object::coff_file_header *header;
  if (error(coff->getHeader(header)))
    return;
  int aux_count = 0;
  const object::coff_symbol *symbol = 0;
  for (int i = 0, e = header->NumberOfSymbols; i != e; ++i) {
    if (aux_count--) {
      // Figure out which type of aux this is.
      if (symbol->StorageClass == COFF::IMAGE_SYM_CLASS_STATIC
        && symbol->Value == 0) { // Section definition.
        const object::coff_aux_section_definition *asd;
        if (error(coff->getAuxSymbol<object::coff_aux_section_definition>(i,
              asd)))
          return;
        outs() << "AUX "
               << format("scnlen 0x%x nreloc %d nlnno %d checksum 0x%x ",
                 unsigned(asd->Length), unsigned(asd->NumberOfRelocations),
                 unsigned(asd->NumberOfLinenumbers), unsigned(asd->CheckSum))
               << format("assoc %d comdat %d\n", unsigned(asd->Number),
                 unsigned(asd->Selection));
      } else
        outs() << "AUX Unknown\n";
    } else {
      StringRef name;
      if (error(coff->getSymbol(i, symbol)))
        return;
      if (error(coff->getSymbolName(symbol, name)))
        return;
      if ((int) symbol->SectionNumber != SectionIndex) {
        aux_count = symbol->NumberOfAuxSymbols;
        continue;
      }

      outs() << "[" << format("%2d", i) << "]" << "(sec "
             << format("%2d", int(symbol->SectionNumber)) << ")" << "(fl 0x00)"
             // Flag bits, which COFF doesn't have.
             << "(ty " << format("%3x", unsigned(symbol->Type)) << ")"
             << "(scl "
             << format("%3x", unsigned(symbol->StorageClass)) << ") "
             << "(nx "
             << unsigned(symbol->NumberOfAuxSymbols) << ") " << "0x"
             << format("%08x", unsigned(symbol->Value)) << " " << name << "\n";
      aux_count = symbol->NumberOfAuxSymbols;
    }
  }

}

static void runSymbolsCommand(std::vector<std::string> &CommandLine) {
  if (CommandLine.size() < 2) {
    outs() << "Did not understand section name or address.\n";
    return;
  }

  StringRef SectionNameOrAddress = CommandLine[1];
  const object::ObjectFile* Executable = DAS->getExecutable();

  error_code ec;
  uint64_t Address;
  object::SectionRef Section = *Executable->section_end();
  if (SectionNameOrAddress.getAsInteger(0, Address) && Address != 0) {
    Section = DAS->getSectionByAddress(Address);
  }

  if (Section == *Executable->section_end()) {
    Section = DAS->getSectionByName(SectionNameOrAddress);
  }

  if (Section == *Executable->section_end()) {
    errs() << "Could not find section!\n";
    return;
  }

  if (error(Section.getAddress(Address))) {
    return;
  }

  StringRef SectionName;
  error(Section.getName(SectionName));
  outs() << "SYMBOL TABLE FOR SECTION " << SectionName << " at 0x"
         << format("%08x", unsigned(Address)) << "\n";

  if (const object::COFFObjectFile *coff =
    dyn_cast<const object::COFFObjectFile>(Executable)) {
    dumpCOFFSymbols(coff, Address);
  } else if (const object::ELF32LEObjectFile *elf =
    dyn_cast<const object::ELF32LEObjectFile>(Executable)) {
    return dumpELFSymbols(elf, Address);
  } else if (const object::ELF32BEObjectFile *elf =
    dyn_cast<const object::ELF32BEObjectFile>(Executable)) {
    return dumpELFSymbols(elf, Address);
  } else if (const object::ELF64BEObjectFile *elf =
    dyn_cast<const object::ELF64BEObjectFile>(Executable)) {
    return dumpELFSymbols(elf, Address);
  } else if (const object::ELF64LEObjectFile *elf =
    dyn_cast<const object::ELF64LEObjectFile>(Executable)) {
    return dumpELFSymbols(elf, Address);
  } else {
    errs() << "Unsupported section type.\n";
  }
}

///===---------------------------------------------------------------------===//
/// runSaveCommand - Saves current module to a .ll file
///
static void runSaveCommand(std::vector<std::string> &CommandLine) {
  if (CommandLine.size() != 2) {
    outs() << "usage: save <filename.ll>\n";
    return;
  }

  std::string ErrorInfo;
  raw_fd_ostream FOut(CommandLine[1].c_str(), ErrorInfo);

  FOut << *(DEC->getModule());

  if (!ErrorInfo.empty()) {
    outs() << "Errors on write: \n" << ErrorInfo << "\n";
  }
}

///===---------------------------------------------------------------------===//
/// runQuitCommand - Exits the program
///
static void runQuitCommand(std::vector<std::string> &CommandLine) {
	// was 130 but changed to 0 because this exit is after success
	exit(0);  //Note: This is for fork/exec in shell.
}

static void runDumpCommand(std::vector<std::string> &CommandLine) {
  uint64_t NumLinesToDump, Address;
  StringRef NumLinesRef;

  if (CommandLine.size() < 2) {
    errs() << "dump <address> [numlines]\n";
    return;
  }

  StringRef AddrRef = CommandLine[1];
  if (!AddrRef.getAsInteger(0, Address)) {
    errs() << "Invalid address!\n";
    return;
  }

  if (CommandLine.size() >= 3) {
    NumLinesRef = CommandLine[2];
  } else {
    NumLinesRef = "10";
  }
  NumLinesRef.getAsInteger(0, NumLinesToDump);

  object::SectionRef Section = DAS->getSectionByAddress(Address);
  StringRef Name;
  StringRef Contents;
  uint64_t BaseAddr;
  bool BSS;
  if (error(Section.getName(Name)))
    return;
  if (error(Section.getContents(Contents)))
    return;
  if (error(Section.getAddress(BaseAddr)))
    return;
  if (error(Section.isBSS(BSS)))
    return;

  if (Section == *DAS->getExecutable()->section_end()) {
    outs() << "No section found with that name or containing that address\n";
    return;
  }

  outs() << "Contents of section " << Name << ":\n";
  if (BSS) {
    outs() << format("<skipping contents of bss section at [%04" PRIx64
      ", %04" PRIx64 ")>\n", BaseAddr, BaseAddr + Contents.size());
    return;
  }

  uint64_t NumLinesDumped = 0;

  // Dump out the content as hex and printable ascii characters.
  for (std::size_t Index = Address, end = BaseAddr + Contents.size();
       Index < end && NumLinesDumped < NumLinesToDump;
       Index += 16, ++NumLinesDumped) {
    outs() << format(" %04" PRIx64 " ", Index);
    // Dump line of hex.
    for (std::size_t i = 0; i < 16; ++i) {
      if (i != 0 && i % 4 == 0)
        outs() << ' ';
      if (Index + i < end)
        outs() << hexdigit((Contents[Index - BaseAddr + i] >> 4) & 0xF, true)
               << hexdigit(Contents[Index - BaseAddr + i] & 0xF, true);
      else
        outs() << "  ";
    }
    // Print ascii.
    outs() << "  ";
    for (std::size_t i = 0; i < 16 && Index + i < end; ++i) {
      if (std::isprint(
          static_cast<unsigned char>(Contents[Index - BaseAddr + i]) & 0xFF))
        outs() << Contents[Index - BaseAddr + i];
      else
        outs() << ".";
    }
    outs() << "\n";
  }
}

static void initializeCommands() {
  CommandParser.registerCommand("?", &printHelp);
  CommandParser.registerCommand("help", &printHelp);
  CommandParser.registerCommand("decompile", &runDecompileCommand);
  CommandParser.registerCommand("disassemble", &runDisassembleCommand);
  CommandParser.registerCommand("dump", &runDumpCommand);
  CommandParser.registerCommand("load", &runLoadCommand);
  CommandParser.registerCommand("quit", &runQuitCommand);
  CommandParser.registerCommand("sections", &runSectionsCommand);
  CommandParser.registerCommand("symbols", &runSymbolsCommand);
  CommandParser.registerCommand("save", &runSaveCommand);
  // TODO:
  // CommandParser.registerCommand("cfg", &runCfgCommand);
  // CommandParser.registerCommand("functions", &runFunctionsCommand);
}

int main(int argc, char *argv[]) {
  ProgramName = argv[0];
  if(ProgramName.find("./")==0){
	  // Remove the "./" from the beginning of the program name
	  ProgramName = ProgramName.substr(2, ProgramName.length() - 2);
  }

  // If no parameter is given to dish, stop execution
  if (argc < 2) {
    // Tell the user how to run the program
    errs() << ProgramName << ": No positional arguments specified!" << "\n";
    errs() << "Must specify exactly 1 positional argument: See: ./"
        << ProgramName << " -help" << "\n";
    return 1;
  }

  // Stack trace err hdlr
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  // Calls a shutdown function when destructor is called
  llvm_shutdown_obj Y;

  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllDisassemblers();
  InitializeAllTargets();

  // Register the target printer for --version.
  cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);

  cl::ParseCommandLineOptions(argc, argv, "DIsassembler SHell");

  initializeCommands();

  if (error_code Err = loadBinary(InputFileName.getValue())) {
    errs() << ProgramName << ": Could not open the file '"
        << InputFileName.getValue() << "'. " << Err.message() << ".\n";
  }

  CommandParser.runShell(ProgramName);

  return 0;
}
