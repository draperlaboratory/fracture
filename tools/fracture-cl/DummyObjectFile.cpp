//===--- DummyObjectFile.cpp - [Name] ----------------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Represents a raw binary file.
//
// Note: Could also use the ancestor class Binary for this feature, but we did
//       not do that because we want to add ObjectFile capabilities so that we
//       would have capabilities to add sections, symbols, etc.
//
// Author: rtc1032
// Date: Oct 11, 2012
//
//===----------------------------------------------------------------------===//

#include "DummyObjectFile.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

namespace object {

  DummyObjectFile::DummyObjectFile(std::unique_ptr<MemoryBuffer> &Object,
    std::error_code& ec) : ObjectFile(Binary::ID_ELF32B, std::move(Object)) {
    // NOTE: Figure out if using ID_ELF32B breaks anything.
    //       We want it to ID as an object, but we don't want it to try to
    //       disassemble as an ELF...We may have to change the LLVM base code.
    // this->Data.swap(Object);
    // this->TypeID = Binary::ID_ELF32B;
    ec = object_error::success;
  }

  // Ideally, the following should be in the objectfile namespace but
  // we did not want to change the base llvm.
  ObjectFile* DummyObjectFile::createDummyObjectFile(
    std::unique_ptr<MemoryBuffer> &Object) {
    std::error_code ec;
    return new DummyObjectFile(Object, ec);
  }

  symbol_iterator DummyObjectFile::begin_symbols() const {
    DataRefImpl ret;
    ret.p = intptr_t(0);
    return symbol_iterator(SymbolRef(ret, this));
  }

  symbol_iterator DummyObjectFile::end_symbols() const {
    return begin_symbols();
  }

  symbol_iterator DummyObjectFile::begin_dynamic_symbols() const {
    //TODO: Implement?
    report_fatal_error("Dynamic symbols unimplemented in DummyObjectFile");
  }

  symbol_iterator DummyObjectFile::end_dynamic_symbols() const {
    //TODO: Implement?
    report_fatal_error("Dynamic symbols unimplemented in DummyObjectFile");
  }

  section_iterator DummyObjectFile::begin_sections() const {
    DataRefImpl ret;
    ret.p = intptr_t(this->Data.get());
    return section_iterator(SectionRef(ret, this));
  }

  section_iterator DummyObjectFile::end_sections() const {
    //DataRefImpl ret;
    //ret.p = intptr_t(0);
    //return section_iterator(SectionRef(ret, this));
	 return begin_sections();
  }

  library_iterator DummyObjectFile::begin_libraries_needed() const {
    //TODO: Implement?
    report_fatal_error("Libraries needed unimplemented in DummyObjectFile");
  }

  library_iterator DummyObjectFile::end_libraries_needed() const {
    //TODO: Implement?
    report_fatal_error("Libraries needed unimplemented in DummyObjectFile");
  }

  uint8_t DummyObjectFile::getBytesInAddress() const {
    // TODO: Implement based on target?
    return 4;
  }

  StringRef DummyObjectFile::getFileFormatName() const {
    // TODO: Implement based on target?
    return "<unknown format>-<unknown-arch>";
  }

  unsigned DummyObjectFile::getArch() const {
    // TODO: Implement based on target?
    return Triple::UnknownArch;
  }

  StringRef DummyObjectFile::getLoadName() const {
    // TODO: Implement based on constructor/filename?
    return "";
  }

  std::error_code DummyObjectFile::getSymbolNext(DataRefImpl Symb,
                                            SymbolRef& Res) const {
    // TODO: Implement
    Res = SymbolRef(Symb, this);
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolName(DataRefImpl Symb,
                                            StringRef& Res) const {
    // TODO: Implement
    Res = StringRef("");
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolAddress(DataRefImpl Symb,
                                               uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolFileOffset(DataRefImpl Symb,
                                                  uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolSize(DataRefImpl Symb,
                                            uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolType(DataRefImpl Symb,
                                            SymbolRef::Type& Res) const {
    // TODO: Implement
    Res = SymbolRef::ST_Unknown;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolNMTypeChar(DataRefImpl Symb,
                                                  char& Res) const {
    // TODO: Implement
    Res = 'U'; // U = Undefined, see binutils documentation.
    return object_error::success;
  }

  uint32_t DummyObjectFile::getSymbolFlags(DataRefImpl Symb) const {
    // TODO: Implement
    return SymbolRef::SF_Undefined;
  }

  std::error_code DummyObjectFile::getSymbolSection(DataRefImpl Symb,
                                               section_iterator& Res) const {
    // TODO: Implement
    Res = begin_sections();
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSymbolValue(DataRefImpl Symb,
                                                     uint64_t &Val) const {
    Val = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionNext(DataRefImpl Sec,
                                             SectionRef& Res) const {
    DataRefImpl ret;
    ret.p = intptr_t(0);
    Res = SectionRef(ret, this);
    // TODO: Implement
    //Res = end_sections();
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionName(DataRefImpl Sec,
                                             StringRef& Res) const {
    MemoryBuffer *Buf = reinterpret_cast<MemoryBuffer*>(Sec.p);
    Res = Buf->getBufferIdentifier();
    //Res = StringRef("<unknown>");
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionAddress(DataRefImpl Sec,
                                                uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionSize(DataRefImpl Sec,
                                             uint64_t& Res) const {
    // TODO: we will need a custom section type if we want to add sections
    MemoryBuffer *Buf = reinterpret_cast<MemoryBuffer*>(Sec.p);
    Res = Buf->getBufferSize();
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionContents(DataRefImpl Sec,
                                                 StringRef& Res) const {
    MemoryBuffer *Buf = reinterpret_cast<MemoryBuffer*>(Sec.p);
    Res = Buf->getBuffer();
    //Res = StringRef("None");
    return object_error::success;
  }

  std::error_code DummyObjectFile::getSectionAlignment(DataRefImpl Sec,
                                                  uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionText(DataRefImpl Sec, bool& Res) const {
    // TODO: Implement
    Res = true;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionData(DataRefImpl Sec, bool& Res) const {
    // TODO: Implement
    Res = true;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionBSS(DataRefImpl Sec, bool& Res) const {
    // TODO: Implement
    Res = true;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionRequiredForExecution(DataRefImpl Sec,
                                                            bool& Res) const {
    // TODO: Implement
    Res = true;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionVirtual(DataRefImpl Sec,
                                               bool& Res) const {
    // TODO: Implement
    Res = false;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionZeroInit(DataRefImpl Sec,
                                                bool& Res) const {
    // TODO: Implement
    Res = false;
    return object_error::success;
  }

  std::error_code DummyObjectFile::isSectionReadOnlyData(DataRefImpl Sec,
                                                    bool& Res) const {
    // TODO: Implement
    Res = false;
    return object_error::success;
  }

  std::error_code DummyObjectFile::sectionContainsSymbol(DataRefImpl Sec,
                                                    DataRefImpl Symb,
                                                    bool& Result) const {
    // TODO: Implement
    Result = true;
    return object_error::success;
  }

  relocation_iterator DummyObjectFile::getSectionRelBegin(
                                                      DataRefImpl Sec) const {
    // TODO: Implement
    DataRefImpl Ret;
    Ret.p = 0;
    return relocation_iterator(RelocationRef(Ret, this));
  }

  relocation_iterator DummyObjectFile::getSectionRelEnd(DataRefImpl Sec) const {
    // TODO: Implement
    return getSectionRelBegin(Sec);
  }

  std::error_code DummyObjectFile::getRelocationNext(DataRefImpl Rel,
                                                RelocationRef& Res) const {
    // TODO: Implement
    DataRefImpl Ret;
    Ret.p = 0;
    Res = RelocationRef(Ret, this);
    return object_error::success;
  }

  std::error_code DummyObjectFile::getRelocationAddress(DataRefImpl Rel,
                                                   uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getRelocationOffset(DataRefImpl Rel,
                                                  uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  symbol_iterator DummyObjectFile::getRelocationSymbol(DataRefImpl Rel) const {
    // TODO: Implement
    return begin_symbols();
  }

  std::error_code DummyObjectFile::getRelocationType(DataRefImpl Rel,
                                                uint64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getRelocationTypeName(DataRefImpl Rel,
                                          SmallVectorImpl<char>& Result) const {
    // TODO: Implement
    StringRef Res("Unknown");
    Result.append(Res.begin(), Res.end());
    return object_error::success;
  }

  std::error_code DummyObjectFile::getRelocationAdditionalInfo(DataRefImpl Rel,
                                                          int64_t& Res) const {
    // TODO: Implement
    Res = 0;
    return object_error::success;
  }

  std::error_code DummyObjectFile::getRelocationValueString(DataRefImpl Rel,
                                          SmallVectorImpl<char>& Result) const {
    // TODO: Implement
    StringRef Res("Unknown");
    Result.append(Res.begin(), Res.end());
    return object_error::success;
  }

  std::error_code DummyObjectFile::getLibraryNext(DataRefImpl Lib,
                                             LibraryRef& Res) const {
    // TODO: Implement
    DataRefImpl Ret;
    Ret.p = 0;
    Res = LibraryRef(Ret, this);

    return object_error::success;
  }


  std::error_code DummyObjectFile::getLibraryPath(DataRefImpl Lib,
                                             StringRef& Res) const {
    // TODO: Implement
    Res = "";
    return object_error::success;
  }

} /* namespace object */
} /* namespace llvm */
