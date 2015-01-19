//===--- StructuredModuleEditor.h - Disassembler ----------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief Wrapper for a CFG editor. Implements functionality for
/// CFG modifications (e.g. adding, removing, and cloning nodes)
///
/// Author: lqb4061
/// Date: Aug 20, 2013
///
//===----------------------------------------------------------------------===//

#ifndef StructuredModuleEditor_H_
#define StructuredModuleEditor_H_

#include "llvm/Linker/Linker.h"
#include "llvm-c/Linker.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "BasicCallGraph.h"

using namespace llvm;

/// \brief An abstraction for a CFG editor. This class interacts with .ll files
/// by parsing them into a CFG and performs a variety of functions related to the
/// modification of said CFG.
class StructuredModuleEditor {

private:
  typedef std::vector<Function*> FuncList;
  typedef std::vector<BasicBlock*> BBList;
  typedef std::vector<Instruction*> InstList;
  typedef std::vector<GlobalVariable*> GlobalList;
  typedef std::vector<Value*> ValueList;
  typedef std::vector<std::string> StringList;

  Module *M;
  BasicCallGraph *CG;
  raw_ostream &OS;

  bool getCallSite(Function *Func, uint64_t Index, CallSite &CS);

  bool signaturesMatch(Function *F1, Function *F2);

  /// \brief Prints the contents of a vector of Instructions to this
  /// StructuredModuleEditor's output stream
  ///
  /// Typical usage:
  /// \code
  ///   dumpInsts(Instructions);
  /// \endcode
  ///
  /// \param Insts - A reference to a vector of Instructions
  ///
  void dumpInsts(InstList &Insts);

  /// \brief Deletes all the instructions pointed to by the specified InstList
  /// and then clears the InstList
  ///
  /// Typical usage:
  /// \code
  ///   removeChain(InstChain);
  /// \endcode
  ///
  /// \param Chain - A reference to a vector of pointers to Instructions
  ///
  void removeChain(InstList &Chain);

  /// \brief Gathers a list of all call instructions dependending on F's existence
  /// and returns the list
  ///
  /// Typical usage:
  /// \code
  ///   getCallsToFunction(SomeFunc);
  /// \endcode
  ///
  /// \param V - A pointer to a Value
  ///
  /// \returns Returns an InstList containing all call instructions dependending
  /// on F's existence
  InstList getCallsToFunction(Function *F);

public:
  StructuredModuleEditor(const std::string &Filename, raw_ostream &OS);
  StructuredModuleEditor(Module *M, raw_ostream &OS);
  virtual
  ~StructuredModuleEditor();

  FuncList* getFuncsWithSameSignature(StringRef FuncName);
  FuncList* getFuncsWithSameSignature(Function *Func);

  void dumpFuncsWithSameSignature(StringRef FuncName);
  void dumpFuncsWithSameSignature(Function *Func);

  ValueList getUseChain(Value *V);

  void instrumentFunctionsThatCallFunction(StringRef FuncName);
  void instrumentFunctionsThatCallFunction(Function *Callee);

  void instrumentCallsToFunction(StringRef FuncName);
  void instrumentCallsToFunction(Function *Callee);

  // TODO: Comment me
  void linkModule(const std::string &Filename);

  /// \brief Links the given module to this StructuredModuleEditor's module.
  ///
  /// Typical usage:
  /// \code
  ///   linkModule(Mod);
  /// \endcode
  ///
  /// \param Mod - A Module to link to this StructuredModuleEditor's module.
  /// Mod should not contain any of the same global symbols as this editor's
  /// module or else the linkage will fail because global symbols cannot be
  /// multiply defined.
  ///
  void linkModule(Module *Mod);

  /// \brief Parses the file with the given filename into
  ///  a Module and returns the Module.
  ///  The file must be a structurally-valid LLVM IR file.
  Module* getModule(const std::string &Filename);

  /// \brief Prints the IR for for this StructuredModuleEditor's Module
  ///
  /// Typical usage:
  /// \code
  ///   printIR();
  /// \endcode
  ///
  void printIR();

  /// \brief Prints the CFG info for for this StructuredModuleEditor's CFG
  ///
  /// Typical usage:
  /// \code
  ///   printCFG();
  /// \en
  void dumpCallGraphText();

  /// \brief Spawns popup windows in Dotty for each function in this
  /// StructuredModuleEditor's CFG
  ///
  /// Typical usage:
  /// \code
  ///   viewCFG();
  /// \en
  void showCfgInDotty();

  /// \brief Given the name of a function, an index, and the name of
  /// another function, sets the called function of the "index"th callsite within the
  /// first function to the second function.
  ///
  /// Typical usage:
  /// \code
  ///   replaceEdge("OldFunc", 3, "NewFunc");
  /// \endcode
  ///
  /// \param CallerName - The name of a Function whose call should be
  /// replaced
  /// \param CallSiteIndex - The index within the calling function's list of
  /// callsites whose called function should be replaced
  /// \param DestinationName - The name of a Function to which the call should
  /// be redirected
  ///
  /// \returns Returns false if replacing the old call destination with
  /// the new call destination would result in a structurally invalid
  /// Module; else returns true.
  bool replaceEdge(StringRef CallerName, uint64_t CallSiteIndex,
      StringRef DestinationName);

  /// \brief Given a function, an index, and another function,
  /// sets the called function of the "index"th callsite within the
  /// first function to the second function.
  ///
  /// Typical usage:
  /// \code
  ///   replaceEdge(CFG->getFunctionByName("OldFunc"), 3,
  ///     CFG->getFunctionByName("NewFunc"));
  /// \endcode
  ///
  /// \param Caller - The function whose out-edge is being redirected
  /// \param CallSiteIndex - The index within the calling function's list of
  /// callsites whose called function should be replaced
  /// \param Destination - The function to which the call should
  /// be redirected
  ///
  /// \returns Returns false if replacing the old call destination with
  /// the new call destination would result in a structurally invalid
  /// Module; else returns true.
  bool replaceEdge(Function *Caller, uint64_t CallSiteIndex,
      Function *Destination);

  /// \brief Replaces all calls to the function with the name OldFuncName to
  /// the function with the name NewFuncName
  ///
  /// Typical usage:
  /// \code
  ///   redirectFunctionCalls("OldFunc", "NewFunc");
  /// \endcode
  ///
  /// \param OldFuncName - The name of a Function whose callers should be
  /// redirected
  /// \param NewFuncName - The name of a Function to which the calls to the function
  /// with the name OldFuncName should be redirected
  ///
  /// \returns Returns false if replacing the old call destination's references with
  /// the new call destination's references would result in a structurally invalid
  /// Module; else returns true.
  bool replaceFunc(StringRef OldFuncName, StringRef NewFuncName);

  /// \brief Redirects all calls to OldFunc to NewFunc
  ///
  /// Typical usage:
  /// \code
  ///   redirectFunctionCalls(&OldFunc, &NewFunc);
  /// \endcode
  ///
  /// \param OldFunc - The Function whose callers should be redirected
  /// \param NewFunc - The Function to which the calls to OldFunc
  /// should be redirected
  ///
  /// \returns Returns false if replacing OldFunc references with NewFunc
  /// would result in a structurally invalid Module; else returns true.
  bool replaceFunc(Function *OldFunc, Function *NewFunc);

  /// \brief Creates a wrapper function which calls a function with the name specified by PreFunc
  /// (if it is not null), a function with the name specified by OriginalFunc, and  a function
  /// with the name specified by PostFunc (if it is not null), in that order. This new function
  /// takes the name of the wrapped function and the wrapped function is assigned
  /// some name by LLVM.
  ///
  /// Typical usage:
  /// \code
  ///   wrapFunc("fibRecursive", "prologue", "epilogue");
  /// \endcode
  ///
  /// \param OriginalFunc - The name of the function to wrap
  /// \param PreFunc - The name of the function the wrapper should invoke before invoking OriginalFunc.
  /// \param PostFunc - The name of the function the wrapper should invoke after invoking OriginalFunc.
  ///
  ///
  /// \returns Returns the wrapper function
  Function* wrapFunc(StringRef OriginalFunc, StringRef PreFunc,
      StringRef PostFunc);

  /// \brief Creates a wrapper function which calls PreFunc (if it is not null), OriginalFunc, and
  /// PostFunc (if it is not null), in that order. This new function takes the name of OriginalFunc
  /// and OriginalFunc is assigned some name by LLVM.
  ///
  /// Typical usage:
  /// \code
  ///   wrapFunc(CFG->getFunctionByName("fibRecursive"),
  ///              CFG->getFunctionByName("prologue"),
  ///              CFG->getFunctionByName("epilogue"));
  /// \endcode
  ///
  /// \param OriginalFunc - The function to wrap
  /// \param PreFunc - The function the wrapper should invoke before invoking OriginalFunc. Should
  /// be NULL if no call should be made before OriginalFunc is invoked.
  /// \param PostFunc - The function the wrapper should invoke after invoking OriginalFunc. Should
  /// be NULL if no call should be made after OriginalFunc is invoked.
  ///
  ///
  /// \returns Returns the wrapper function
  Function* wrapFunc(Function *OriginalFunc, Function *PreFunc,
      Function *PostFunc);

  /// \brief Removes the function with the given name from the CFG and the Module.
  ///
  /// Typical usage:
  /// \code
  ///   removeFunc("fibRecursive")
  /// \endcode
  ///
  /// \param FuncName - The name of a Function to remove from this StructuredModuleEditor's CFG
  /// and module
  ///
  /// \returns Returns false if no function exists in this StructuredModuleEditor's CFG with the given name;
  /// else returns true
  bool removeFunc(StringRef FuncName);

  /// \brief Removes the given function from the CFG and the Module. Any instructions
  /// which depend on the functions existence are also removed from the Module. As removing
  /// instructions may result in non-well-formed basic blocks, any basic blocks which are
  /// structurally invalid are removed. As removing basic blocks may result in non-well-formed
  /// functions, any functions which are structurally invalid are removed. These functions are
  /// removed via a recursive call to this function.
  ///
  /// Typical usage:
  /// \code
  ///   removeFunc(0x400698)
  /// \endcode
  ///
  /// \param FunctionToRemove - A pointer to a Function to remove from this StructuredModuleEditor's CFG
  /// and module
  ///
  ///
  /// \returns Returns false if the function does not exist within this StructuredModuleEditor's CFG;
  /// else returns true
  bool removeFunc(Function *FunctionToRemove);

  /// \brief Clones the function with the given name and inserts it into this StructuredModuleEditor's
  /// CFG and module. The clone's CFG node will have all the same out-edges as the original function's
  /// CFG node, but it will not have any in-edges because it is newly-created.
  ///
  /// Typical usage:
  /// \code
  ///   cloneFunc("fibRecursive")
  /// \endcode
  ///
  /// \param OriginalName - The name of a Function to clone into this StructuredModuleEditor's
  /// CFG and module
  ///
  ///
  /// \returns Returns a pointer to the clone function if the cloning was successful; otherwise returns
  /// NULL.
  Function* cloneFunc(StringRef OriginalName);

  /// \brief Clones the given function and inserts it into this StructuredModuleEditor's CFG and module.
  /// The clone's CFG node will have all the same out-edges as the original function's CFG node, but it
  /// will not have any in-edges because it is newly-created.
  ///
  /// Typical usage:
  /// \code
  ///   cloneFunc(0x400698)
  /// \endcode
  ///
  /// \param Original - A pointer to a Function to clone into this StructuredModuleEditor's CFG and module
  ///
  ///
  /// \returns Returns a pointer to the clone function if the cloning was successful; otherwise returns
  /// NULL.
  Function* cloneFunc(Function *Original);
};

#endif /* StructuredModuleEditor_H_ */
