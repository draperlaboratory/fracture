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
#include "llvm/Support/COFF.h"
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
#include <ostream>
#include <iomanip>
#include <stdio.h>
#include <fstream>
#include "BinFun.h"
#include "DummyObjectFile.h"
#include "CodeInv/Decompiler.h"
#include "CodeInv/Disassembler.h"
#include "CodeInv/StrippedDisassembler.h"
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
StrippedDisassembler *SDAS = 0;
std::unique_ptr<object::ObjectFile> TempExecutable;
bool isStripped = false;

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

static cl::opt<bool> StrippedBinary("stripped", cl::Hidden,
    cl::desc("Run stripped disassembler to locate functions in stripped binary."));

static cl::opt<bool> printGraph("print-graph", cl::Hidden,
    cl::desc("Print graph for stripped file, must also enable stripped command"));


static bool error(std::error_code ec) {
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
static std::error_code loadBinary(StringRef FileName) {
  // File should be stdin or it should exist.
  if (FileName != "-" && !sys::fs::exists(FileName)) {
    errs() << ProgramName << ": No such file or directory: '" << FileName.data()
        << "'.\n";
    return make_error_code(std::errc::no_such_file_or_directory);
  }

  ErrorOr<object::OwningBinary<object::Binary> > Binary
    = object::createBinary(FileName);
  if (std::error_code err = Binary.getError()) {
    errs() << ProgramName << ": Unknown file format: '" << FileName.data()
        << "'.\n Error Msg: " << err.message() << "\n";

    ErrorOr<std::unique_ptr<MemoryBuffer>> MemBuf =
      MemoryBuffer::getFile(FileName);
    if (std::error_code err = MemBuf.getError()) {
      errs() << ProgramName << ": Bad Memory!: '" << FileName.data() << "'.\n";
      return err;
    }

    std::unique_ptr<object::ObjectFile> ret(
      object::DummyObjectFile::createDummyObjectFile(MemBuf.get()));
    TempExecutable.swap(ret);
  } else {
    if (Binary.get().getBinary()->isObject()) {
      std::pair<std::unique_ptr<object::Binary>, std::unique_ptr<MemoryBuffer> >
        res = Binary.get().takeBinary();
      ErrorOr<std::unique_ptr<object::ObjectFile> > ret
        = object::ObjectFile::createObjectFile(
          res.second.release()->getMemBufferRef());
      TempExecutable.swap(ret.get());
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
    TargetOptions(), Reloc::DynamicNoPIC, CodeModel::Default, CodeGenOpt::Default,
    outs(), errs());
  DAS = new Disassembler(MCD, TempExecutable.release(), NULL, outs(), outs());
  DEC = new Decompiler(DAS, NULL, outs(), outs());

  if (!MCD->isValid()) {
    errs() << "Warning: Unable to initialized LLVM MC API!\n";
    return make_error_code(std::errc::not_supported);
  }

  return std::error_code();
}

// Hasher for using strings in switch statements
constexpr unsigned int str2int(const char* str, int h = 0) {
  return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

///===---------------------------------------------------------------------===//
/// printHelp       - Prints the possible commands
/// TODO: Expand this to print descriptions of the commands.
/// TODO: Expand to print help for subsections and all sections.
///
static void printHelp(std::vector<std::string> &CommandLine) {
  std::map<std::string, void (*)(std::vector<std::string> &)> Commands =
      CommandParser.getCmdMap();
  outs() << "\n--COMMANDS--\n\n";
  for (std::map<std::string, void (*)(std::vector<std::string> &)>::iterator
      CmdIt = Commands.begin(), CmdEnd = Commands.end(); CmdIt != CmdEnd;
      ++CmdIt) {
    switch(str2int(CmdIt->first.c_str())) {
      case  str2int("?") :
        outs() << "? - Displays usable commands and descriptions "
               << "of their uses\n\n\n";
        break;
      case  str2int("decompile") :
        outs() << "decompile - Decompile a given function\n"
               << "USAGE:\n"
               << "\tdec [FUNCNAME] or dec [FUNCADDRESS]\n"
               << "DESCRIPTION:\n"
               << "\tDecompile a machine function into LLVM IR given a "
               << "function name or\n\tfunction address\n\n\n";
        break;
      case  str2int("disassemble") :
        outs() << "disassemble - Disassemble a given function\n"
               << "USAGE:\n"
               << "\tdis [FUNCNAME] or dis [FUNCADDRESS]\n"
               << "DESCRIPTION:\n"
               << "\tDisassemble a machine function into architecture-specific"
               << " assembly\n\tlanguage given a function name or function "
               << "address\n\n\n";
        break;
      case  str2int("dump") :
        outs() << "dump - Fill me in...\n\n\n";
        break;
      case  str2int("help") :
        outs() << "help - Displays usable commands and descriptions "
               << "of their uses\n\n\n";
        break;
      case  str2int("load") :
        outs() << "load - Load a binary into Fracture\n"
               << "USAGE:\n"
               << "\tload [FILENAME]\n"
               << "DESCRIPTION:\n"
               << "\tLoad a given binary into Fracture while Fracture is "
               << "already running\n\n\n";
        break;
      case  str2int("quit") :
        outs() << "quit - Terminate the application\n\n\n";
        break;
      case  str2int("save") :
        outs() << "save - Save LLVM IR to a file\n"
               << "USAGE:\n"
               << "\tsave [FILENAME]\n"
               << "DESCRIPTION:\n"
               << "\tSave decompiled LLVM IR to a file using the specified file"
               << "name.\n\tThe decompile command must be run before running "
               << "the save command.\n\tDecompiled LLVM IR should be saved as a"
               << " .ll file, which can then be\n\trun using the lli command"
               << " outside of fracture.\n\n\n";
               break;
      case  str2int("sections") :
        outs() << "sections - Print the names of all sections contained"
               << " in the binary\n\n\n";
        break;
      case  str2int("symbols") :
        outs() << "symbols - Print section symbols\n"
               << "USAGE:\n"
               << "\tsym [SECTIONNAME]\n"
               << "DESCRIPTION:\n"
               << "\tPrint all symbols contained in a given section, sorted by"
               << " address.\n\n";
               break;
    }
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
  if (std::error_code Err = loadBinary(FileName)) {
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
  std::error_code ec;
  std::vector<FractureSymbol *> Syms;

  Address = 0;
  for (object::symbol_iterator si = elf->symbols().begin(), se =
         elf->symbols().end(); si != se; ++si) {
    Syms.push_back(new FractureSymbol(*si));
  }
  for (object::symbol_iterator si = elf->dynamic_symbol_begin(), se =
          elf->dynamic_symbol_end(); si != se; ++si) {
    FractureSymbol *temp = new FractureSymbol(*si);
    Syms.push_back(temp);
  }
  if (isStripped)
      for (auto &it : SDAS->getStrippedGraph()->getHeadNodes()) {
        StringRef name = (SDAS->getMain() == it->Address ?
                                  "main" : DAS->getFunctionName(it->Address));
        FractureSymbol tempSym(it->Address, name,
                               0, object::SymbolRef::Type::ST_Function, 0);
        Syms.push_back(new FractureSymbol(tempSym));
      }

  for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
      se = Syms.end();
      si != se; ++si) {
    if (error(ec)){
      for(auto &it : Syms)
        delete it;
      return retVal;
    }

    StringRef Name;

    if (error((*si)->getName(Name)))
      continue;
    if (error((*si)->getAddress(Address)))
      continue;

    if (Address == object::UnknownAddressOrSize) {
      retVal = false;
      Address = 0;
    }

    if (funcName.str() == Name.str()) {
      retVal = true;
      for(auto &it : Syms)
        delete it;
      return retVal;
    }
  }
  for (auto &it : Syms)
    delete it;
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



    //Binary is not stripped, return address based on symbol name
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
  //Set section to section containing address
  StringRef SectionName;
  object::SectionRef Section = DAS->getSectionByAddress(Address);
  DAS->setSection(Section);

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
         << "Idx Name               Size      Address          Type\n";
  std::error_code ec;
  unsigned i = 1;
  for (object::section_iterator si = DAS->getExecutable()->section_begin(),
         se = DAS->getExecutable()->section_end(); si != se; ++si) {
    if (error(ec))
      return;
    StringRef Name;
    if (error(si->getName(Name)))
      return;
    uint64_t Address = si->getAddress();
    uint64_t Size = si->getSize();
    bool Text, Data, BSS;
    Text = si->isText();
    Data = si->isData();
    BSS = si->isBSS();
    std::string Type =
      (std::string(Text ? "TEXT " : "") + (Data ? "DATA " : "")
        + (BSS ? "BSS" : ""));
    outs() << format("%3d %-18s %08" PRIx64 " %016" PRIx64 " %s\n",
      i, Name.str().c_str(), Size, Address, Type.c_str());
    ++i;
  }
}

bool symbolSorter(FractureSymbol *symOne, FractureSymbol *symTwo) {
  uint64_t addrOne = 0, addrTwo = 0;
  symOne->getAddress(addrOne);
  symTwo->getAddress(addrTwo);
  return addrOne < addrTwo;
}

template <class ELFT>
static void dumpELFRelocSymbols(const object::ELFObjectFile<ELFT>* elf,
  unsigned Address) {
  std::error_code ec;

  // Grab symbols only for the current section so that relocation entries
  // actually print in proper sections, i.e .rel.dyn and .rel.plt
  object::SectionRef CurrentSection = DAS->getSectionByAddress(Address);
  for (object::relocation_iterator ri = CurrentSection.relocation_begin(); ri !=
      CurrentSection.relocation_end(); ++ri) {
    if (error(ec))
      return;
    uint64_t Addr = 0;
    uint64_t SectAddr = 0;
    uint64_t Offset = 0;
    uint64_t Type = 0;
    SmallVector<char, 25> TypeName;
    SmallVector<char, 25> Value;
    StringRef Name;
    StringRef SectionName;

    ri->getSymbol()->getName(Name);
    if (error(ri->getAddress(Addr)))
      continue;
    //if (error(ri->getOffset(Offset))) // FIXME: Maybe getOffset only 
    //  continue;                       // works for a certain reloc type?
    if (error(ri->getType(Type)))
      continue;
    if (error(ri->getTypeName(TypeName)))
      continue;
    if (error(ri->getValueString(Value)))
      continue;

    object::SectionRef Section = DAS->getSectionByAddress(Addr);
    SectAddr = Section.getAddress();

    const char *Fmt;
    Fmt = elf->getBytesInAddress() > 4 ? "%016" PRIx64 :
      "%08" PRIx64;
    SmallVectorImpl<char>::iterator ti= TypeName.begin();
    outs() << format(Fmt, Addr) << " "
           << format(Fmt, Offset) << " " // FIXME: Should be Info section
           << ti << "\t"
           << format(Fmt, Offset) << " " // FIXME: Should be Symbol Value
           << Name << "\n";
  }
}

template <class ELFT>
static void dumpELFSymbols(const object::ELFObjectFile<ELFT>* elf,
  unsigned Address) {
  std::error_code ec;
  std::vector<FractureSymbol *> Syms;
  std::map<StringRef, uint64_t> RelocOrigins = DAS->getRelocOrigins();

  for (object::symbol_iterator si = elf->symbols().begin(), se =
         elf->symbols().end(); si != se; ++si) {
    Syms.push_back(new FractureSymbol(*si));
  }

  for (object::symbol_iterator si = elf->dynamic_symbol_begin(), se =
       elf->dynamic_symbol_end(); si != se; ++si) {
    FractureSymbol *temp = new FractureSymbol(*si);
    temp->matchAddress(RelocOrigins);
    Syms.push_back(temp);
  }
  if (isStripped)
    for (auto &it : SDAS->getStrippedGraph()->getHeadNodes()) {
      StringRef name = (SDAS->getMain() == it->Address ?
                                "main" : DAS->getFunctionName(it->Address));
      FractureSymbol tempSym(it->Address, name,
                             0, object::SymbolRef::Type::ST_Function, 0);
      Syms.push_back(new FractureSymbol(tempSym));
    }

  // Sort symbols by address
  sort(Syms.begin(), Syms.end(), symbolSorter);

  for (std::vector<FractureSymbol *>::iterator si = Syms.begin(),
         se = Syms.end();
       si != se; ++si) {
    if (error(ec)) {
      for(auto &it : Syms)
        delete it;
      return;
    }
    StringRef Name;
    StringRef SectionName;
    object::SectionRef Section;
    uint64_t Addr = 0;
    object::SymbolRef::Type Type;
    uint64_t Size;
    uint32_t Flags = 0;
    uint64_t SectAddr;
    uint32_t Value;
    if (error((*si)->getName(Name)))
      continue;
    if(Name == "$d" || Name == "$a" || Name == "$t")
      continue;
    if (error((*si)->getAddress(Addr)))
      continue;
    if (error((*si)->getAlignment(Value))) // NOTE: This used to be getValue...
      continue;
    Section = DAS->getSectionByAddress(Addr);
    SectAddr = Section.getAddress();
    if (error((*si)->getType(Type)))
      continue;
    if (error((*si)->getSize(Size)))
      continue;

    // Doesn't print symbol information for symbols which aren't in the section
    // specified by the function parameter
    if (SectAddr != Address)
      continue;
    Section.getName(SectionName);
    if (Name == SectionName)
      continue;

    bool Global = Flags & object::SymbolRef::SF_Global;
    bool Weak = Flags & object::SymbolRef::SF_Weak;

    if (Addr == object::UnknownAddressOrSize)
      Addr = 0;
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

    outs() << format(Fmt, Addr) << " "
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
  dumpELFRelocSymbols(elf, Address);

  for(auto &it : Syms)
    delete it;
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
  std::error_code ec;
  for (object::section_iterator si = coff->section_begin(), se =
         coff->section_end(); si != se; ++si, ++Index) {
    uint64_t SectionAddr = si->getAddress();
    uint64_t SectionSize = si->getSize();
    if (SectionAddr <= Address && Address < SectionAddr + SectionSize) {
      SectionIndex = Index;
      break;
    }
  }
  if (SectionIndex == -1) {
    outs() << "No section found with that name or containing that address\n";
    return;
  }
/*
  for (object::import_directory_iterator idi = coff->import_directory_begin();
       idi != coff->import_directory_end(); ++idi) {
    for (object::imported_symbol_iterator isi = idi->imported_symbol_begin();
         isi != idi->imported_symbol_end(); ++isi) {
      StringRef symName;
      uint16_t ordinal;
      isi->getSymbolName(symName);
      isi->getOrdinal(ordinal);
      outs() << "Ordinal: " << ordinal << "\t";
      outs() << "SYMNAME: " << symName << "\n";
    }
  }
  outs() << "\n\nDelay import directories \n\n";
  for (object::delay_import_directory_iterator didi = coff->delay_import_directory_begin();
       didi != coff->delay_import_directory_end(); ++didi) {
    for (object::imported_symbol_iterator isi = didi->imported_symbol_begin();
         isi != didi->imported_symbol_end(); ++isi) {
      StringRef symName;
      uint16_t ordinal;
      isi->getSymbolName(symName);
      isi->getOrdinal(ordinal);
      outs() << "Ordinal: " << ordinal << "\t";
      outs() << "SYMNAME: " << symName << "\n";
    }
  }
  outs() << "\n\nEXPORT DIRECTORY STUFF\n\n"; 
  for (object::export_directory_iterator edi = coff->export_directory_begin();
       edi != coff->export_directory_end(); ++edi) {
    StringRef dllName, symName;
    uint32_t ordinalBase, ordinal, exportRVA;
    edi->getDllName(dllName);
    edi->getOrdinalBase(ordinalBase);
    edi->getOrdinal(ordinal);
    edi->getExportRVA(exportRVA);
    edi->getSymbolName(symName);
    outs() << "DLLNAME: " << dllName << "\n"
           << "ORDBASE: " << ordinalBase << "\n"
           << "ORDINAL: " << ordinal << "\n"
           << "EXPTRVA: " << exportRVA << "\n"
           << "SYMNAME: " << symName << "\n\n";
  }

  outs() << "\n\n BASE RELOCS \n\n";
  for (object::base_reloc_iterator bri = coff->base_reloc_begin();
       bri != coff->base_reloc_end(); ++bri) {
    uint8_t type;
    uint32_t RVA;
    bri->getType(type);
    bri->getRVA(RVA);
    auto coffSymbol = coff->getSymbol(RVA);
    if (coffSymbol.getError())
      outs() << "ERROR!\n";
    outs() << "TYP: " << type << "\n"
           << "RVA: " << format("%08x", RVA) << "\n";
  }

  outs() << "SYMSIZE: " << coff->getSymbolTableEntrySize() << "\n";
  outs() << "NUMSYMS: " << coff->getNumberOfSymbols() << "\n";
  outs() << coff->getSymbolTable();
  for (object::basic_symbol_iterator bsi = coff->symbol_begin_impl();
       bsi != coff->symbol_end_impl(); ++bsi) {
    bsi->printName(outs());
    outs() << "\n";
  }

  for (object::section_iterator si = coff->section_begin();
       si != coff->section_end(); ++si) {
    const object::coff_section *coffsec = coff->getCOFFSection(*si);
      for (object::relocation_iterator ri = si->relocation_begin();
         ri != si->relocation_end(); ++ri) {
      uint64_t add;
      ri->getAddress(add);
      outs() << "ADDR" << add << "\n";
      outs() << "BLAH";
    }
    outs() << "NUMRELOCS" << coffsec->NumberOfRelocations << "\n";
    outs() << "NAME: " << coffsec->Name << "\n";
    outs() << "POINTER: " << coffsec->PointerToRelocations << "\n";
  }


  int aux_count = 0;
  for (int i = 0, e = coff->getNumberOfSymbols(); i != e; ++i) {
    ErrorOr<object::COFFSymbolRef> symbol = coff->getSymbol(i);
    if (aux_count--) {
      // Figure out which type of aux this is.
      if (symbol->getStorageClass() == COFF::IMAGE_SYM_CLASS_STATIC
        && symbol->getValue() == 0) { // Section definition.
        const object::coff_aux_section_definition *asd;
        if (error(coff->getAuxSymbol<object::coff_aux_section_definition>(i,
              asd)))
          return;
        outs() << "AUX "
               << format("scnlen 0x%x nreloc %d nlnno %d checksum 0x%x ",
                 unsigned(asd->Length), unsigned(asd->NumberOfRelocations),
                 unsigned(asd->NumberOfLinenumbers), unsigned(asd->CheckSum))
               << format("assoc %d comdat %d\n",
                 unsigned(asd->getNumber(false)),
                 unsigned(asd->Selection));
      } else
        outs() << "AUX Unknown\n";
    } else {
      StringRef name;
      if (error(coff->getSymbolName(symbol.get(), name)))
        return;
      if ((int) symbol->getSectionNumber() != SectionIndex) {
        aux_count = symbol->getNumberOfAuxSymbols();
        continue;
      }

      outs() << "[" << format("%2d", i) << "]" << "(sec "
             << format("%2d", int(symbol->getSectionNumber()))
             << ")" << "(fl 0x00)"
             // Flag bits, which COFF doesn't have.
             << "(ty " << format("%3x", unsigned(symbol->getType())) << ")"
             << "(scl "
             << format("%3x", unsigned(symbol->getStorageClass())) << ") "
             << "(nx "
             << unsigned(symbol->getNumberOfAuxSymbols()) << ") " << "0x"
             << format("%08x", unsigned(symbol->getValue()))
             << " " << name << "\n";
      aux_count = symbol->getNumberOfAuxSymbols();
    }
  }
*/
}

static void runSymbolsCommand(std::vector<std::string> &CommandLine) {
  if (CommandLine.size() < 2) {
    outs() << "Did not understand section name or address.\n";
    return;
  }

  StringRef SectionNameOrAddress = CommandLine[1];
  const object::ObjectFile* Executable = DAS->getExecutable();

  std::error_code ec;
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

  Address = Section.getAddress();

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

  std::error_code ErrorInfo;
  raw_fd_ostream FOut(CommandLine[1], ErrorInfo,
    sys::fs::OpenFlags::F_RW);

  FOut << *(DEC->getModule());

  if (ErrorInfo) {
    outs() << "Errors on write: \n" << ErrorInfo.message() << "\n";
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
  BaseAddr = Section.getAddress();
  BSS = Section.isBSS();

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

  if (std::error_code Err = loadBinary(InputFileName.getValue())) {
    errs() << ProgramName << ": Could not open the file '"
        << InputFileName.getValue() << "'. " << Err.message() << ".\n";
  }
//If the -stripped flag is set and the file is actually stripped.
  if(DAS->getExecutable()->symbol_begin() == DAS->getExecutable()->symbol_end()
     && StrippedBinary){
    isStripped = true;
    outs() << "File is Stripped\n";
    SDAS = new StrippedDisassembler(DAS, TripleName);
    SDAS->findStrippedMain();
    SDAS->functionsIterator(SDAS->getStrippedSection(".text"));
    //Also print stripped graph
    if(printGraph)
      SDAS->getStrippedGraph()->printGraph();
    SDAS->getStrippedGraph()->correctHeadNodes();
  }



  CommandParser.runShell(ProgramName);
  delete SDAS;
  return 0;
}
