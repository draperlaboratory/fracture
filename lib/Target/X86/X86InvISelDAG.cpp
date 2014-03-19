//===- X86InvISelDAG.cpp - Interface for X86 InvISelDAG  =========-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// This file takes code found in LLVM and modifies it.
//
//===----------------------------------------------------------------------===//
//
// Provides inverse instruction selector functionality for the ARM targets.
//
//===----------------------------------------------------------------------===//

#include "Target/X86/X86InvISelDAG.h"
#include "X86BaseInfo.h"

using namespace llvm;

namespace fracture {

#include "X86GenInvISel.inc"

SDNode* X86InvISelDAG::Transmogrify(SDNode *N) {
  // Insert fixups here
  if (!N->isMachineOpcode()) {
    // Drop noreg registers
    if (N->getOpcode() == ISD::CopyFromReg) {
      const RegisterSDNode *R = dyn_cast<RegisterSDNode>(N->getOperand(1));
      if (R != NULL && R->getReg() == 0) {
        SDValue Tmp = CurDAG->getUNDEF(R->getValueType(0));
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Tmp);
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), N->getOperand(0));
      }
    }

    // FIXME: This is wrong, originally it returns NULL.
    // I think we need to add it to the CurDAG (only if it hasn't
    // already been added to the CurDAG...)
    return N;                // already selected
  }

  uint16_t TargetOpc = N->getMachineOpcode();
  switch(TargetOpc) {
    default:
    	outs() << "TargetOpc: " << TargetOpc << "\n";
    	break;
    case X86::JBE_1:{
    			SDValue Chain = N->getOperand(0);
          SDValue Target = N->getOperand(1);
          SDValue EFLAGSCFR = N->getOperand(2);
          SDLoc SL(N);

          // Calculate the Branch Target
          SDValue BT = CurDAG->getConstant(
            cast<ConstantSDNode>(Target)->getZExtValue(), Target.getValueType());

          // Condition Code is "Below or Equal" <=
          SDValue CC = CurDAG->getCondCode(ISD::SETLE);

          SDValue BrNode =
            CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, CC, BT, Chain);
          CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);
          return NULL;
          break;
    }
    case X86::CMP32mi8:{
    	// NOTE: Pattern in ARM DAG Selector is busted as not handling CPSR
    	//       not sure how to fix, so this might just be a hack!
    	// Pattern: (CMPri:int32 GPR:$Rn, (imm:i32):$i, pred:$p, pred:%noreg)
    	// Emits: (ARMcmp GPR:$Rn, (imm:i32):$i)
    	unsigned Opc = X86ISD::CMP;
    	SDValue N0 = N->getOperand(1);
    	SDValue N1 = N->getOperand(2);
    	SDValue ResNode = CurDAG->getNode(Opc, SDLoc(N), MVT::Other, N0, N1);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), ResNode);
    	return NULL;
    	break;
    }
    case X86::PUSH32r:{
    	//Subtract 4 from the stack
    	//Move the new value into place

    	//Get the arguments
    	SDValue Chain = N->getOperand(0);
    	SDValue Val = N->getOperand(1);	//Arg passes to push
    	SDValue ESP = N->getOperand(2);

    	SDLoc SL(N);
    	SDVTList AddVTList = CurDAG->getVTList(MVT::i32);

    	//Subtract
    	SDValue Width = CurDAG->getTargetConstant(4, MVT::i32);//get constant
    	SDValue Addr = CurDAG->getNode(ISD::SUB, SL, AddVTList, ESP, Width);	//ESP -= 4;
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Addr);

    	const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
    	MachineMemOperand *MMO = NULL;
    	if (MN->memoperands_empty()) {
    		errs() << "NO MACHINE OPS for STR_PRE_IMM!\n";
    	} else {
    		MMO = *(MN->memoperands_begin());
    	}

    	//Store
    	SDValue Store = CurDAG->getStore(Chain, SL, Val, Addr, MMO);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store);

    	return NULL;
    	break;
    }
    /*
    case X86::MOV64rr:{
    	SDValue Chain = N->getOperand(0);
    	//SDValue Target = N->getOperand(1);
    	SDLoc SL(N);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Chain);

    	return NULL;
    	break;
    }
    */
  }

  SDNode* TheRes = InvertCode(N);
  return TheRes;
}

}
