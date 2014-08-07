//===--- mkAllInsts.cpp --------------------------------------*- C++ -*-===//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Builds a directory containing binaries for every supported instruction on a
// given architecture. These binaries can then be run on Fracture for testing
// purposes.
//
// NOTE: Fracture must be able to handle a valid return opcode correctly in
//       order for these binaries to be useful
//          -For ARM:        BX_RET
//          -For i386:       RETL
//          -For PowerPC64:  BLR
//          -For MIPS:       (TBD)
//
// NOTE: MIPS has not been implemented yet, but most of its implementation
//       has been put in as comments. To implement it:
//          -uncomment out the relevant peices of code
//          -replace instances of *MIPS triple* with the correct triple
//              for MIPS
//          -in makeBins, set RET to a valid return instruction for MIPS
//              and give it the correct operands
//          -the "buildMI" function for MIPS will have to be tweaked
//              substantially until as many instructions as possible can pass
//              through it and be printed successfully
//
// Author: cjw3357
// Date: Aug 7, 2014
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "../lib/Target/ARM/InstPrinter/ARMInstPrinter.h"
#include "../lib/Target/X86/InstPrinter/X86IntelInstPrinter.h"
#include "../lib/Target/PowerPC/InstPrinter/PPCInstPrinter.h"
//#include "../lib/Target/Mips/InstPrinter/MipsInstPrinter.h"
#define GET_REGINFO_ENUM
#include "../lib/Target/ARM/ARMGenRegisterInfo.inc"
#define GET_REGINFO_ENUM
#include "../lib/Target/X86/X86GenRegisterInfo.inc"
#define GET_REGINFO_ENUM
#include "../lib/Target/PowerPC/PPCGenRegisterInfo.inc"
/*#define GET_REGINFO_ENUM
#include "../lib/Target/Mips/MipsGenRegisterInfo.inc"*/
#define GET_INSTRINFO_ENUM
#include "../lib/Target/ARM/ARMGenInstrInfo.inc"
#define GET_INSTRINFO_ENUM
#include "../lib/Target/X86/X86GenInstrInfo.inc"
#define GET_INSTRINFO_ENUM
#include "../lib/Target/PowerPC/PPCGenInstrInfo.inc"
/*#define GET_INSTRINFO_ENUM
#include "../lib/Target/Mips/MipsGenInstrInfo.inc"*/

using namespace llvm;

//===----------------------------------------------------------------------===//
// Global Variables and Struct Definitions
//===----------------------------------------------------------------------===//

//Simple struct to facilitate printing an instruction
struct MIBplus {
    MCInstBuilder *MIB;
    bool asmPrintable;
};

//Global output and error stream for sending info to the console
raw_ostream &OS = outs(), &ES = errs();
//Global file streams for the Result File, Unsupported File, and Supported File
raw_fd_ostream *RS, *US, *SS;

//===----------------------------------------------------------------------===//
// Function Declarations
//===----------------------------------------------------------------------===//

void makeBins(std::string TripleName, std::string DirName, bool printAsm);
MIBplus buildMI(std:: string triple, const MCInstrInfo *MII,
                                            MCContext *MCCtx, unsigned op);
MIBplus buildARMMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op);
MIBplus buildX86MI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op);
MIBplus buildPPCMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op);
//MIBplus buildMIPSMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op);
MCInstPrinter* getTargetInstPrinter(std::string triple,
                    const MCAsmInfo *AsmInfo, const MCInstrInfo *MII,
                    const MCRegisterInfo *MRI,  const MCSubtargetInfo *STI);

//===----------------------------------------------------------------------===//
// Main and Function Definitions
//===----------------------------------------------------------------------===//

int main(int argc, char* argv[])
{
    //Output stream and error stream for sending info to the console
    //Initialize global file streams
    RS = NULL; US = NULL; SS = NULL;
    
    enum Arch {NONE, arm, i386, powerpc64, mips};
    
    //Handle flags and arguments
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
                  "arch:\n\t\t-arm\t- specifies ARM architecture\n\t\t-i386" <<
                  "\t- specifies x86 32-bit architecture\n\t\t-powerpc64\t-" <<
                  " specifies PowerPC 64-bit architecture\n\t\t-mips\t- " <<
                  "specifies MIPS architecture (unimplemented)\n\n";
            return 0;
        }
    }
    Arch arch = NONE;
    bool printAsm = false;
    bool res = false, sup = false, unsup = false;
    std::string ErrMsg;
    for(int i = 1; i < argc; i++) {
        arg = argv[i];
        if(arg == "-asm") {
            printAsm = true;
        } else if(arg == "-res") {
            res = true;
        } else if(arg == "-unsup") {
            unsup = true;
        } else if(arg == "-sup") {
            sup = true;
        } else if(arch != NONE) {
            if(arg == "-arm" || arg == "-i386" ||
                    arg == "-powerpc64" || arg == "-mips") {
                ES << "mkAllInsts: May only specify one architecture." <<
                                            " Use -help for more info.\n";
                return 1;
            } else {
                ES << "mkAllInsts: Unknown flag '" << arg <<
                                            "'. Use -help for more info.\n";
                return 2;
            }
        } else {
            if(arg == "-arm") { arch = arm; }
            else if(arg == "-i386") { arch = i386; }
            else if(arg == "-powerpc64") { arch = powerpc64; }
            else if(arg == "-mips") { arch = mips; }
            else {
                ES << "mkAllInsts: unknown flag '" << arg <<
                                            "'. Use -help for more info\n";
                return 2;
            }
        }
    }
    
    //Initialize instruction info
    InitializeAllTargetInfos();
    
    //Initialize the target
    std::string TripleName, DirName, filePre;
    if(arch == NONE) {
        ES << "mkAllInsts: Must specify an architecture." <<
                                            " Use -help for more info.\n";
        return 3;
    } else if(arch == arm) {
        filePre = "arm-";
        system("rm -rf armBins/");
        system("rm -rf armAsms/");
        if(printAsm) {
            system("mkdir armAsms");
            DirName = " armAsms/";
        } else {
            system("mkdir armBins");
            DirName = " armBins/";
        }
        TripleName = "arm-unknown-unknown";
        LLVMInitializeARMTargetMC();
    } else if(arch == i386) {
        filePre = "i386-";
        system("rm -rf i386Bins/");
        system("rm -rf i386Asms/");
        if(printAsm) {
            system("mkdir i386Asms");
            DirName = " i386Asms/";
        } else {
            system("mkdir i386Bins");
            DirName = " i386Bins/";
        }
        TripleName = "i386-unknown-unknown";
        LLVMInitializeX86TargetMC();
    } else if(arch == powerpc64) {
        filePre = "powerpc64-";
        system("rm -rf powerpc64Bins/");
        system("rm -rf powerpc64Asms/");
        if(printAsm) {
            system("mkdir powerpc64Asms");
            DirName = " powerpc64Asms/";
        } else {
            system("mkdir powerpc64Bins");
            DirName = " powerpc64Bins/";
        }
        TripleName = "powerpc64-unknown-unknown";
        LLVMInitializePowerPCTargetMC();
    } else if(arch == mips) {
        ES << "mkAllInsts: MIPS is not implemented\n";
        return 4;
        /*filePre = "mips-";
        system("rm -rf mipsBins/");
        system("rm -rf mipsAsms/");
        if(printAsm) {
            system("mkdir mipsAsms");
            DirName = " mipsAsms/";
        } else {
            system("mkdir mipsBins");
            DirName = " mipsBins/";
        }
        TripleName = *MIPS triple*;
        LLVMInitializeMipsTargetMC();*/
    }
    
    if(res) {
        RS = new raw_fd_ostream((filePre + "results.txt").c_str(),
                                                    ErrMsg, sys::fs::F_None);
    } else if(unsup) {
        US = new raw_fd_ostream((filePre + "unsupported.txt").c_str(),
                                                    ErrMsg, sys::fs::F_None);
    } else if(sup) {
        SS = new raw_fd_ostream((filePre + "supported.txt").c_str(),
                                                    ErrMsg, sys::fs::F_None);
    }
    
    //Call function to create the binaries
    makeBins(TripleName, DirName, printAsm);
    
    if(RS) { delete RS; }
    if(US) { delete US; }
    if(SS) { delete SS; }
    
    return 0;
}

//===----------------------------------------------------------------------===//
// * makeBins - For each instruction on a given architecture, make a file in a
// *            sub-directory containing a binary that can be run on Fracture
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
    //Create a valid return instruction for the given architecture
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
    } else if(TripleName == "powerpc64-unknown-unknown") {
        lastInst = PPC::INSTRUCTION_LIST_END;
        RET = new MCInstBuilder(PPC::BLR);
    /*} else if(TripleName == *MIPS triple*) {
        lastInst = Mips::INSTRUCTION_LIST_END;
        RET = new MCInstBuilder(*MIPS return*);*/
    } else {
        ES << "mkAllInsts::makeBins: unknown triple name received\n";
        abort();
    }
    
    //Create an array to keep track of used file names
    std::string *flist = new std::string[lastInst];
    for(unsigned i = 0; i < lastInst; i++) {
        flist[i] = "";
    }
    
    //Loop variables
    const char *opname;
    char suffix;
    std::string cmd, fname, tempname;
    raw_fd_ostream *FS;
    MIBplus MIBP;
    //Loop through each instruction and print it to a file
    for(unsigned op = 0; op < lastInst; op++) {
        
        //Build the machine instruction
        MIBP = buildMI(TripleName, MII, MCCtx, op);
        
        //If it's valid, print the machine instruction
        if(MIBP.MIB) {
            opname = MII->getName(op);
            
            //Determine if the file name is equivalent to the name of a
            //previously created file without case sensitivity
            //If it is, add a suffix to differentiate it
            fname = (std::string)opname;
            tempname = fname;
            for(unsigned i = 0; i < tempname.size(); i++) {
                tempname[i] = std::tolower(tempname[i]);
            }
            suffix = '1';
            for(int i = 0; i < (int)op; i++) {
                if(flist[i] == tempname) {
                    if(suffix == '1') {
                        fname += ((std::string)"-" + suffix);
                        tempname += ((std::string)"-" + suffix);
                    } else {
                        fname[fname.size()-1] = suffix;
                        tempname[tempname.size()-1] = suffix;
                    }
                    suffix++;
                    i = -1;
                }
            }
            flist[op] = tempname;
            
            //Print the instruction to a file and put the file in place
            FS = new raw_fd_ostream(fname.c_str(), ErrMsg, sys::fs::F_None);
            MCInst MI = static_cast<MCInst&>(*(MIBP.MIB));
            cmd = DirName;
            if(RS && MIBP.asmPrintable) {
                MIP->printInst(&MI, *RS, annot);
                *RS << "\n";
            }
            if(SS) {
                if(MIBP.asmPrintable) {
                    MIP->printInst(&MI, *SS, annot);
                    *SS << "\n";
                } else {
                    *SS << "\tCANNOT PRINT TO ASM!\n";
                }
            }
            if(printAsm) {
                *FS << MII->getName(op) << ": ";
                if(MIBP.asmPrintable) {
                    MIP->printInst(&MI, *FS, annot);
                    *FS << "\n";
                } else {
                    *FS << "CANNOT PRINT TO ASM!\n";
                }
            } else {
                MCE->EncodeInstruction(MI, *FS, *dummy, *STI);
                MI = static_cast<MCInst&>(*RET);
                MCE->EncodeInstruction(MI, *FS, *dummy, *STI);
            }
            cmd = "mv " + (std::string)fname + cmd;
            system(cmd.c_str());
            delete FS;
            delete MIBP.MIB;
        }
    }
    delete[] flist;
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
    //delete TheTarget; <-FIXME Segfaults when uncommented. Problematic?
}

//===----------------------------------------------------------------------===//
// * buildMI - Calls the appropriate target function for the given triple
// *
MIBplus buildMI(std:: string triple, const MCInstrInfo *MII,
                                                MCContext *MCCtx, unsigned op)
{
    if(triple == "arm-unknown-unknown") {
        return buildARMMI(MII, MCCtx, op);
    } else if(triple == "i386-unknown-unknown") {
        return buildX86MI(MII, MCCtx, op);
    } else if(triple == "powerpc64-unknown-unknown") {
        return buildPPCMI(MII, MCCtx, op);
    /*} else if(triple == *MIPS triple*) {
        return buildMIPSMI(MII, MCCtx, op);*/
    } else {
        ES << "mkAllInsts::buildMI: unknown triple name received\n";
        abort();
    }
}

//===----------------------------------------------------------------------===//
// * buildARMMI - Returns an MIBplus containing a valid representation of the
// *              ARM instruction corresponding to the op code 'op', along with
// *              a boolean representing whether the instruction can be printed
// *              to assembly or not
MIBplus buildARMMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op)
{
    //Initialize the MIBplus
    MIBplus MIBP;
    MIBP.asmPrintable = true;
    MIBP.MIB = NULL;

    std::string opname = MII->getName(op);
    const MCInstrDesc MID = MII->get(op);
    uint64_t TSFlags = MID.TSFlags;
    unsigned short size = MID.Size;
    if(RS) { *RS << opname << ":\t"; }
    
    //Don't make an MIB if it's a pseudo-instruction
    if(MID.isPseudo()) {
        if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
        if(US) { *US << opname << ":\tIS PSEUDO INSTRUCTION\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Don't make an MIB if ARMMCCodeEmitter won't be able to handle it
    //Condition was taken from ARMMCCodeEmitter::EncodeInstruction
    else if(size == 0 ||
    (TSFlags & (0x3f << 7)/*ARMII::FormMask*/) == (0 << 7)/*ARMII::Pseudo*/) {
        if(RS) { *RS << "IS UNSUPPORTED!\n"; }
        if(US) { *US << opname << ":\tIS UNSUPPORTED\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Make an MIB containing valid operands for the instruction
    else {
        //tPICADD seems to be able to print to binary, but not to assembly
        if(op == ARM::tPICADD) {
            if(RS) { *RS << "CANNOT PRINT TO ASM!\n"; }
            MIBP.asmPrintable = false;
        }
        if(SS) { *SS << opname << ":"; }
        MIBP.MIB = new MCInstBuilder(op);
        const MCOperandInfo *MOI = MID.OpInfo;
        MCOperandInfo opinfo;
        unsigned optype;
        unsigned numopers = MID.NumOperands;
        bool firstPred = true;
        
        //This loop goes through and adds the correct operands to the MIB
        for(unsigned i = 0; i < numopers; i++) {
            opinfo = MOI[i];
            optype = opinfo.OperandType;
            if(optype == MCOI::OPERAND_UNKNOWN) {
            
                if(opinfo.isPredicate()) {
                    if(firstPred) {
                        MIBP.MIB->addImm(14);
                        firstPred = false;
                    } else {
                        MIBP.MIB->addReg(0);
                        firstPred = true;
                    }
                } else if(opinfo.isOptionalDef()) {
                    MIBP.MIB->addReg(0);
                } else if((op == ARM::tLDRspi || op == ARM::tSTRspi) &&
                                                                i == 1) {
                    MIBP.MIB->addReg(ARM::SP);
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
                        MIBP.MIB->addReg(ARM::R0);
                    } else if((op >= ARM::t2LDRD_POST &&
                                op <= ARM::t2LDRDi8) ||
                                (op >= ARM::t2STRD_POST &&
                                op <= ARM::t2STRDi8)) {
                        MIBP.MIB->addImm(0);
                    } else {
                        MIBP.MIB->addImm(2);
                    }
                    
                } else if((opname.find("x2") != std::string::npos ||
                            opname.find("VLD2b") != std::string::npos ||
                            opname.find("VST2b") != std::string::npos) &&
                            opinfo.RegClass == ARM::DPairRegClassID) {
                    MIBP.MIB->addReg(ARM::D0_D2);
                } else {
                    MIBP.MIB->addReg(ARMMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
                }
                
            } else if(optype == MCOI::OPERAND_IMMEDIATE) {
                MIBP.MIB->addImm(0);
            } else if(optype == MCOI::OPERAND_REGISTER) {
                MIBP.MIB->addReg(ARMMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
            } else if(optype == MCOI::OPERAND_MEMORY) {
                 MIBP.MIB->addExpr(MCConstantExpr::Create(0x8000, *MCCtx));
            } else if(optype == MCOI::OPERAND_PCREL) {
                MIBP.MIB->addImm(0x10);
            }
        }
        return MIBP;
    }
}

//===----------------------------------------------------------------------===//
// * buildX86MI - Returns an MIBplus containing a valid representation of the
// *              x86 instruction corresponding to the op code 'op', along with
// *              a boolean representing whether the instruction can be printed
// *              to assembly or not
MIBplus buildX86MI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op)
{
    //Initialize the MIBplus
    MIBplus MIBP;
    MIBP.asmPrintable = true;
    MIBP.MIB = NULL;
    
    std::string opname = MII->getName(op);
    const MCInstrDesc MID = MII->get(op);
    uint64_t TSFlags = MID.TSFlags;
    if(RS) { *RS << opname << ":\t"; }
    
    //Don't make an MIB if it's a pseudo-instruction
    if(MID.isPseudo()) {
        if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
        if(US) { *US << opname << ":\tIS PSEUDO INSTRUCTION!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Don't make an MIB if X86MCCodeEmitter won't be able to handle it
    //Condition was taken from X86MCCodeEmitter::EncodeInstruction
    else if((TSFlags & 127/*X86II::FormMask*/) == 0/*X86II::Pseudo*/) {
        if(RS) { *RS << "IS UNSUPPORTED!\n"; }
        if(US) { *US << opname << ":\tIS UNSUPPORTED!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //TSFlags for EH_RETURN and EH_RETURN64 execute the LLVM unreachable
    //"Unknown immediate size" at X86BaseInfo.h:562, so skip them
    else if(op == X86::EH_RETURN || op == X86::EH_RETURN64) {
        if(RS) { *RS << "ERROR UNKNOWN IMMEDIATE SIZE!\n"; }
        if(US) { *US << opname << ":\tERROR UNKNOWN IMMEDIATE SIZE!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Int_CVTSD2SSrm and Int_VCVTSD2SSrm execute the abort at
    //X86MCCodeEmitter.cpp:1565, so skip them
    else if(op == X86::Int_CVTSD2SSrm || op == X86::Int_VCVTSD2SSrm) {
        if(RS) { *RS << "ERROR CANNOT ENCODE ALL OPERANDS!\n"; }
        if(US) { *US << opname << ":\tERROR CANNOT ENCODE ALL OPERANDS!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //MCCodeEmitter appears to be unable to handle these 12 instructions, so
    //skip them
    else if(opname.find("mik") != std::string::npos ||
                opname.find("rik") != std::string::npos) {
        if(RS) { *RS << "ERROR CANNOT ENCODE CORRECTLY!\n"; }
        if(US) {*US << opname << ":\tERROR CANNOT ENCODE CORRECTLY!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Make an MIB containing valid operands for the instruction
    else {
        //MOV32ri64 and TAILJMPr seem to be able to print to binary, but not to
        //    assembly
        if(op == X86::MOV32ri64 || op == X86::TAILJMPr) {
            if(RS) { *RS << "CANNOT PRINT TO ASSEMBLY!\n"; }
            MIBP.asmPrintable = false;
        }
        if(SS) { *SS << opname << ":"; }
        MIBP.MIB = new MCInstBuilder(op);
        const MCOperandInfo *MOI = MID.OpInfo;
        MCOperandInfo opinfo;
        unsigned optype;
        unsigned numopers = MID.NumOperands;
        
        //Special cases
        if(numopers == 2 && MOI[0].OperandType == MCOI::OPERAND_MEMORY &&
                !MOI[0].isLookupPtrRegClass() &&
                MOI[1].OperandType == MCOI::OPERAND_MEMORY &&
                !MOI[1].isLookupPtrRegClass()) {
            MIBP.MIB->addExpr(MCConstantExpr::Create(0x0, *MCCtx));
            MIBP.MIB->addReg(0);
        } else if(op == X86::LEA64_32r || op == X86::LEA64r) {
            MIBP.MIB->addReg(X86::EAX);
            MIBP.MIB->addReg(X86::EAX);
            MIBP.MIB->addImm(1);
            MIBP.MIB->addReg(X86::EAX);
            MIBP.MIB->addExpr(MCConstantExpr::Create(0x0, *MCCtx));
            MIBP.MIB->addReg(0);
        } else {
            //This loop goes through and adds the correct operands to the MIB
            for(unsigned i = 0; i < numopers; i++) {
                opinfo = MOI[i];
                optype = opinfo.OperandType;
                if(optype == MCOI::OPERAND_UNKNOWN) {
                    if(opinfo.RegClass == -1) {
                        MIBP.MIB->addImm(0);
                    } else {
                        MIBP.MIB->addReg(X86MCRegisterClasses[
                                            opinfo.RegClass].getRegister(0));
                    }
                } else if(optype == MCOI::OPERAND_IMMEDIATE) {
                    MIBP.MIB->addImm(0);
                } else if(optype == MCOI::OPERAND_REGISTER) {
                    MIBP.MIB->addReg(X86MCRegisterClasses[
                                            opinfo.RegClass].getRegister(0));
                } else if(optype == MCOI::OPERAND_MEMORY) {
                
                    if(opinfo.isLookupPtrRegClass()) {
                        if((op >= X86::CMPS16 && op <= X86::CMPS8) ||
                                op == X86::MOVSB || op == X86::MOVSL ||
                                op == X86::MOVSQ || op == X86::MOVSW) {
                            MIBP.MIB->addReg(X86::EDI);
                            MIBP.MIB->addReg(X86::ESI);
                            MIBP.MIB->addReg(0);
                            i += 2;
                        } else if((op >= X86::LODSB && op <= X86::LODSW) ||
                                    (op >= X86::OUTSB && op <= X86::OUTSW)) {
                            MIBP.MIB->addReg(X86::EAX);
                            MIBP.MIB->addReg(0);
                            i += 1;
                        } else {
                            MIBP.MIB->addReg(X86::EAX);
                            MIBP.MIB->addImm(0);
                            MIBP.MIB->addReg(0);
                            MIBP.MIB->addExpr(MCConstantExpr::Create(0x0,
                                                                    *MCCtx));
                            MIBP.MIB->addReg(0);
                            i += 4;
                        }
                    } else if(op == X86::MOV8mr_NOREX ||
                                op == X86::MOV8rm_NOREX ||
                                op == X86::MOVZX32_NOREXrm8) {
                        MIBP.MIB->addReg(X86::EAX);
                        MIBP.MIB->addImm(1);
                        MIBP.MIB->addReg(X86::EAX);
                        MIBP.MIB->addExpr(MCConstantExpr::Create(0x0, *MCCtx));
                        MIBP.MIB->addReg(0);
                        i += 4;
                    } else if(opinfo.RegClass != -1) {
                        MIBP.MIB->addReg(X86MCRegisterClasses[
                                            opinfo.RegClass].getRegister(0));
                    } else {
                         MIBP.MIB->addExpr(MCConstantExpr::Create(0x8000,
                                                                     *MCCtx));
                    }
                     
                } else if(optype == MCOI::OPERAND_PCREL) {
                    MIBP.MIB->addImm(0x10);
                }
            }
        }
        return MIBP;
    }
}

//===----------------------------------------------------------------------===//
// * buildPPCMI - Returns an MIBplus containing a valid representation of the
// *              PPC instruction corresponding to the op code 'op', along with
// *              a boolean representing whether the instruction can be printed
// *              to assembly or not
MIBplus buildPPCMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op)
{
    //Initialize the MIBplus
    MIBplus MIBP;
    MIBP.asmPrintable = true;
    MIBP.MIB = NULL;

    std::string opname = MII->getName(op);
    const MCInstrDesc MID = MII->get(op);
    unsigned short sched = MID.SchedClass;
    if(RS) { *RS << opname << ":\t"; }

    //Don't make an MIB if it's a pseudo-instruction
    if(MID.isPseudo()) {
        if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
        if(US) { *US << opname << ":\tIS PSEUDO INSTRUCTION!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Don't make an MIB if PPCMCCodeEmitter won't be able to handle it
    //Every instruction that can't be encoded has a scheduling class of 0
    else if(sched == 0) {
        if(RS) { *RS << "IS UNSUPPORTED!\n"; }
        if(US) { *US << opname << ":\tIS UNSUPPORTED!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Make an MIB containing valid operands for the instruction
    else {
        if(SS) { *SS << opname << ":"; }
        MIBP.MIB = new MCInstBuilder(op);
        const MCOperandInfo *MOI = MID.OpInfo;
        MCOperandInfo opinfo;
        unsigned optype;
        unsigned numopers = MID.NumOperands;
        
        //This loop goes through and adds the correct operands to the MIB
        for(unsigned i = 0; i < numopers; i++) {
            opinfo = MOI[i];
            optype = opinfo.OperandType;
            if(optype == MCOI::OPERAND_UNKNOWN) {
                
                if(opinfo.RegClass == -1) {
                    if(opname.find("TLS") != std::string::npos) {
                        MIBP.MIB->addExpr(MCConstantExpr::Create(0x0, *MCCtx));
                    } else if((op >= PPC::MFOCRF && op <= PPC::MFOCRF8) ||
                                (op >= PPC::MTOCRF && op <= PPC::MTOCRF8)) {
                        MIBP.MIB->addReg(PPC::CR0);
                    } else {
                        MIBP.MIB->addImm(12);
                    }
                } else {
                    MIBP.MIB->addReg(PPCMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
                }
            
            } else if(optype == MCOI::OPERAND_IMMEDIATE) {
                MIBP.MIB->addImm(0);
            } else if(optype == MCOI::OPERAND_REGISTER) {
                MIBP.MIB->addReg(PPCMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
            } else if(optype == MCOI::OPERAND_MEMORY) {
                 MIBP.MIB->addExpr(MCConstantExpr::Create(0x8000, *MCCtx));
            } else if(optype == MCOI::OPERAND_PCREL) {
                MIBP.MIB->addImm(0x10);
            }
        }
        return MIBP;
    }
}

/*
//===----------------------------------------------------------------------===//
// * buildMIPSMI - Returns an MIBplus containing a valid representation of the
// *               MIPS instruction corresponding to the op code 'op', along 
// *               with a boolean representing whether the instruction can be
// *               printed to assembly or not
MIBplus buildMIPSMI(const MCInstrInfo *MII, MCContext *MCCtx, unsigned op)
{
    //Initialize the MIBplus
    MIBplus MIBP;
    MIBP.asmPrintable = true;
    MIBP.MIB = NULL;

    std::string opname = MII->getName(op);
    const MCInstrDesc MID = MII->get(op);
    uint64_t flags = MID.TSFlags;
    if(RS) { *RS << opname << ":\t"; }
    
    //Don't make an MIB if it's a pseudo-instruction
    if(MID.isPseudo()) {
        if(RS) { *RS << "IS PSEUDO INSTRUCTION!\n"; }
        if(US) { *US << opname << ":\tIS PSEUDO INSTRUCTION!\n"; }
        MIBP.asmPrintable = false;
        return MIBP;
    }
    //Make an MIB containing valid operands for the instruction
    else {
        if(SS) { *SS << opname << ":"; }
        MIBP.MIB = new MCInstBuilder(op);
        const MCOperandInfo *MOI = MID.OpInfo;
        MCOperandInfo opinfo;
        unsigned optype;
        unsigned numopers = MID.NumOperands;
        
        //This loop goes through and adds the correct operands to the MIB
        for(unsigned i = 0; i < numopers; i++) {
            opinfo = MOI[i];
            optype = opinfo.OperandType;
            if(optype == MCOI::OPERAND_UNKNOWN) {
                if(opinfo.RegClass == -1) {
                    MIBP.MIB->addImm(0);
                } else {
                    MIBP.MIB->addReg(MipsMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
                }
            } else if(optype == MCOI::OPERAND_IMMEDIATE) {
                MIBP.MIB->addImm(0);
            } else if(optype == MCOI::OPERAND_REGISTER) {
                MIBP.MIB->addReg(MipsMCRegisterClasses[
                                        opinfo.RegClass].getRegister(0));
            } else if(optype == MCOI::OPERAND_MEMORY) {
                 MIBP.MIB->addExpr(MCConstantExpr::Create(0x8000, *MCCtx));
            } else if(optype == MCOI::OPERAND_PCREL) {
                MIBP.MIB->addImm(0x10);
            }
        }
        return MIBP;
    }
}*/

//===----------------------------------------------------------------------===//
// * getTargetInstPrinter - returns the correct MCInstPrinter for the given
// *                         target
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
    } else if(triple == "powerpc64-unknown-unknown") {
        MIP = new PPCInstPrinter(*AsmInfo, *MII, *MRI, false);
        return MIP;
    /*} else if(triple == *MIPS triple*) {
        MIP = new MipsInstPrinter(*AsmInfo, *MII, *MRI);
        return MIP;*/
    } else {
        ES<<"mkAllInsts::getTargetInstPrinter: unknown triple name received\n";
        abort();
    }
}


