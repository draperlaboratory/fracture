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

    	SDValue DSext = CurDAG->getSExtOrTrunc(DS, SL, MVT::i64);
    	SDValue EA = CurDAG->getNode(ISD::ADD, SL, MVT::i64, DSext, X1);
    	SDValue DStrunc = CurDAG->getSExtOrTrunc(EA, SL, MVT::i32);

      SDValue Store = CurDAG->getStore(Chain, SL, X1, EA, MMO);

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store);
    	CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), DStrunc);   //Chain
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
    //case PPC::CMPLWI:{
    	/*
    	 * opcode: 185
    	 *
    	 * two inputs: i32 copy from reg, and constant
    	 *
    	 * cmplwi cr3,Rx,value is comparable to cmpli 3,0,Rx,value
    	 * cmpli BF,L,RA,UI
    	 *
    	 * a <- (RA)32:63
		 	 	 if a <u (UI) then c <- 0b100
		 	 	 else if a >u (480 || UI) then c <- 0b010
		 	 	 else c <- 0b001
		 	 	 CR4xBF:4xBF+3 <- c || XERSO
    	 *
    	 *
		 *The contents of register (RA)32:63 zero-extended
		 are compared with UI, treating
		 the operands as unsigned integers. The result of the
		 comparison is placed into CR field BF.
    	 */

    	//return NULL;
    //	break;
    //}
    case PPC::B:{

      SDValue Chain = N->getOperand(0);
      SDValue Offset = CurDAG->getConstant(1, MVT::i32);

      SDLoc SL(N);

      SDValue BrNode = CurDAG->getNode(ISD::BR, SL, MVT::Other, Offset, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);


    	return NULL;
    	break;
    }
    case PPC::BL:{
    	// opcode 161
    	//TODO:LR <-iea CIA + 4
    	// CIA == current instruction address
      SDValue Chain = N->getOperand(0);
      SDValue Offset = CurDAG->getConstant(1, MVT::i32);

      SDLoc SL(N);

      SDValue BrNode = CurDAG->getNode(ISD::BR, SL, MVT::Other, Offset, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);


    	return NULL;
    	break;

    }
    case PPC::gBC:{
    	/* opcode 877

    	 this is not mentioned anywhere - I am assuming it's related to branch conditional

			 4 inputs, 2 outputs: 5 i32, 1 chain

    	 bc BO,BI,target_addr

				if ¬BO2 then CTR <- CTR - 1
				ctr_ok <- BO2 | ((CTR != 0) XOR BO3)
				cond_ok <- BO0 | (CRBI === BO1) // CR is condition register
				if ctr_ok & cond_ok then
					NIA <-iea EXTS(BD || 0b00)


		 The BI field specifies the Condition Register bit to be
		 tested. The BO field is used to resolve the branch as
		 described in Figure 21. target_addr specifies the
		 branch target address.
		 If AA=0 then the branch target address is the sum of
		 BD || 0b00 sign-extended and the address of this
		 instruction, with the high-order 32 bits of the branch target
		 address set to 0 in 32-bit mode.
    	*/

      SDValue Chain = N->getOperand(0);
      SDValue Op1 = N->getOperand(1); // const 4 - branch if false
      // page 6 of http://cache.freescale.com/files/32bit/doc/app_note/AN2491.pdf
      SDValue Op2 = N->getOperand(2); // cond code
      uint64_t Op3 = N->getConstantOperandVal(3);
      																// const * 4 for branch target offset
      																// always multiply by 4.

      SDValue BranchTarget = CurDAG->getConstant(Op3 * 4, MVT::i32);

      //SDValue Op4 = N->getOperand(4); // CTR register? implicit use/def
      ///SDValue Op5 = N->getOperand(5); // RM register? implicit use



      uint64_t TarVal = N->getConstantOperandVal(1);	// this index was arbitrary... didn't compile with 1
      SDValue tempEIP = CurDAG->getConstant(TarVal, MVT::i32);
      //SDValue EFLAGSCFR = N->getOperand(2);


      SDLoc SL(N);
      // Calculate the Branch Target
      SDValue TempEIPval = CurDAG->getConstant(cast<ConstantSDNode>(tempEIP)->getZExtValue(), tempEIP.getValueType());

      // Condition Code is "Equal", for no good reason
      SDValue Condition = CurDAG->getCondCode(ISD::SETEQ);
      SDValue BrNode = CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, Condition, TempEIPval, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);

    	return NULL;
    	break;

    }
    case PPC::RLDICL:{
    	 /*
    	  * Rotate Left Double Word Immediate then Clear Left
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



      SDValue RS = N->getOperand(0);	// register x9
      SDValue SH = N->getOperand(1);	// const: 0
      //SDValue MB = N->getOperand(2);	// const: 32
      SDLoc SL(N);

      SDValue R = CurDAG->getNode(ISD::ROTL, SL, MVT::i64, RS, SH);

      uint64_t MBVal = N->getConstantOperandVal(2);	// get value of MB
      // TODO: MASK(x, y) Mask having 1s in positions x through y (wrapping if x > y)
      //build bitmask
      uint64_t C1 = 0;
      for (uint64_t i = 0; i < MBVal; ++i) {
      		C1 += 1ULL << i;
      }
      uint64_t Shift = 64 - MBVal;
      C1 = C1 << Shift;
      // if MBVal == 32, Windows Calculator won't convert decimal->binary
      // outputs correctly for unsigned ints.

      SDValue M = CurDAG->getConstant(C1, MVT::i64);

      SDValue RA = CurDAG->getNode(ISD::AND, SL, MVT::i64, R, M);
      //ISD::OR
      //ISD::ROTL
      //ISD::OR
      //getConstant
      //ISD::AND

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), RA);


    	return NULL;
    	break;
    }

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
