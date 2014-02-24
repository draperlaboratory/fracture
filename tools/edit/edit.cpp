//===--- edit/edit.cpp - Structured LLVM IR Editor ---------------------*- C++ -*-===//
//
//                     Draper Disassembly Infrastructure
//
// This file is currently unlicensed and not for distribution.
//
//===----------------------------------------------------------------------===//
//
// The Structured LLVM IR Editor provides a CLI to permit the user to transform
// an LLVM IR Module according to a fixed set of operations
//
//===----------------------------------------------------------------------===//

#include <string>

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include "Commands/Commands.h"
#include "Edit/StructuredModuleEditor.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Global Variables and Parameters
//===----------------------------------------------------------------------===//
static std::string ProgramName;
static Commands CommandParser;

StructuredModuleEditor *Editor = 0;

//Command Line Options
cl::opt<std::string> InputFileName(cl::Positional, cl::desc("<input file>"),
		cl::init("-"));

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

static void runWrapCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 4) {
		outs()
				<< "You must specify a function to wrap as well as pre/post-invocation functions.\n";
		return;
	}

	StringRef OriginalFunc = CommandLine[1];
	StringRef PreFunc = CommandLine[2];
	StringRef PostFunc = CommandLine[3];

	if (!Editor->wrapFunc(OriginalFunc, PreFunc, PostFunc))
		outs() << "Could not wrap function '" << OriginalFunc << "'\n";
}

static void runLoadCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 2) {
		outs() << "You must specify a file to load!\n";
		return;
	}

	StringRef Filename = CommandLine[1];

	Editor->getModule(Filename);
}

static void runPrintModuleCommand(std::vector<std::string> &CommandLine) {
	Editor->printIR();
}

static void runLinkModuleCommand(std::vector<std::string> &CommandLine) {
	std::string ModuleName = CommandLine[1];
	Editor->linkModule(ModuleName);
}

static void runSignaturesCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 2) {
		outs()
				<< "You must specify a function in order to find other functions with the same signature!\n";
		return;
	}

	StringRef FuncName = CommandLine[1];

	Editor->dumpFuncsWithSameSignature(FuncName);
}

static void runInstrumentCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 2) {
		outs()
				<< "You must specify a function to which you are interested in instrumenting calls!\n";
		return;
	}

	StringRef FuncName = CommandLine[1];

	Editor->instrumentFunctionsThatCallFunction(FuncName);
}

static void runRemoveFunctionCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 2) {
		outs() << "You must specify a function to remove!\n";
		return;
	}

	StringRef FuncToRemove = CommandLine[1];

	Editor->removeFunc(FuncToRemove);
}

static void runCloneFunctionCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 2) {
		outs() << "You must specify a function to clone!\n";
		return;
	}

	StringRef FuncToClone = CommandLine[1];

	if (!Editor->cloneFunc(FuncToClone))
		outs() << "Failed to clone function '" << FuncToClone
				<< "'. Are you sure it exists in the CFG?\n";
}

static void runReplaceEdgeCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 4) {
		outs() << "You must specify a calling function, a callsite index,"
				" and a destination function!\n";
		return;
	}

	StringRef CallingFunction = CommandLine[1];

	StringRef CallSiteStr = CommandLine[2];
	uint64_t CallSiteIndex;
	CallSiteStr.getAsInteger(0, CallSiteIndex);

	StringRef DestinationName = CommandLine[3];

	if (!Editor->replaceEdge(CallingFunction, CallSiteIndex, DestinationName))
		outs() << "Could not replace edge from " << CallingFunction
				<< " to point to " << DestinationName
				<< ". Are you sure the old edge destination and the new destination "
						"have the same signature?\n";
}

static void runReplaceFunctionCommand(std::vector<std::string> &CommandLine) {
	if (CommandLine.size() < 3) {
		outs()
				<< "You must specify both a function and its replacement function!\n";
		return;
	}

	StringRef Func1 = CommandLine[1];
	StringRef Func2 = CommandLine[2];

	Editor->replaceFunc(Func1, Func2);
}

static void runCallGraphDumpCommand(std::vector<std::string> &CommandLine) {
	Editor->dumpCallGraphText();
}

static void runDottyCommand(std::vector<std::string> &CommandLine) {
	Editor->showCfgInDotty();
}

static void runQuitCommand(std::vector<std::string> &CommandLine) {
	exit(0);
}

static void initializeCommands() {
	CommandParser.registerCommand("?", &printHelp);
	CommandParser.registerCommand("help", &printHelp);
	CommandParser.registerCommand("load", &runLoadCommand);
	CommandParser.registerCommand("module", &runPrintModuleCommand);
	CommandParser.registerCommand("link", &runLinkModuleCommand);
	CommandParser.registerCommand("remove", &runRemoveFunctionCommand);
	CommandParser.registerCommand("clone", &runCloneFunctionCommand);
	CommandParser.registerCommand("edge", &runReplaceEdgeCommand);
	CommandParser.registerCommand("replace", &runReplaceFunctionCommand);
	CommandParser.registerCommand("wrap", &runWrapCommand);
	CommandParser.registerCommand("signatures", &runSignaturesCommand);
	CommandParser.registerCommand("instrument", &runInstrumentCommand);
	CommandParser.registerCommand("cfg", &runCallGraphDumpCommand);
	CommandParser.registerCommand("dotty", &runDottyCommand);
	CommandParser.registerCommand("quit", &runQuitCommand);
}

int main(int argc, char *argv[]) {

	// Removes the "./" from the beginning of the program name
	ProgramName = argv[0];
	ProgramName = ProgramName.substr(2, ProgramName.length() - 2);

	// If no parameter is given to edit, stops execution
	if (argc < 2) {
		// Tells the user how to run the program
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

	cl::ParseCommandLineOptions(argc, argv, "Structured LLVM IR Editor");

	// Initializes the commands for this tool
	initializeCommands();

	// Instantiates our StructuredModuleEditor with the specified file and output stream
	Editor = new StructuredModuleEditor(InputFileName, outs());

	// Starts the CLI
	CommandParser.runShell(ProgramName);

	return 0;
}
