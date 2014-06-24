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

#include "PowerPCGenInvISel.inc"

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

  errs() << "opcode " << TargetOpc << " is coming.\n";
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
      	MMO = new MachineMemOperand(
      			MachinePointerInfo(0, 0), MachineMemOperand::MOStore, 8, 0);	// need 8bytes for ppc64
      }

      SDLoc SL(N);

    	SDValue EA = CurDAG->getNode(ISD::ADD, SL, MVT::i32, DS, RA);
    	SDValue EAext = CurDAG->getZExtOrTrunc(EA, SL, MVT::i64);
      SDValue Store = CurDAG->getStore(Chain, SL, X9, EAext, MMO);


    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Store);   //Chain
      FixChainOp(Store.getNode());


    	return NULL;
    	break;
    }
    case PPC::STDU:{

    	/*
    	 *
    	 * EA <- (RA) + EXTS(DS || 0b00)
				MEM(EA, 8) <- (RS)
				RA <- EA

				Let the effective address (EA) be the sum
				(RA)+ (DS||0b00). (RS) is stored into the doubleword in
				storage addressed by EA.
				EA is placed into register RA.
				If RA=0, the instruction form is invalid.
    	 *
    	 *
    	 */

    	SDValue Chain = N->getOperand(0);
    	SDValue X1 = N->getOperand(1);	// register x1 a.k.a. RS, 64bit
    	SDValue DS = N->getOperand(2);	// const -96, 32bit
    	//SDValue RA = N->getOperand(3);	// r1, 32bit

      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for LEAVE!\n";
      } else {
      	MMO = new MachineMemOperand(
      			MachinePointerInfo(0, 0), MachineMemOperand::MOStore, 8, 0);	// need 8bytes for ppc64
      }

      SDLoc SL(N);

    	SDValue DSext = CurDAG->getZExtOrTrunc(DS, SL, MVT::i64);
    	SDValue EA = CurDAG->getNode(ISD::ADD, SL, MVT::i64, DSext, X1);
      SDValue Store = CurDAG->getStore(Chain, SL, X1, EA, MMO);

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), SDValue(Store.getNode(), 1));   //Chain
      FixChainOp(Store.getNode());


    	return NULL;
    	break;
    }
    case PPC::STW:{
    	/*
    	 * opcode 645
    	 * if RA = 0 then b <- 0
				else b <- (RA)
				EA <- b + EXTS(D)
				MEM(EA, 4) <- (RS)32:63
				Let the effective address (EA) be the sum (RA|0)+ D.
				(RS)32:63 are stored into the word in storage addressed
				by EA.

    	 */

    	SDValue Chain = N->getOperand(0);
    	SDValue X9 = N->getOperand(1);	// register x9 a.k.a. RS
    	SDValue D = N->getOperand(2);		//
    	SDValue RA = N->getOperand(3);	// r31

      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for LEAVE!\n";
      } else {
      	MMO = new MachineMemOperand(
      			MachinePointerInfo(0, 0), MachineMemOperand::MOStore, 4, 0);	//MCO.getImm()
      }

      SDLoc SL(N);

    	SDValue EA = CurDAG->getNode(ISD::ADD, SL, MVT::i32, D, RA);

      SDValue Store = CurDAG->getStore(Chain, SL, X9, EA, MMO);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Store);   //Chain

      FixChainOp(Store.getNode());


    	return NULL;
    	break;
    }

    case PPC::B:{

      SDValue Chain = N->getOperand(0);
      uint64_t Offset = N->getConstantOperandVal(1);

      SDValue tempOffset = CurDAG->getConstant(Offset, MVT::i32);

      SDLoc SL(N);

      // Condition Code is TRUE
      SDValue Condition = CurDAG->getCondCode(ISD::SETTRUE2);
      SDValue BrNode = CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, Condition, tempOffset, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);


    	return NULL;
    	break;
    }
    case PPC::LI:{

    	/*
    	 * Load a 16-bit signed immediate value into register Rx.
				li Rx,value (equivalent to: addi Rx,0,value)

				addi:
					RT <- (RA) + EXTS(SI)
    	 */
    	SDValue RT = N->getOperand(1);	// 64bit
    	SDValue SI = N->getOperand(2);	// 32bit

      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for LEAVE!\n";
      } else {
      	MMO = new MachineMemOperand(
      			MachinePointerInfo(0, 0), MachineMemOperand::MOStore, 8, 0);	// need 8bytes for ppc64
      }

      SDLoc SL(N);

    	SDValue SISext = CurDAG->getZExtOrTrunc(SI, SL, MVT::i64);
      SDValue Store = CurDAG->getStore(RT, SL, RT, SISext, MMO);

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Store);
      FixChainOp(Store.getNode());

    	return NULL;
    	break;
    }

/*
    case PPC::RLDICL:{
    	 /*
    	  * n <- sh5 || sh0:4
				  r <- ROTL64((RS), n)
					b <- mb5 || mb0:4
					m <- MASK(b, 63)
					RA <- r & m

					The contents of register RS are rotated64 left SH bits.
					A mask is generated having 1-bits from bit MB through
					bit 63 and 0-bits elsewhere. The rotated data are
					ANDed with the generated mask and the result is
					placed into register RA.

					MASK(x, y) Mask having 1s in positions x through y
					(wrapping if x > y) and 0s elsewhere
    	 */
/*


      SDValue RS = N->getOperand(0);	// register x9
      SDValue SH = N->getOperand(1);	// const: 0
      SDValue MB = N->getOperand(2);	// const: 32

      //ISD::OR
      //ISD::ROTL, getLoad
      //ISD::OR
      //getConstant
      //ISD::AND


    	return NULL;
    	break;
    }
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
