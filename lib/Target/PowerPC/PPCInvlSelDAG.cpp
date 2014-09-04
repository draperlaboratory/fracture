//===- PPCInvISelDAG.cpp - Interface for X86 InvISelDAG  =========-*- C++ -*-=//
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

#include "Target/PowerPC/PPCInvISelDAG.h"
#include "PowerPCBaseInfo.h"

using namespace llvm;

namespace fracture {

#include "PPCGenInvISel.inc"

/*! \brief Transmogrify converts Arch specific OpCodes to LLVM IR.
 *
 *  Transmogrify is the handles Arch specific OpCodes that are not automatically
 *      supported.  This method will either emit LLVM IR or in more complicated
 *      cases call into the IR Emitter.
 */
SDNode* PPCInvISelDAG::Transmogrify(SDNode *N) {
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
    case PPC::STD:{

    	/*
    	 *
    	 * std RS,DS(RA)
    	 * if RA = 0 then b <- 0
		 	 * else b <- (RA)
		 	 * EA <- b + EXTS(DS || 0b00)
		 	 * MEM(EA, 8) <- (RS)

		 	 * Let the effective address (EA) be the sum
		 	 * (RA|0)+ (DS||0b00). (RS) is stored into the doubleword
		 	 * in storage addressed by EA.
		 	*/

    	SDValue Chain = N->getOperand(0);
    	SDValue X9 = N->getOperand(1);	// register x9 a.k.a. RS
    	SDValue DS = N->getOperand(2);	// const 48
    	SDValue RA = N->getOperand(3);	// r31

      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for LEAVE!\n";
      } else {
        MMO = *(MN->memoperands_begin());
      }

      EVT LdType = N->getValueType(0);
      SDLoc SL(N);

      /* SDValue B;  TODO: branch condition
      if (RA == 0)
      	B = CurDAG->getConstant(0x0, LdType);
      else*/
      SDValue	B = CurDAG->getLoad(LdType, SL, Chain, RA, MMO); //Load from RA

    	SDValue C1 = CurDAG->getConstant(0xb00, LdType);
    	SDValue EA_or = CurDAG->getNode(ISD::OR, SL, LdType, SDValue(B.getNode(),1), DS, C1);   // DS || 0xb00
    	// TODO: sign extend EA_or

    	SDValue EA = CurDAG->getNode(ISD::ADD, SL, LdType, SDValue(EA_or.getNode(),1), EA_or, B);

    	SDValue X9_val = CurDAG->getLoad(LdType, SL, SDValue(EA.getNode(),1), X9, MMO); //Load from X9
      SDValue Store = CurDAG->getStore(SDValue(X9_val.getNode(),1), SL, X9_val, EA, MMO);

    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), SDValue(Store.getNode(),1));   //Chain

      //For getLoad or getStore
      FixChainOp(B.getNode());
      FixChainOp(X9_val.getNode());
      FixChainOp(Store.getNode());

    	return NULL;
    	break;
    }
    	/*
    case PPC::RLDICL:


    	 * 	n <- sh5 || sh0:4
				  r <- ROTL64((RS), n)
					b <- mb5 || mb0:4
					m <- MASK(b, 63)
					RA <- r & m
    	 */


    	/*
      SDValue RS = N->getOperand(0);	// register x9
      SDValue SH = N->getOperand(1);	// const: 0
      SDValue MB = N->getOperand(2);	// const: 32



    	return NULL;
    	break;
    	*/
  }


  SDNode* TheRes = InvertCode(N);
  return TheRes;
}

/*! \brief ConvertNoRegToZero handles the NoReg input case.
 *
 *  ConvertNoRegToZero NoReg inputs were causing fracture to crash.  This
 *      method converts those cases to an i32 constant.
 */
SDValue PPCInvISelDAG::ConvertNoRegToZero(const SDValue N){
  if(N.getOpcode() == ISD::CopyFromReg ){
    const RegisterSDNode *R = dyn_cast<RegisterSDNode>(N.getOperand(1));
    if (R != NULL && R->getReg() == 0)
      return CurDAG->getConstant(4, MVT::i32);
  }
  return N;
}


}
