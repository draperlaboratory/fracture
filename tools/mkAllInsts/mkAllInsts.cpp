//===--- mkAllARMInsts.cpp --------------------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Builds a directory containing binaries for every valid ARM instruction.
// These binaries can then be run on Fracture for testing purposes.
//
// NOTE: Fracture must be able to handle the BX_RET opcode correctly in order
//		 for these binaries to be useful
//
// Author: cjw3357
// Date: July 21, 2014
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/Support/TargetSelect.h"
#include "../lib/Target/ARM/InstPrinter/ARMInstPrinter.h"
#include "../../fracture/lib/Target/ARM/ARMBaseInfo.h"
#include "../lib/Target/X86/InstPrinter/X86IntelInstPrinter.h"
#include "../../fracture/lib/Target/X86/X86BaseInfo.h"

using namespace llvm;

raw_fd_ostream *RS, *US, *SS;

void makeBins(std::string TripleName, std::string DirName, bool printAsm);
MCInstBuilder* buildMI(std:: string triple, const MCInstrInfo *MII,
											MCContext *MCCtx, unsigned op);
MCInstBuilder* buildARMMI(const MCInstrInfo *MII,MCContext *MCCtx,unsigned op);
MCInstBuilder* buildX86MI(const MCInstrInfo *MII,MCContext *MCCtx,unsigned op);
MCInstPrinter* getTargetInstPrinter(std::string triple,
					const MCAsmInfo *AsmInfo, const MCInstrInfo *MII,
					const MCRegisterInfo *MRI,  const MCSubtargetInfo *STI);

int main(int argc, char* argv[])
{
	//Output stream and error stream for sending info to the console
	raw_ostream &OS = outs();
	raw_ostream &ES = errs();
	RS = NULL; US = NULL; SS = NULL;
	std::string ErrMsg;
	
	enum Arch {NONE, ARM, x86, PPC, MIPS};
	
	//Handle flags and arguments
	Arch arch = NONE;
	bool printAsm = false;
	std::string arg;
	for(int i = 1; i < argc; i++) {
		arg = argv[i];
		if(arg == "-help") {
			OS << "\nmkAllInsts -- builds a directory containing a binary " << 
				  "for every supported\ninstruction on a given architecture" <<
				  "\n\nUsage:  mkAllInsts [options] <arch>\n\n\toptions:" <<
				  "\n\t\t-help\t- displays program info\n\t\t-asm\t- " <<
				  "creates files with assembly code instead of binary\n\t\t" <<
				  "-res\t- creates a file with the results of every " <<
				  "instruction\n\t\t-unsup\t- creates a file listing every " <<
				  "unsupported instruction\n\t\t-sup\t- creates a file " <<
				  "listing every supported instruction\n\t" <<
				  "arch:\n\t\t-arm\t- specifies ARM architecture\n\t\t-x86" <<
				  "\t- specifies x86 architecture\n\t\t-ppc\t- specifies " <<
				  "PowerPC architecture\n\t\t-mips\t- specifies MIPS " <<
				  "architecture\n\n";
			return 0;
		} else if(arg == "-asm") {
			printAsm = true;
		} else if(arg == "-res") {
			RS = new raw_fd_ostream("results.txt", ErrMsg, sys::fs::F_None);
		} else if(arg == "-unsup") {
			US = new raw_fd_ostream("unsupported.txt",ErrMsg,sys::fs::F_None);
		} else if(arg == "-sup") {
			SS = new raw_fd_ostream("supported.txt", ErrMsg, sys::fs::F_None);
		} else if(arch != NONE) {
			if(arg == "-arm" || arg == "-x86" ||
					arg == "-ppc" || arg == "-mips") {
				ES << "mkAllInsts: May only specify one architecture." <<
											" Use -help for more info.\n";
				return 1;
			} else {
				ES << "mkAllInsts: Unknown argument '" << arg <<
											"'. Use -help for more info.\n"; 
				return 2;
			}
		} else {
			if(arg == "-arm") { arch = ARM; }
			else if(arg == "-x86") { arch = x86; }
			else if(arg == "-ppc") { arch = PPC; }
			else if(arg == "mips") { arch = MIPS; }
			else {
				ES << "mkAllInsts: unknown argument '" << arg <<
											"'. Use -help for more info\n"; 
				return 2;
			}
		}
	}
	
	//Initialize instruction info
	InitializeAllTargetInfos();
	std::string TripleName, DirName;
	
	if(arch == NONE) {
		ES << "mkAllInsts: Must specify an architecture." <<
											" Use -help for more info.\n";
		return 3;
	} else if(arch == ARM) {
		system("rm -rf ARMbins/");
		system("rm -rf ARMasms/");
		if(printAsm) {
			system("mkdir ARMasms");
			DirName = " ARMasms/";
		} else {
			system("mkdir ARMbins");
			DirName = " ARMbins/";
		}
		TripleName = "arm-unknown-unknown";
		LLVMInitializeARMTargetMC();
	} else if(arch == x86) {
		system("rm -rf x86bins/");
		system("rm -rf x86asms/");
		if(printAsm) {
			system("mkdir x86asms");
			DirName = " x86asms/";
		} else {
			system("mkdir x86bins");
			DirName = " x86bins/";
		}
		TripleName = "i386-unknown-unknown";
		LLVMInitializeX86TargetMC();
	} else if(arch == PPC) {
		ES << "mkAllInsts: PPC is not implemented\n";
		return 4;
	} else if(arch == MIPS) {
		ES << "mkAllInsts: MIPS is not implemented\n";
		return 4;
	}
	
	makeBins(TripleName, DirName, printAsm);
	
	if(RS) { delete RS; }
	if(US) { delete US; }
	if(SS) { delete SS; }
	
	return 0;
}

// * makeBins - For each ARM instruction, make a file in a sub-directory
//				containing a binary that can be run on Fracture
void makeBins(std::string TripleName, std::string DirName, bool printAsm)
{	
	//Create LLVM objects necessary for building a machine instruction
	std::string ErrMsg;
	const Target *TheTarget = TargetRegistry::lookupTarget(TripleName, ErrMsg);
	const MCInstrInfo *MII = TheTarget->createMCInstrInfo();
	const MCRegisterInfo *MRI = TheTarget->createMCRegInfo(TripleName);
	const MCAsmInfo *AsmInfo = TheTarget->createMCAsmInfo(*MRI, TripleName);
	MCObjectFileInfo *MCOFI = new MCObjectFileInfo();
	MCContext *MCCtx = new MCContext(AsmInfo, MRI, MCOFI);
	
	//Create LLVM objects necessary for printing a machine instruction
	StringRef CPUName = "generic", Features = "", annot = "";
	const MCSubtargetInfo *STI = TheTarget->createMCSubtargetInfo(TripleName,
															CPUName, Features);
	SmallVectorImpl<MCFixup> *dummy = new SmallVector<MCFixup, 0>();
	MCInstPrinter *MIP = getTargetInstPrinter(TripleName,AsmInfo,MII,MRI,STI);
	MCCodeEmitter *MCE = TheTarget->createMCCodeEmitter(*MII, *MRI, *STI,
																	*MCCtx);
	MCInstBuilder *RET;
	unsigned lastInst;
	if(TripleName == "arm-unknown-unknown") {
		lastInst = ARM::INSTRUCTION_LIST_END;
		RET = new MCInstBuilder(ARM::BX_RET);
		RET->addImm(14);
		RET->addReg(0);
	} else if(TripleName == "i386-unknown-unknown") {
		lastInst = X86::INSTRUCTION_LIST_END;
		RET = new MCInstBuilder(X86::RETL);
	} else {
		lastInst = 0;
		RET = new MCInstBuilder(0);
	}
	
	//Loop through each instruction and print it to a file
	const char *opname;
	std::string cmd;
	
	raw_fd_ostream *FS;
	MCInstBuilder *MIB;
	for(unsigned op = 0; op < lastInst; op++) {
		//Build the machine instruction
		MIB = buildMI(TripleName, MII, MCCtx, op);
		
		//If it's valid, print the machine instruction
		if(MIB) {
			opname = MII->getName(op);
			FS = new raw_fd_ostream(opname, ErrMsg, sys::fs::F_None);
			MCInst MI = static_cast<MCInst&>(*MIB);
			cmd = DirName;
			if(RS) {
				MIP->printInst(&MI, *RS, annot);
				*RS << "\n"; }
			if(printAsm) {
				*FS << MII->getName(op) << ": ";
				MIP->printInst(&MI, *FS, annot);
				*FS << "\n";
			} else {
				//*RS << "SUCCESS\n";
				MCE->EncodeInstruction(MI, *FS, *dummy, *STI);
				MI = static_cast<MCInst&>(*RET);
				MCE->EncodeInstruction(MI, *FS, *dummy, *STI);
			}
			cmd = "mv " + (std::string)opname + cmd;
			system(cmd.c_str());
			delete FS;
			delete MIB;
		}
	}
	delete RET;
	delete MCE;
	delete MIP;
	delete dummy;
	delete MCCtx;
	delete MCOFI;
	delete STI;
	delete AsmInfo;
	delete MRI;
	delete MII;
	//delete TheTarget;
}

MCInstBuilder* buildMI(std:: string triple, const MCInstrInfo *MII,
											MCContext *MCCtx, unsigned op)
{
	if(triple == "arm-unknown-unknown") {
		return buildARMMI(MII, MCCtx, op);
	} else if(triple == "i386-unknown-unknown") {
		return buildX86MI(MII, MCCtx, op);
	} else {
		return NULL;
	}
}

// * buildMI  - Returns an MCInstBuilder* pointing to a valid representation
// *			of the ARM instruction corresponding to the op code 'op'
MCInstBuilder* buildARMMI(const MCInstrInfo *MII,MCContext *MCCtx,unsigned op)
{
	std::string opname = MII->getName(op);
	const MCInstrDesc MID = MII->get(op);
	if(RS) { *RS << opname << ":\t"; }
	//Don't make an MIB if it's a pseudo-instruction
	if(MID.isPseudo()) {
		if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
		if(US) { *US << opname << "\n"; }
		return NULL;
	}
	//Don't make an MIB it it's one of these instructions
	else if(op == ARM::t2Int_eh_sjlj_setjmp ||
			op == ARM::t2Int_eh_sjlj_setjmp_nofp ||
			op == ARM::tInt_eh_sjlj_longjmp ||
			op == ARM::tInt_eh_sjlj_setjmp ||
			op == ARM::VLDMQIA || op == ARM::VSTMQIA ||
			op == ARM::tPICADD ||
			opname.find("Pseudo") != std::string::npos) {
		if(RS) { *RS << "ERROR IS UNPRINTABLE!\n"; }
		if(US) { *US << opname << "\n"; }
		return NULL;
	}
	//Make an MIB containing valid operands for the instruction
	else {
		if(SS) { *SS << opname << "\n"; }
		MCInstBuilder *MIB = new MCInstBuilder(op);
		const MCOperandInfo *MOI = MID.OpInfo;
		MCOperandInfo opinfo;
		unsigned optype;
		unsigned numopers = MID.NumOperands;
		bool firstPred = true;
		
		for(unsigned i = 0; i < numopers; i++) {
			opinfo = MOI[i];
			optype = opinfo.OperandType;
			if(optype == MCOI::OPERAND_UNKNOWN) {
			
				if(opinfo.isPredicate()) {
					if(firstPred) {
						MIB->addImm(14);
						firstPred = false;
					} else {
						MIB->addReg(0);
						firstPred = true;
					}
				} else if(opinfo.isOptionalDef()) {
					MIB->addReg(0);
				} else if((op == ARM::tLDRspi || op == ARM::tSTRspi) &&
																i == 1) {
					MIB->addReg(ARM::SP);
				} else if(opinfo.RegClass == -1) {
				
					if((op>=ARM::FLDMXDB_UPD && op<=ARM::FSTMXIA_UPD) ||
						  (op>=ARM::LDMDA && op<=ARM::LDMIB_UPD) ||
						  (op>=ARM::STMDA && op<=ARM::STMIB_UPD) ||
						  (op>=ARM::VLDMDDB_UPD && op<=ARM::VLDMSIA_UPD) ||
						  (op>=ARM::VSTMDDB_UPD && op<=ARM::VSTMSIA_UPD) ||
						  (op>=ARM::sysLDMDA && op<=ARM::sysSTMIB_UPD) ||
						  (op>=ARM::t2LDMDB && op<=ARM::t2LDMIA_UPD) ||
						  (op>=ARM::t2STMDB && op<=ARM::t2STMIA_UPD) ||
						  (op==ARM::tLDMIA || op==ARM::tPOP) ||
						  (op==ARM::tPUSH || op==ARM::tSTMIA_UPD)) {
						MIB->addReg(ARM::R0);
					} else if((op >= ARM::t2LDRD_POST &&
								op <= ARM::t2LDRDi8) ||
								(op >= ARM::t2STRD_POST &&
								op <= ARM::t2STRDi8)) {
						MIB->addImm(0);
					} else {
						MIB->addImm(2);
					}
					
				} else if((opname.find("x2") != std::string::npos ||
							opname.find("VLD2b") != std::string::npos ||
							opname.find("VST2b") != std::string::npos) &&
							opinfo.RegClass == ARM::DPairRegClassID) {
					MIB->addReg(ARM::D0_D2);
				} else {
					MIB->addReg(ARMMCRegisterClasses[
										opinfo.RegClass].getRegister(0));
				}
				
			} else if(optype == MCOI::OPERAND_IMMEDIATE) {
				MIB->addImm(0);
			} else if(optype == MCOI::OPERAND_REGISTER) {
				MIB->addReg(ARMMCRegisterClasses[opinfo.RegClass].getRegister(0));
			} else if(optype == MCOI::OPERAND_MEMORY) {
		 		MIB->addExpr(MCConstantExpr::Create(0x8000, *MCCtx));
			} else if(optype == MCOI::OPERAND_PCREL) {
				MIB->addImm(0x10);
			}
		}
		return MIB;
	}
}

MCInstBuilder* buildX86MI(const MCInstrInfo *MII,MCContext *MCCtx,unsigned op)
{
	raw_ostream &OS = outs();
	
	std::string opname = MII->getName(op);
	const MCInstrDesc MID = MII->get(op);
	if(RS) { *RS << opname << ":\t"; }
	OS << opname << "\n";
	//Don't make an MIB if it's a pseudo-instruction
	if(MID.isPseudo()) {
		if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
		if(US) { *US << opname << "\n"; }
		return NULL;
	}
	else if(opname.find("Fp") != std::string::npos) {
		if(RS) { *RS << "ERROR IS UNPRINTABLE!\n"; }
		if(US) { *US << opname << "\n"; }
		return NULL;
	}
	//Make an MIB containing valid operands for the instruction
	else {
		if(SS) { *SS << opname << "\n"; }
		MCInstBuilder *MIB = new MCInstBuilder(op);
		const MCOperandInfo *MOI = MID.OpInfo;
		MCOperandInfo opinfo;
		unsigned optype;
		unsigned numopers = MID.NumOperands;
		//bool firstPred = true;
		
		for(unsigned i = 0; i < numopers; i++) {
			opinfo = MOI[i];
			optype = opinfo.OperandType;
			if(optype == MCOI::OPERAND_UNKNOWN) {
			
				if(opinfo.RegClass == -1) {
					MIB->addImm(0);
				} else {
					MIB->addReg(X86MCRegisterClasses[
										opinfo.RegClass].getRegister(0));
				}
				
			} else if(optype == MCOI::OPERAND_IMMEDIATE) {
				MIB->addImm(0);
			} else if(optype == MCOI::OPERAND_REGISTER) {
				MIB->addReg(X86MCRegisterClasses[
										opinfo.RegClass].getRegister(0));
			} else if(optype == MCOI::OPERAND_MEMORY) {
		 		MIB->addExpr(MCConstantExpr::Create(0x8000, *MCCtx));
			} else if(optype == MCOI::OPERAND_PCREL) {
				MIB->addImm(0x10);
			}
		}
		return MIB;
	}
}

MCInstPrinter* getTargetInstPrinter(std::string triple,
					const MCAsmInfo *AsmInfo, const MCInstrInfo *MII,
					const MCRegisterInfo *MRI,  const MCSubtargetInfo *STI)
{
	MCInstPrinter *MIP;
	if(triple == "arm-unknown-unknown") {
		MIP = new ARMInstPrinter(*AsmInfo, *MII, *MRI, *STI);
		return MIP;
	} else if(triple == "i386-unknown-unknown") {
		MIP = new X86IntelInstPrinter(*AsmInfo, *MII, *MRI);
		return MIP;
	} else {
		return NULL;
	}
}


