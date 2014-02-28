//===--- StructuredModuleEditor.cpp - StructuredModuleEditor ----------------*- C++ -*-===//
//
//                     Draper Disassembly Infrastructure
//
// This file is currently unlicensed and not for distribution.
//
//===----------------------------------------------------------------------===//
//
// Wrapper for a CFG editor. Implements functionality for
// CFG modifications (e.g. adding, removing, and cloning nodes)
//
// Author: lqb4061
// Date: Aug 20, 2013
//
//===----------------------------------------------------------------------===//

#include "Edit/StructuredModuleEditor.h"

using namespace llvm;

StructuredModuleEditor::StructuredModuleEditor(const std::string &Filename,
		raw_ostream &OS) :
		M(0), CG(0), OS(OS) {
	// Parses the specified file as a Module
	M = getModule(Filename);

	// Generates a call graph for the module
	CG = new BasicCallGraph(*M);
}

StructuredModuleEditor::StructuredModuleEditor(Module *M, raw_ostream &OS) :
		M(M), CG(0), OS(OS) {
	// Generates a call graph for the module
	CG = new BasicCallGraph(*M);
}

StructuredModuleEditor::~StructuredModuleEditor() {
  delete M;
}

StructuredModuleEditor::FuncList* StructuredModuleEditor::getFuncsWithSameSignature(
		StringRef FuncName) {
	Function *Func = M->getFunction(FuncName);
	return getFuncsWithSameSignature(Func);
}

StructuredModuleEditor::FuncList* StructuredModuleEditor::getFuncsWithSameSignature(
		Function *Func) {
	if (Func == NULL) {
		OS << "Function not found!\n";
		return NULL;
	}

	FuncList *MatchingFuncs = new FuncList;
	for (Module::iterator FI = M->begin(), FE = M->end(); FI != FE; ++FI) {
		if (signaturesMatch(Func, FI))
			MatchingFuncs->push_back(FI);
	}

	return MatchingFuncs;
}

void StructuredModuleEditor::dumpFuncsWithSameSignature(StringRef FuncName) {
	Function *Func = M->getFunction(FuncName);
	return dumpFuncsWithSameSignature(Func);
}

void StructuredModuleEditor::dumpFuncsWithSameSignature(Function *Func) {
	FuncList *Funcs = getFuncsWithSameSignature(Func);

	OS << Funcs->size() << " functions with the following signature:\n";
	OS << "*** RETURN TYPE: " << *(Func->getFunctionType()) << "\n";
	for (Function::arg_iterator AI = Func->arg_begin(), AE = Func->arg_end();
			AI != AE; ++AI) {
		OS << "*** ARG TYPE: " << *AI << "\n";
	}
	OS << "\n";

	for (FuncList::iterator FI = Funcs->begin(), FE = Funcs->end(); FI != FE;
			++FI) {
		OS << (*FI)->getName() << "\n";
	}
}

Module* StructuredModuleEditor::getModule(const std::string &Filename) {
// Parses a .ll file into a module
	LLVMContext &Context = getGlobalContext();
	SMDiagnostic Err;
	Module *Mod = ParseIRFile(Filename, Err, Context);

// If we did not produce a valid module,
// the editor is useless.
	assert(Mod && "Failed to parse IR file!");

	return Mod;
}

void StructuredModuleEditor::linkModule(const std::string &Filename) {
	Module *Mod = getModule(Filename);
	linkModule(Mod);
}

void StructuredModuleEditor::linkModule(Module *LinkMe) {
	std::string ErrorMsg;
	if (Linker::LinkModules(M, LinkMe, Linker::PreserveSource, &ErrorMsg)) {
		OS << ErrorMsg << "\n";
		return;
	}

	for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
		CG->getOrInsertFunction(I);
}

void StructuredModuleEditor::instrumentFunctionsThatCallFunction(
		StringRef FuncName) {
	Function *Func = M->getFunction(FuncName);
	instrumentFunctionsThatCallFunction(Func);
}

void StructuredModuleEditor::instrumentFunctionsThatCallFunction(
		Function *Callee) {
	if (Callee == NULL) {
		OS << "Function not found!\n";
		return;
	}

	InstList Calls = getCallsToFunction(Callee);

	FuncList Callers;
	for (InstList::iterator II = Calls.begin(), IE = Calls.end(); II != IE;
			++II) {
		Function *Caller = (*II)->getParent()->getParent();
		if (std::find(Callers.begin(), Callers.end(), Caller) == Callers.end())
			Callers.push_back(Caller);
	}

	OS << Callers.size() << " functions call '" << Callee->getName()
			<< "'...\n";
	OS << "=================================\n";
	for (FuncList::iterator FI = Callers.begin(), FE = Callers.end(); FI != FE;
			++FI) {
		OS << (*FI)->getName() << "\n";
	}
	OS << "=================================\n";

	for (FuncList::iterator FI = Callers.begin(), FE = Callers.end(); FI != FE;
			++FI) {
		std::vector<Value*> PreArgs;
		std::vector<Type*> PreArgTypes;

		Function *Caller = *FI;

		for (Function::arg_iterator AI = Caller->arg_begin(), AE =
				Caller->arg_end(); AI != AE; ++AI) {
			PreArgTypes.push_back(AI->getType());
			PreArgs.push_back(AI);
		}

		std::vector<Type*> PostArgTypes;
		if (!Caller->getReturnType()->isVoidTy()) {
			PostArgTypes.push_back(Caller->getReturnType());
		}

		Constant *PreConst = M->getOrInsertFunction("",
				FunctionType::get(Type::getVoidTy(getGlobalContext()),
						PreArgTypes, false));
		Function *Pre = cast<Function>(PreConst);
		Pre->setName("pre");
		CG->getOrInsertFunction(Pre);

		Constant *PostConst = M->getOrInsertFunction("",
				FunctionType::get(Type::getVoidTy(getGlobalContext()),
						PostArgTypes, false));
		Function *Post = cast<Function>(PostConst);
		Post->setName("post");
		CG->getOrInsertFunction(Post);

		Function *Wrapper = wrapFunc(Caller, Pre, Post);
		/*
		 OS << "\n";
		 OS << "Function '" << Caller->getName() << "'  returns "
		 << *(Caller->getReturnType()) << "\n";
		 int ArgCount = 0;
		 for (Function::arg_iterator AI = Caller->arg_begin(), AE =
		 Caller->arg_end(); AI != AE; ++AI) {
		 OS << "Arg #" << ArgCount++ << ": " << *AI << "\n";
		 }
		 OS << "\n";
		 OS << "Wrapping '" << Caller->getName() << "' with '" << Wrapper->getName()
		 << "'...\n\n";
		 OS << "Pre-invocation function = " << Pre->getName() << "\n";
		 OS << *Pre << "\n";
		 OS << "Post-invocation function = " << Post->getName() << "\n";
		 OS << *Post;
		 OS << "**************************************\n";*/
	}

	OS << "Functions successfully wrapped!\n";
}

void StructuredModuleEditor::instrumentCallsToFunction(StringRef FuncName) {
	Function *Func = M->getFunction(FuncName);
	instrumentCallsToFunction(Func);
}

void StructuredModuleEditor::instrumentCallsToFunction(Function *Callee) {
	if (Callee == NULL) {
		OS << "Function not found!\n";
		return;
	}

	InstList Calls = getCallsToFunction(Callee);

	FuncList Callers;
	for (InstList::iterator II = Calls.begin(), IE = Calls.end(); II != IE;
			++II) {
		Function *Caller = (*II)->getParent()->getParent();
		if (std::find(Callers.begin(), Callers.end(), Caller) == Callers.end())
			Callers.push_back(Caller);
	}

	OS << Callers.size() << " functions call '" << Callee->getName()
			<< "'...\n";
	OS << "=================================\n";
	for (FuncList::iterator FI = Callers.begin(), FE = Callers.end(); FI != FE;
			++FI) {
		OS << (*FI)->getName() << "\n";
	}
	OS << "=================================\n";

	std::vector<Value*> PreArgs;
	std::vector<Type*> PreArgTypes;
	for (Function::arg_iterator I = Callee->arg_begin(), E = Callee->arg_end();
			I != E; ++I) {
		PreArgTypes.push_back(I->getType());
		PreArgs.push_back(I);
	}

	std::vector<Type*> PostArgTypes;
	if (!Callee->getReturnType()->isVoidTy()) {
		PostArgTypes.push_back(Callee->getReturnType());
	}

	FuncList Clones;

	Clones.push_back(Callee);
	for (uint64_t i = 0; i < Callers.size() - 1; i++) {
		Function *Clone = cloneFunc(Callee);
		Clones.push_back(Clone);
	}

	for (uint64_t i = 0; i < Clones.size(); i++) {
		Constant *PreConst = M->getOrInsertFunction("",
				FunctionType::get(Type::getVoidTy(getGlobalContext()),
						PreArgTypes, false));
		Function *Pre = cast<Function>(PreConst);
		Pre->setName("pre");
		CG->getOrInsertFunction(Pre);

		Constant *PostConst = M->getOrInsertFunction("",
				FunctionType::get(Type::getVoidTy(getGlobalContext()),
						PostArgTypes, false));
		Function *Post = cast<Function>(PostConst);
		Post->setName("post");
		CG->getOrInsertFunction(Post);

		/*
		 OS << "\n";
		 OS << "Wrapping '" << Clones.at(i)->getName() << "'...\n\n";
		 OS << "Pre-invocation function = " << Pre->getName() << "\n";
		 OS << *Pre;
		 OS << "Post-invocation function = " << Post->getName() << "\n";
		 OS << *Post;
		 OS << "**************************************\n";*/

		Function *Wrapper = wrapFunc(Clones.at(i), Pre, Post);
		if (i == 0)
			Callee = Wrapper;

		Function *Caller = Callers.at(i);
		for (Function::iterator BBI = Caller->begin(), BBE = Caller->end();
				BBI != BBE; ++BBI) {
			for (BasicBlock::iterator II = BBI->begin(), IE = BBI->end();
					II != IE; ++II) {
				CallSite CS(cast<Value>(II));
				// If this isn't a call, or it is a call to an intrinsic...
				if (!CS || isa<IntrinsicInst>(II))
					continue;

				if (Callee == CS.getCalledFunction()) {
					CS.setCalledFunction(Wrapper);

					// Creates an edge from the calling node to its new destination node
					CallGraphNode *CallingNode = (*CG)[CS.getCaller()];
					CallGraphNode *NewCalleeNode = (*CG)[Wrapper];
					CallingNode->replaceCallEdge(CS, CS, NewCalleeNode);
				}
			}
		}
	}

	OS << "Functions successfully wrapped!\n";
}

StructuredModuleEditor::ValueList StructuredModuleEditor::getUseChain(
		Value *V) {
	ValueList Vals;

	for (Value::use_iterator UI = V->use_begin(), UE = V->use_end(); UI != UE;
			++UI) {
		Value *ValueToPush;

		Instruction *Inst = dyn_cast<Instruction>(*UI);
		if (Inst && Inst->getOpcode() == Instruction::Store) {
			StoreInst *StInst = dyn_cast<StoreInst>(Inst);
			Value *Storee = StInst->getPointerOperand();
			ValueToPush = Storee;
		} else
			ValueToPush = *UI;

		Vals.push_back(ValueToPush);
	}

	return Vals;
}

Function* StructuredModuleEditor::wrapFunc(StringRef OriginalFunc,
		StringRef PreFunc, StringRef PostFunc) {
	Function *Original = M->getFunction(OriginalFunc);
	Function *Pre = M->getFunction(PreFunc);
	Function *Post = M->getFunction(PostFunc);

	return wrapFunc(Original, Pre, Post);
}

Function* StructuredModuleEditor::wrapFunc(Function *OriginalFunc,
		Function *PreFunc, Function *PostFunc) {
	if (OriginalFunc == NULL)
		return NULL;

	if (PreFunc != NULL) {
		for (Function::arg_iterator I = OriginalFunc->arg_begin(), J =
				PreFunc->arg_begin(), E = OriginalFunc->arg_end(); I != E;
				++I, ++J)
			if (I->getType() != J->getType()) {
				OS << PreFunc->getName()
						<< " must have the same argument types as the wrappee!\n";
				return NULL;
			}
	}

	if (PostFunc != NULL) {
		if (OriginalFunc->getReturnType()->isVoidTy()) {
			if (PostFunc->getArgumentList().size() > 0) {
				OS << PostFunc->getName()
						<< " must accept no arguments because the wrappee returns void!\n";
				return NULL;
			}
		} else if (PostFunc->getArgumentList().size() != 1
				|| PostFunc->getArgumentList().front().getType()
						!= OriginalFunc->getReturnType()) {
			OS << *(PostFunc->getType()) << "..."
					<< *(OriginalFunc->getReturnType()) << "\n";
			OS << PostFunc->getName()
					<< " must accept only one argument and that argument must be of the wrappee's return value type!\n";
			return NULL;
		}
	}

// The wrapper copies the given function's arguments and argument types to
// two separate vectors
	std::vector<Value*> WrapperArgs;
	std::vector<Type*> WrapperArgTypes;
	for (Function::arg_iterator I = OriginalFunc->arg_begin(), E =
			OriginalFunc->arg_end(); I != E; ++I) {
		WrapperArgTypes.push_back(I->getType());
		WrapperArgs.push_back(I);
	}

// Creates a function which is identical to the original function except for its name
// (will never "get" an existing function since the name is unique) and
// inserts it into the Module. The name is guaranteed to be unique because when we
// specify a Value's name as "", LLVM generates a unique identifier for it. If we set
// the name later on and the name is a duplicate, LLVM will also generate a unique ID.
// It is just important to avoid specifying a duplicate name during the "getOrInsert" portion
// of our code because we run the risk of getting something which exists instead of
// creating something new.
	Constant* c = M->getOrInsertFunction("",
			FunctionType::get(OriginalFunc->getReturnType(), WrapperArgTypes,
					false), OriginalFunc->getAttributes());
	Function *Wrapper = cast<Function>(c);

	Wrapper->setName(OriginalFunc->getName() + "-wrapper");

// The Wrapper function uses the same calling convention as the wrappee.
	Wrapper->setCallingConv(OriginalFunc->getCallingConv());

// The Wrapper function uses the same parameter names as the wrappee
	for (Function::arg_iterator I = Wrapper->arg_begin(), J =
			OriginalFunc->arg_begin(), E = Wrapper->arg_end(); I != E; ++I, ++J)
		I->setName(J->getName());

// Inserts the Wrapper function into the CFG
	CG->getOrInsertFunction(Wrapper);

// Replaces all references to OriginalFunc with references to Wrapper
	replaceFunc(OriginalFunc, Wrapper);

// Constructs a basic block in the following sequence:
// 1) If a pre-function-invocation function is given, creates a call to that function
//    with the same arguments passed to the wrapped function
// 2) Unconditionally creates a call to the function we are wrapping
//    with the same arguments passed to the wrapped function
// 3) If a post-function-invocation function is given, creates a call to that function
//    with the same return value of the wrapped function
// 4) Returns the v
	BasicBlock *EntryBlock = BasicBlock::Create(getGlobalContext(), "entry",
			Wrapper);
	IRBuilder<> builder(EntryBlock);

	if (PreFunc != NULL) {
		CallInst *PrologueCall = builder.CreateCall(PreFunc, WrapperArgs);
		CallSite CS(PrologueCall);
		(*CG)[Wrapper]->addCalledFunction(CS, (*CG)[PreFunc]);
	}

	CallInst *OriginalCall = builder.CreateCall(OriginalFunc, WrapperArgs);
	CallSite CS(OriginalCall);
	(*CG)[Wrapper]->addCalledFunction(CS, (*CG)[OriginalFunc]);

	if (PostFunc != NULL) {
		CallInst *EpilogueCall;
		if (OriginalCall->getType()->isVoidTy())
			EpilogueCall = builder.CreateCall(PostFunc);
		else
			EpilogueCall = builder.CreateCall(PostFunc, OriginalCall);

		CallSite CS(EpilogueCall);
		(*CG)[Wrapper]->addCalledFunction(CS, (*CG)[PostFunc]);
	}

	if (OriginalCall->getType()->isVoidTy())
		builder.CreateRetVoid();
	else
		builder.CreateRet(OriginalCall);

// Returns the Wrapper function we have created
	return Wrapper;
}

bool StructuredModuleEditor::replaceEdge(StringRef CallerName,
		uint64_t CallSiteIndex, StringRef DestinationName) {
	Function *CallingFunc = M->getFunction(CallerName);
	Function *DestinationFunc = M->getFunction(DestinationName);
	return replaceEdge(CallingFunc, CallSiteIndex, DestinationFunc);
}

bool StructuredModuleEditor::replaceEdge(Function *Caller,
		uint64_t CallSiteIndex, Function *Destination) {
// If either the caller or the callee is not present in the CFG,
// we cannot replace the edge
	if (Caller == NULL || Destination == NULL)
		return false;

// If we cannot locate the specified callsite to redirect,
// we cannot replace the edge
	CallSite CS;
	if (!getCallSite(Caller, CallSiteIndex, CS))
		return false;

	Function *OldDestination = CS.getCalledFunction();

// If the old callee and the new callee do not have the same
// signature, we cannot replace the edge
	if (!signaturesMatch(OldDestination, Destination))
		return false;

// Sets the callsite's callee to the specified callee
	CS.setCalledFunction(Destination);
	CallGraphNode *DestinationNode = CG->getOrInsertFunction(Destination);
	CG->getOrInsertFunction(Caller)->replaceCallEdge(CS, CS, DestinationNode);
	return true;
}

bool StructuredModuleEditor::replaceFunc(StringRef OldFuncName,
		StringRef NewFuncName) {
	Function *CurrentDestination = M->getFunction(OldFuncName);
	Function *NewDestination = M->getFunction(NewFuncName);

	return replaceFunc(CurrentDestination, NewDestination);
}

bool StructuredModuleEditor::replaceFunc(Function *OldFunc, Function *NewFunc) {

	if (OldFunc == NULL || NewFunc == NULL)
		return false;

	if (!signaturesMatch(OldFunc, NewFunc)) {
		OS << "Cannot replace '" << OldFunc->getName() << "' with '"
				<< NewFunc->getName()
				<< "' because they don't have identical signatures\n";
		return false;
	}

// Gathers all the calls to the function we want to bypass
	InstList Calls = getCallsToFunction(OldFunc);

// Iterates over each call to the function we want to bypass and sets the callee
// to the function we want to hook
	for (InstList::iterator I = Calls.begin(), E = Calls.end(); I != E; ++I) {
		CallSite CS(cast<Value>(*I));
		CS.setCalledFunction(NewFunc);

// Creates an edge from the calling node to its new destination node
		CallGraphNode *CallingNode = (*CG)[CS.getCaller()];
		CallGraphNode *NewCalleeNode = (*CG)[NewFunc];
		CallingNode->replaceCallEdge(CS, CS, NewCalleeNode);
	}

// Replace all remaining uses of OldFunc with NewFunc (e.g. pointers)
	OldFunc->replaceAllUsesWith(NewFunc);

	return true;
}

bool StructuredModuleEditor::removeFunc(StringRef FuncName) {
	Function *FunctionToRemove = M->getFunction(FuncName);
	return removeFunc(FunctionToRemove);
}

bool StructuredModuleEditor::removeFunc(Function *FunctionToRemove) {
// Checks to make sure the function we are trying to remove
// actually exists in the CFG
	if (FunctionToRemove == NULL) {
		OS << "Function does not exist in the call graph!\n";
		return false;
	}

	CallGraphNode *NodeToRemove = (*CG)[FunctionToRemove];

	// We cannot remove a node if it has any inteprocedural in-edges
	for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
		CallGraphNode *CallingNode = (*CG)[I];
		for (CallGraphNode::iterator CGNI = CallingNode->begin(), CGNE =
				CallingNode->end(); CGNI != CGNE; ++CGNI) {
			Function *Caller = I;
			Function *Callee = CGNI->second->getFunction();
			if (Callee == FunctionToRemove && Caller != Callee) {
				OS << "Cannot remove " << FunctionToRemove->getName()
						<< " because it has at least one interprocedural edge!\n";
				OS << "It is called by " << Caller->getName() << "\n";
				return false;
			}
		}
	}

// Removes all call graph edges from the node we are removing to its callees.
	NodeToRemove->removeAllCalledFunctions();
	CG->getExternalCallingNode()->removeAnyCallEdgeTo(NodeToRemove);

// Removes all call graph edges from callees to the node we are removing
	for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I) {
		CallGraphNode *CallingNode = (*CG)[I];
		CallingNode->removeAnyCallEdgeTo(NodeToRemove);
	}
	NodeToRemove->removeAnyCallEdgeTo(CG->getCallsExternalNode());

// Removes the function from the module and the CFG
	FunctionToRemove->dropAllReferences();

	// Remove the function from the module
	CG->removeFunctionFromModule(NodeToRemove);

	return true;
}

Function* StructuredModuleEditor::cloneFunc(StringRef OriginalName) {
	Function *FunctionToClone = M->getFunction(OriginalName);
	return cloneFunc(FunctionToClone);
}

Function * StructuredModuleEditor::cloneFunc(Function * Original) {
	if (Original == NULL)
		return NULL;

	ValueMap<const Value*, WeakVH> VMap;

// Creates a clone of the function we are cloning
	Function *Clone = CloneFunction(Original, VMap, false);
	Clone->setName(Original->getName() + "-cloned");

// Adds the clone to the Module
	M->getFunctionList().push_back(Clone);

// Adds the clone to the CFG
	CG->getOrInsertFunction(Clone);

// Adds each of the original function's CFG node's interprocedural out-edges
// to the clone's node. All of the original function's intraprocedural in-edges are redirected to the cloned function.
// The clone will have no interprocedural in-edges as it
// was just created.
	CallGraphNode *CloneNode = CG->getOrInsertFunction(Clone);
	for (Function::iterator BBI = Clone->begin(), BBE = Clone->end();
			BBI != BBE; ++BBI) {
		for (BasicBlock::iterator II = BBI->begin(), IE = BBI->end(); II != IE;
				++II) {
			CallSite CS(cast<Value>(II));
// If this isn't a call, or it is a call to an intrinsic...
			if (!CS || isa<IntrinsicInst>(II))
				continue;

			Function *Callee = CS.getCalledFunction();
			if (Callee == Original) {
				Callee = Clone;
				CS.setCalledFunction(Clone);
			}

			CloneNode->addCalledFunction(CS, CG->getOrInsertFunction(Callee));
		}
	}

	return Clone;
}

bool StructuredModuleEditor::getCallSite(Function *Func, uint64_t Index,
		CallSite &CS) {
	CallGraphNode *FuncNode = CG->getOrInsertFunction(Func);

	uint64_t Count = 0;
	for (CallGraphNode::const_iterator I = FuncNode->begin(), E =
			FuncNode->end(); I != E; ++I) {
		if (Count == Index) {
			CS = CallSite((*I).first);
			return true;
		}

		Count++;
	}

	return false;
}

bool StructuredModuleEditor::signaturesMatch(Function *First,
		Function *Second) {
	if (First == NULL || Second == NULL)
		return false;

	unsigned FirstNumArgs = First->arg_size();
	unsigned SecondNumArgs = Second->arg_size();

// The number of arguments passed to the old function must match the number of
// arguments passed to the new function
	if (FirstNumArgs != SecondNumArgs)
		return false;

// Both functions must abide by the same calling convention
	if (First->getCallingConv() != Second->getCallingConv())
		return false;

// Both functions must have the same return type
	if (First->getReturnType() != Second->getReturnType())
		return false;

// Checks that the arguments to the old function are of the same type as those of
// the new function, and also that they are in the same order
	for (Function::arg_iterator I = Second->arg_begin(), J = First->arg_begin(),
			IE = Second->arg_end(); I != IE; ++I, ++J) {
		if (I->getType() != J->getType())
			return false;
	}

	return true;
}

StructuredModuleEditor::InstList StructuredModuleEditor::getCallsToFunction(
		Function * F) {
	InstList Calls;

// Iterates over every basic block in the module and gathers a list of instructions
// that call to the specified function
	for (Module::iterator FI = M->begin(), FE = M->end(); FI != FE; ++FI) {
		for (Function::iterator BBI = FI->begin(), BBE = FI->end(); BBI != BBE;
				++BBI) {
			for (BasicBlock::iterator II = BBI->begin(), IE = BBI->end();
					II != IE; ++II) {
				CallSite CS(cast<Value>(II));
				// If this isn't a call, or it is a call to an intrinsic...
				if (!CS || isa<IntrinsicInst>(II))
					continue;

				Function *Callee = CS.getCalledFunction();

				if (Callee == F)
					Calls.push_back(CS.getInstruction());
			}
		}
	}

	return Calls;
}

void StructuredModuleEditor::removeChain(InstList &Chain) {
	for (InstList::reverse_iterator I = Chain.rbegin(), E = Chain.rend();
			I != E; ++I) {
		(*I)->eraseFromParent();
	}
	Chain.clear();
}

void StructuredModuleEditor::dumpInsts(InstList &Insts) {
	for (InstList::iterator I = Insts.begin(), E = Insts.end(); I != E; ++I) {
		BasicBlock *BB = (*I)->getParent();
		Function *F = BB->getParent();

		OS << "Func: " << F->getName() << " BB: " << BB->getName() << " #Uses: "
				<< (*I)->getNumUses() << " Address: " << &(**I);
		(*I)->dump();
	}
}

void StructuredModuleEditor::printIR() {
	OS << *M;
}

void StructuredModuleEditor::dumpCallGraphText() {
	CG->print(OS);
}

void StructuredModuleEditor::showCfgInDotty() {
	CG->view();
}
