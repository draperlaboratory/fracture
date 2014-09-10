//===- ARMInvISelDAG.cpp - Interface for ARM InvISelDAG  =========-*- C++ -*-=//
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

#include "Target/ARM/ARMInvISelDAG.h"
#include "ARMBaseInfo.h"

using namespace llvm;

namespace fracture {

#include "ARMGenInvISel.inc"


//Coppied these from https://github.com/llvm-mirror/llvm/blob/f65712bfe35a038e5895ffc859bcf43fda35a8fd/lib/Target/ARM/MCTargetDesc/ARMAddressingModes.h#L413
//static inline unsigned getAM2Offset(unsigned AM2Opc) {
//  return AM2Opc & ((1 << 12)-1);
//}
//static inline unsigned getAM2Op(unsigned AM2Opc) {
//  return ((AM2Opc >> 12) & 1) ? 1 : 0;
//}
//static inline unsigned getAM2ShiftOpc(unsigned AM2Opc) {
//  return ((AM2Opc >> 13) & 7);
//}
//static inline unsigned getAM2IdxMode(unsigned AM2Opc) {
//  return (AM2Opc >> 16);
//}



SDNode* ARMInvISelDAG::Transmogrify(SDNode *N) {
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
  //outs() << "Next Opc: " << TargetOpc << "\n";

  switch(TargetOpc) {
    default:
        // outs() << "To tablegen Opc: " << TargetOpc << "\n";
    	break;
    case ARM::CMPrr:
    case ARM::CMPri: {
      // NOTE: Pattern in ARM DAG Selector is busted as not handling CPSR
      //       not sure how to fix, so this might just be a hack!
      // Pattern: (CMPri:int32 GPR:$Rn, (imm:i32):$i, pred:$p, pred:%noreg)
      // Emits: (ARMcmp GPR:$Rn, (imm:i32):$i)
      unsigned Opc = ARMISD::CMP;
      SDValue N0 = N->getOperand(0);
      SDValue N1 = N->getOperand(1);
      SDValue ResNode = CurDAG->getNode(Opc, SDLoc(N), MVT::i32, N0, N1);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), ResNode);
      return NULL;
    }
    case ARM::t2Bcc:
    case ARM::tBcc:
    case ARM::Bcc: {
//       Pattern: (Bcc:void (bb:Other):$dst, (imm:i32):$cc)
//       Emits: (ARMbrcond:void (bb:Other):$dst, (imm:i32):$cc)
//
//       Pattern: (tBcc:void (bb:Other):$dst, (imm:i32):$cc)
//       Emits: (ARMbrcond:void (bb:Other):$dst, (imm:i32):$cc)
//
//       Pattern: (t2Bcc:void (bb:Other):$dst, (imm:i32):$cc)
//       Emits: (ARMbrcond:void (bb:Other):$dst, (imm:i32):$cc)
      unsigned Opc = ARMISD::BRCOND;
      // Bcc is a control flow instruction, therefore
      // the first operand is always the pointer to the prev inst node.
      SDValue Chain = N->getOperand(0);
      SDValue N1 = N->getOperand(1); // Branch target
      // FIXME: May want to interpret the pred value, we ignore for now.
      SDValue ARMcc = N->getOperand(2); // pred, 14 for unconditional
      SDValue Cmp = N->getOperand(3); // CPSR, or %noreg for unconditional

      SDValue BT = CurDAG->getConstant(
        cast<ConstantSDNode>(N1)->getZExtValue(), N1.getValueType());


      // From ARMISelLowering.cpp ---
      // return DAG.getNode(ARMISD::BRCOND, dl, MVT::Other,
      //                    Chain, Dest, ARMcc, CCR, Cmp);
      // We don't get that, we get Chain, Dest (BT), ARMcc, Cmp
      // Chain flips to the end by convention (no other reason).
      SDValue ResNode = CurDAG->getNode(Opc, SDLoc(N), MVT::Other,
        BT, ARMcc, Cmp, Chain);

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0),
        SDValue(ResNode.getNode(), ResNode.getResNo()));

      return NULL;
    }
      break;
    case ARM::RSBrr: {
      // Pattern: (RSBrr GPR:$Rn, imm:op2, pred:$p)
      // Emits: (sub op2, $Rn)
      SDValue Op1 = N->getOperand(0);
      SDValue Op2 = N->getOperand(1);
      SDLoc SL(N);
      SDValue Sub = CurDAG->getNode(ISD::SUB, SL, MVT::i32, Op2, Op1);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Sub);
      return NULL;
    }
      break;
    case ARM::STR_PRE_IMM: {
      // Pattern: (STR_PRE_IMM GPR:$Rt, GPR:$Rb, imm:offset, pred:$p)
      // Emits: (store $Rt, (add $Rb, imm:offset)).
      // Note also that this variation has an outs to $Rb for the add op
      // This is one variation of a complex instruction:
      //  http://www.heyrick.co.uk/assembler/str.html#str
      SDValue Chain = N->getOperand(0);
      SDValue Tgt = N->getOperand(1);
      SDValue Base = N->getOperand(2);
      SDValue Offset = N->getOperand(3);

      SDLoc SL(N);
      SDVTList AddVTList = CurDAG->getVTList(MVT::i32);

      SDValue Addr = CurDAG->getNode(ISD::ADD, SL, AddVTList, Base, Offset);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Addr);

      // memops might be null here, but not sure if we need to check.
      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for STR_PRE_IMM!\n";
      } else {
        MMO = *(MN->memoperands_begin());
      }
      SDValue Store = CurDAG->getStore(Chain, SL, Tgt, Addr, MMO);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store);

      return NULL;
    }
    case ARM::STRH_POST:
    case ARM::STRD_POST: {
      // Store register , decrement, post index
      // mem[Rn+Rm/#imm] = Rd (32 bit copy)
   	  //
      SDValue Chain = N->getOperand(0);
      SDValue Tgt1 = N->getOperand(1);
      SDValue Tgt2 = N->getOperand(2);
      SDValue Base = N->getOperand(3);
      SDValue Offset = N->getOperand(4);

      SDLoc SL(N);
      SDVTList AddVTList = CurDAG->getVTList(MVT::i32);

      SDValue Addr1 = CurDAG->getNode(ISD::SUB, SL, AddVTList, Base, Offset);

      // memops might be null here, but not sure if we need to check.
      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
    	  errs() << "NO MACHINE OPS for STRD_POST!\n";
         }
      else {
           MMO = *(MN->memoperands_begin());
         }
      SDValue Four = CurDAG->getConstant(4, MVT::i32, false, false);
      SDValue Store = CurDAG->getStore(Chain, SL, Tgt1, Addr1, MMO);
      SDValue Addr2 = CurDAG->getNode(ISD::ADD, SL, AddVTList, Addr1, Four);
      SDValue Store2 = CurDAG->getStore(Store, SL, Tgt2, Addr2, MMO);
      SDValue Addr3 = CurDAG->getNode(ISD::ADD, SL, AddVTList, Addr2, Four);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Addr3);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store2);
      FixChainOp(Store.getNode());
      FixChainOp(Store2.getNode());

         return NULL;
       }


    // case ARM::LDRD_POST: {
    //   // Load, increment, post index
    //   // RD = mem[Rn+Rm/#imm]
   	//   //
    //   SDValue Chain = N->getOperand(0);
    //   SDValue Op1 = N->getOperand(1);
    //   SDValue Op2 = N->getOperand(2);
    //   SDValue Base = N->getOperand(3);
    //   SDValue Offset = N->getOperand(4);

    //   SDLoc SL(N);
    //   SDVTList AddVTList = CurDAG->getVTList(MVT::i32);

    //   SDValue Addr1 = CurDAG->getNode(ISD::SUB, SL, AddVTList, Base, Offset);

    //   // memops might be null here, but not sure if we need to check.
    //   const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
    //   MachineMemOperand *MMO = NULL;
    //   if (MN->memoperands_empty()) {
    // 	  errs() << "NO MACHINE OPS for STRD_POST!\n";
    //      }
    //   else {
    //        MMO = *(MN->memoperands_begin());
    //      }
    //   SDValue Four = CurDAG->getConstant(4, MVT::i32, false, false);
    //   SDValue Store = CurDAG->getStore(Chain, SL, Tgt1, Addr1, MMO);
    //   SDValue Addr2 = CurDAG->getNode(ISD::ADD, SL, AddVTList, Addr1, Four);
    //   SDValue Store2 = CurDAG->getStore(Store, SL, Tgt2, Addr2, MMO);
    //   SDValue Addr3 = CurDAG->getNode(ISD::ADD, SL, AddVTList, Addr2, Four);
    //   CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Addr3);
    //   CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Store2);
    //   FixChainOp(Store.getNode());
    //   FixChainOp(Store2.getNode());

    //      return NULL;
    //    }
    // }
 
//    case ARM::LDRi12: {
//      //load the Ptr
//      //ldr chain [ptr offset]
//
//      SDValue Chain 	  = N->getOperand(0);
//      SDValue Ptr       = N->getOperand(1);
//      SDValue Offset    = N->getOperand(2);
//      SDLoc SL(N);
//  	  SDVTList VTList = CurDAG->getVTList(MVT::i32);
//
//
//  	  EVT LdType = N->getValueType(0);
//
//  	  SDValue Addr = CurDAG->getNode(ISD::ADD, SL, VTList, Ptr, Offset);
//   	  SDValue Ldr = CurDAG->getLoad(LdType, SL, Chain, Addr,
//          	        MachinePointerInfo::getConstantPool(), false, false, true, 0, 0);
//      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ldr);
//      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), SDValue(Ldr.getNode(),1));
//      FixChainOp(Ldr.getNode());
//
//          	return NULL;
//      }
    case ARM::STRi12: {
      //store the tgt filled with the the stack space indicated by  base + offset
      //str Tgt [base offset]

      SDValue Chain = N->getOperand(0);
      SDValue Tgt = N->getOperand(1);
      SDValue Base = N->getOperand(2);
      SDValue Offset = N->getOperand(3);

      SDLoc SL(N);
      SDVTList AddVTList = CurDAG->getVTList(MVT::i32);
      SDValue Addr = CurDAG->getNode(ISD::ADD, SL, AddVTList, Base, Offset);


      // memops might be null here, but not sure if we need to check.
      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for STRi12!\n";
    	    } else {
    	      MMO = *(MN->memoperands_begin());
    	    }
      SDValue Store = CurDAG->getStore(Chain, SL, Tgt, Addr, MMO);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Store);
      FixChainOp(Store.getNode());
	return NULL;
      }
    case ARM::LDMIA:            // Load variations...
                            //   LD?  Inc?   Bef?    WB?
      InvLoadOrStoreMultiple(N, true, true, false, false);
      return NULL;
    case ARM::LDMIB:
      InvLoadOrStoreMultiple(N, true, true, true, false);
      return NULL;
    case ARM::LDMDA:
      InvLoadOrStoreMultiple(N, true, false, false, false);
      return NULL;
    case ARM::LDMDB:
      InvLoadOrStoreMultiple(N, true, false, true, false);
      return NULL;
    case ARM::LDMIA_UPD:        // writeback variations
      InvLoadOrStoreMultiple(N, true, true, false, true);
      return NULL;
    case ARM::LDMIB_UPD:
      InvLoadOrStoreMultiple(N, true, true, true, true);
      return NULL;
    case ARM::LDMDA_UPD:
      InvLoadOrStoreMultiple(N, true, false, false, true);
      return NULL;
    case ARM::LDMDB_UPD:
      InvLoadOrStoreMultiple(N, true, false, true, true);
      return NULL;
    case ARM::STMIA:            // store variations...
      InvLoadOrStoreMultiple(N, false, true, false, false);
      return NULL;
    case ARM::STMIB:
      InvLoadOrStoreMultiple(N, false, true, true, false);
      return NULL;
    case ARM::STMDA:
      InvLoadOrStoreMultiple(N, false, false, false, false);
      return NULL;
    case ARM::STMDB:
      InvLoadOrStoreMultiple(N, false, false, true, false);
      return NULL;
    case ARM::STMIA_UPD:        // increment after, writeback
      InvLoadOrStoreMultiple(N, false, true, false, true);
      return NULL;
    case ARM::STMIB_UPD:        // increment before, write back
      InvLoadOrStoreMultiple(N, false, true, true, true);
      return NULL;
    case ARM::STMDA_UPD:        // Decrement After, write back
      InvLoadOrStoreMultiple(N, false, false, false, true);
      return NULL;
    case ARM::STMDB_UPD:        // Decrement before, write back
      InvLoadOrStoreMultiple(N, false, false, true, true);
      return NULL;
    case ARM::MOVr:
       case ARM::MOVi: {
         // Note: Cannot use dummy arithmetic here because it will get folded
         // (removed) from the DAG. Instead we search for a CopyToReg, if it
         // exists we set it's debug location to the Mov, and if it doesn't we
         // print an error and do nothing.
         SDNode *C2R = NULL;
         for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E;
              ++I) {
           if (I->getOpcode() == ISD::CopyToReg) {
             C2R = *I;
             break;
           }
         }
         assert(C2R && "Move instruction without CopytoReg!");
         C2R->setDebugLoc(N->getDebugLoc());
         CurDAG->ReplaceAllUsesOfValueWith(SDValue(N,0), N->getOperand(0));
         return NULL;
         break;
       }
    case ARM::BL:
    case ARM::BLX:
      //missing open bracket {
      SDValue Chain = N->getOperand(0);
      SDValue Offset = N->getOperand(1);
      SDLoc SL(N);
      SDVTList VTList = CurDAG->getVTList(MVT::i32, MVT::Other);
      SDValue CallNode =
      CurDAG->getNode(ARMISD::CALL, SL, VTList, Offset, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0),
      SDValue(CallNode.getNode(), 0));
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1),
      SDValue(CallNode.getNode(), 1));
      return NULL;
  }


  //If Transmogrify fails to find the opcode then we will send it to the
  //tablegen file to search for a match. If this fails, then fracture will
  // crash with a debug code.
  SDNode* TheRes = InvertCode(N);
  return TheRes;
}

bool ARMInvISelDAG::SelectImmShifterOperand(SDValue N,
                                              SDValue &BaseReg,
                                              SDValue &Opc,
                                              bool CheckProfitability) {
  // ShiftOpc ShOpcVal = N.getOpcode());

  // // Don't match base register only case. That is matched to a separate
  // // lower complexity pattern with explicit register operand.
  // if (ShOpcVal == ARM_AM::no_shift) return false;

  // BaseReg = N.getOperand(0);
  // unsigned ShImmVal = 0;
  // ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1));
  // if (!RHS) return false;
  // ShImmVal = RHS->getZExtValue() & 31;
  // Opc = CurDAG->getTargetConstant(ARM_AM::getSORegOpc(ShOpcVal, ShImmVal),
  //                                 MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectRegShifterOperand(SDValue N,
                                              SDValue &BaseReg,
                                              SDValue &ShReg,
                                              SDValue &Opc,
                                              bool CheckProfitability) {
  // ARM_AM::ShiftOpc ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOpcode());

  // // Don't match base register only case. That is matched to a separate
  // // lower complexity pattern with explicit register operand.
  // if (ShOpcVal == ARM_AM::no_shift) return false;

  // BaseReg = N.getOperand(0);
  // unsigned ShImmVal = 0;
  // ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1));
  // if (RHS) return false;

  // ShReg = N.getOperand(1);
  // if (CheckProfitability && !isShifterOpProfitable(N, ShOpcVal, ShImmVal))
  //   return false;
  // Opc = CurDAG->getTargetConstant(ARM_AM::getSORegOpc(ShOpcVal, ShImmVal),
  //                                 MVT::i32);
  return true;
}


bool ARMInvISelDAG::SelectAddrModeImm12(SDValue N,
                                          SDValue &Base,
                                          SDValue &OffImm) {
  // N is the first operand, but needs to be turned into ISD::ADD
  // Base should be the same as N
  // OffImm should be the Immediate offset
  assert(N.getNode() == Base.getNode()
    && "Node to replace does not match base!");

  Base = CurDAG->getNode(ISD::ADD, SDLoc(N.getNode()),
    N.getValueType(), Base, OffImm);

  // AddNode->dump();

  // Base only.
  // if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
  //     !CurDAG->isBaseWithConstantOffset(N)) {
  //   if (N.getOpcode() == ISD::FrameIndex) {
  //     // Match frame index.
  //     int FI = cast<FrameIndexSDNode>(N)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
  //     OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
  //     return true;
  //   }

  //   // if (N.getOpcode() == ARMISD::Wrapper &&
  //   //     !(Subtarget->useMovt() &&
  //   //                  N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
  //   //   Base = N.getOperand(0);
  //   // } else
  //     Base = N;
  //   OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
  //   return true;
  // }

  // if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //   int RHSC = (int)RHS->getZExtValue();
  //   if (N.getOpcode() == ISD::SUB)
  //     RHSC = -RHSC;

  //   if (RHSC >= 0 && RHSC < 0x1000) { // 12 bits (unsigned)
  //     Base   = N.getOperand(0);
  //     if (Base.getOpcode() == ISD::FrameIndex) {
  //       int FI = cast<FrameIndexSDNode>(Base)->getIndex();
  //       Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
  //     }
  //     OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
  //     return true;
  //   }
  // }

  // // Base only.
  // Base = N;
  // OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}



bool ARMInvISelDAG::SelectLdStSOReg(SDValue N, SDValue &Base, SDValue &Offset,
                                      SDValue &Opc) {
  // ARMSubTarget Subtarget = TLI->getSubTarget();
  // if (N.getOpcode() == ISD::MUL
  //   // &&
  //   //   ((!Subtarget->isLikeA9() && !Subtarget->isSwift()) || N.hasOneUse())
  //     ) {
  //   if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //     // X * [3,5,9] -> X + X * [2,4,8] etc.
  //     int RHSC = (int)RHS->getZExtValue();
  //     if (RHSC & 1) {
  //       RHSC = RHSC & ~1;
  //       ARM_AM::AddrOpc AddSub = ARM_AM::add;
  //       if (RHSC < 0) {
  //         AddSub = ARM_AM::sub;
  //         RHSC = - RHSC;
  //       }
  //       if (isPowerOf2_32(RHSC)) {
  //         unsigned ShAmt = Log2_32(RHSC);
  //         Base = Offset = N.getOperand(0);
  //         Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, ShAmt,
  //                                                           ARM_AM::lsl),
  //                                         MVT::i32);
  //         return true;
  //       }
  //     }
  //   }
  // }

  // if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
  //     // ISD::OR that is equivalent to an ISD::ADD.
  //     !CurDAG->isBaseWithConstantOffset(N))
  //   return false;

  //   // Leave simple R +/- imm12 operands for LDRi12
  // if (N.getOpcode() == ISD::ADD || N.getOpcode() == ISD::OR) {
  //   int RHSC;
  //   if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/1,
  //                               -0x1000+1, 0x1000, RHSC)) // 12 bits.
  //     return false;
  // }

  // // Otherwise this is R +/- [possibly shifted] R.
  // ARM_AM::AddrOpc AddSub = N.getOpcode() == ISD::SUB ? ARM_AM::sub:ARM_AM::add;
  // ARM_AM::ShiftOpc ShOpcVal =
  //   ARM_AM::getShiftOpcForNode(N.getOperand(1).getOpcode());
  // unsigned ShAmt = 0;

  // Base   = N.getOperand(0);
  // Offset = N.getOperand(1);

  // if (ShOpcVal != ARM_AM::no_shift) {
  //   // Check to see if the RHS of the shift is a constant, if not, we can't fold
  //   // it.
  //   if (ConstantSDNode *Sh =
  //          dyn_cast<ConstantSDNode>(N.getOperand(1).getOperand(1))) {
  //     ShAmt = Sh->getZExtValue();
  //     if (isShifterOpProfitable(Offset, ShOpcVal, ShAmt))
  //       Offset = N.getOperand(1).getOperand(0);
  //     else {
  //       ShAmt = 0;
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   } else {
  //     ShOpcVal = ARM_AM::no_shift;
  //   }
  // }

  // // Try matching (R shl C) + (R).
  // if (N.getOpcode() != ISD::SUB && ShOpcVal == ARM_AM::no_shift &&
  //     !(Subtarget->isLikeA9() || Subtarget->isSwift() ||
  //       N.getOperand(0).hasOneUse())) {
  //   ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOperand(0).getOpcode());
  //   if (ShOpcVal != ARM_AM::no_shift) {
  //     // Check to see if the RHS of the shift is a constant, if not, we can't
  //     // fold it.
  //     if (ConstantSDNode *Sh =
  //         dyn_cast<ConstantSDNode>(N.getOperand(0).getOperand(1))) {
  //       ShAmt = Sh->getZExtValue();
  //       if (isShifterOpProfitable(N.getOperand(0), ShOpcVal, ShAmt)) {
  //         Offset = N.getOperand(0).getOperand(0);
  //         Base = N.getOperand(1);
  //       } else {
  //         ShAmt = 0;
  //         ShOpcVal = ARM_AM::no_shift;
  //       }
  //     } else {
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   }
  // }

  // Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, ShAmt, ShOpcVal),
  //                                 MVT::i32);
  return true;
}


//-----

AddrMode2Type ARMInvISelDAG::SelectAddrMode2Worker(SDValue N,
                                                     SDValue &Base,
                                                     SDValue &Offset,
                                                     SDValue &Opc) {
  // if (N.getOpcode() == ISD::MUL &&
  //     (!(Subtarget->isLikeA9() || Subtarget->isSwift()) || N.hasOneUse())) {
  //   if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //     // X * [3,5,9] -> X + X * [2,4,8] etc.
  //     int RHSC = (int)RHS->getZExtValue();
  //     if (RHSC & 1) {
  //       RHSC = RHSC & ~1;
  //       ARM_AM::AddrOpc AddSub = ARM_AM::add;
  //       if (RHSC < 0) {
  //         AddSub = ARM_AM::sub;
  //         RHSC = - RHSC;
  //       }
  //       if (isPowerOf2_32(RHSC)) {
  //         unsigned ShAmt = Log2_32(RHSC);
  //         Base = Offset = N.getOperand(0);
  //         Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, ShAmt,
  //                                                           ARM_AM::lsl),
  //                                         MVT::i32);
  //         return AM2_SHOP;
  //       }
  //     }
  //   }
  // }

  // if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
  //     // ISD::OR that is equivalent to an ADD.
  //     !CurDAG->isBaseWithConstantOffset(N)) {
  //   Base = N;
  //   if (N.getOpcode() == ISD::FrameIndex) {
  //     int FI = cast<FrameIndexSDNode>(N)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //   } else if (N.getOpcode() == ARMISD::Wrapper &&
  //              !(Subtarget->useMovt() &&
  //                N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
  //     Base = N.getOperand(0);
  //   }
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(ARM_AM::add, 0,
  //                                                     ARM_AM::no_shift),
  //                                   MVT::i32);
  //   return AM2_BASE;
  // }

  // // Match simple R +/- imm12 operands.
  // if (N.getOpcode() != ISD::SUB) {
  //   int RHSC;
  //   if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/1,
  //                               -0x1000+1, 0x1000, RHSC)) { // 12 bits.
  //     Base = N.getOperand(0);
  //     if (Base.getOpcode() == ISD::FrameIndex) {
  //       int FI = cast<FrameIndexSDNode>(Base)->getIndex();
  //       Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //     }
  //     Offset = CurDAG->getRegister(0, MVT::i32);

  //     ARM_AM::AddrOpc AddSub = ARM_AM::add;
  //     if (RHSC < 0) {
  //       AddSub = ARM_AM::sub;
  //       RHSC = - RHSC;
  //     }
  //     Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, RHSC,
  //                                                       ARM_AM::no_shift),
  //                                     MVT::i32);
  //     return AM2_BASE;
  //   }
  // }

  // if ((Subtarget->isLikeA9() || Subtarget->isSwift()) && !N.hasOneUse()) {
  //   // Compute R +/- (R << N) and reuse it.
  //   Base = N;
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(ARM_AM::add, 0,
  //                                                     ARM_AM::no_shift),
  //                                   MVT::i32);
  //   return AM2_BASE;
  // }

  // // Otherwise this is R +/- [possibly shifted] R.
  // ARM_AM::AddrOpc AddSub = N.getOpcode() != ISD::SUB ? ARM_AM::add:ARM_AM::sub;
  // ARM_AM::ShiftOpc ShOpcVal =
  //   ARM_AM::getShiftOpcForNode(N.getOperand(1).getOpcode());
  // unsigned ShAmt = 0;

  // Base   = N.getOperand(0);
  // Offset = N.getOperand(1);

  // if (ShOpcVal != ARM_AM::no_shift) {
  //   // Check to see if the RHS of the shift is a constant, if not, we can't fold
  //   // it.
  //   if (ConstantSDNode *Sh =
  //          dyn_cast<ConstantSDNode>(N.getOperand(1).getOperand(1))) {
  //     ShAmt = Sh->getZExtValue();
  //     if (isShifterOpProfitable(Offset, ShOpcVal, ShAmt))
  //       Offset = N.getOperand(1).getOperand(0);
  //     else {
  //       ShAmt = 0;
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   } else {
  //     ShOpcVal = ARM_AM::no_shift;
  //   }
  // }

  // // Try matching (R shl C) + (R).
  // if (N.getOpcode() != ISD::SUB && ShOpcVal == ARM_AM::no_shift &&
  //     !(Subtarget->isLikeA9() || Subtarget->isSwift() ||
  //       N.getOperand(0).hasOneUse())) {
  //   ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOperand(0).getOpcode());
  //   if (ShOpcVal != ARM_AM::no_shift) {
  //     // Check to see if the RHS of the shift is a constant, if not, we can't
  //     // fold it.
  //     if (ConstantSDNode *Sh =
  //         dyn_cast<ConstantSDNode>(N.getOperand(0).getOperand(1))) {
  //       ShAmt = Sh->getZExtValue();
  //       if (isShifterOpProfitable(N.getOperand(0), ShOpcVal, ShAmt)) {
  //         Offset = N.getOperand(0).getOperand(0);
  //         Base = N.getOperand(1);
  //       } else {
  //         ShAmt = 0;
  //         ShOpcVal = ARM_AM::no_shift;
  //       }
  //     } else {
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   }
  // }

  // Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, ShAmt, ShOpcVal),
  //                                 MVT::i32);
  return AM2_SHOP;
}

bool ARMInvISelDAG::SelectAddrMode2OffsetReg(SDNode *Op, SDValue N,
                                            SDValue &Offset, SDValue &Opc) {
  // unsigned Opcode = Op->getOpcode();
  // ISD::MemIndexedMode AM = (Opcode == ISD::LOAD)
  //   ? cast<LoadSDNode>(Op)->getAddressingMode()
  //   : cast<StoreSDNode>(Op)->getAddressingMode();
  // ARM_AM::AddrOpc AddSub = (AM == ISD::PRE_INC || AM == ISD::POST_INC)
  //   ? ARM_AM::add : ARM_AM::sub;
  // int Val;
  // if (isScaledConstantInRange(N, /*Scale=*/1, 0, 0x1000, Val))
  //   return false;

  // Offset = N;
  // ARM_AM::ShiftOpc ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOpcode());
  // unsigned ShAmt = 0;
  // if (ShOpcVal != ARM_AM::no_shift) {
  //   // Check to see if the RHS of the shift is a constant, if not, we can't fold
  //   // it.
  //   if (ConstantSDNode *Sh = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //     ShAmt = Sh->getZExtValue();
  //     if (isShifterOpProfitable(N, ShOpcVal, ShAmt))
  //       Offset = N.getOperand(0);
  //     else {
  //       ShAmt = 0;
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   } else {
  //     ShOpcVal = ARM_AM::no_shift;
  //   }
  // }

  // Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, ShAmt, ShOpcVal),
  //                                 MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectAddrMode2OffsetImmPre(SDNode *Op, SDValue N,
                                            SDValue &Offset, SDValue &Opc) {
  // unsigned Opcode = Op->getOpcode();
  // ISD::MemIndexedMode AM = (Opcode == ISD::LOAD)
  //   ? cast<LoadSDNode>(Op)->getAddressingMode()
  //   : cast<StoreSDNode>(Op)->getAddressingMode();
  // ARM_AM::AddrOpc AddSub = (AM == ISD::PRE_INC || AM == ISD::POST_INC)
  //   ? ARM_AM::add : ARM_AM::sub;
  // int Val;
  // if (isScaledConstantInRange(N, /*Scale=*/1, 0, 0x1000, Val)) { // 12 bits.
  //   if (AddSub == ARM_AM::sub) Val *= -1;
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(Val, MVT::i32);
  //   return true;
  // }

  return false;
}



bool ARMInvISelDAG::SelectAddrMode2OffsetImm(SDNode *Op, SDValue N,
                                            SDValue &Offset, SDValue &Opc) {
  // unsigned Opcode = Op->getOpcode();
  // ISD::MemIndexedMode AM = (Opcode == ISD::LOAD)
  //   ? cast<LoadSDNode>(Op)->getAddressingMode()
  //   : cast<StoreSDNode>(Op)->getAddressingMode();
  // ARM_AM::AddrOpc AddSub = (AM == ISD::PRE_INC || AM == ISD::POST_INC)
  //   ? ARM_AM::add : ARM_AM::sub;
  // int Val;
  // if (isScaledConstantInRange(N, /*Scale=*/1, 0, 0x1000, Val)) { // 12 bits.
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM2Opc(AddSub, Val,
  //                                                     ARM_AM::no_shift),
  //                                   MVT::i32);
  //   return true;
  // }



  return false;
}

bool ARMInvISelDAG::SelectAddrOffsetNone(SDValue N, SDValue &Base) {
  Base = N;
  return true;
}

bool ARMInvISelDAG::SelectAddrMode3(SDValue N,
                                      SDValue &Base, SDValue &Offset,
                                      SDValue &Opc) {
  // if (N.getOpcode() == ISD::SUB) {
  //   // X - C  is canonicalize to X + -C, no need to handle it here.
  //   Base = N.getOperand(0);
  //   Offset = N.getOperand(1);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(ARM_AM::sub, 0),MVT::i32);
  //   return true;
  // }

  // if (!CurDAG->isBaseWithConstantOffset(N)) {
  //   Base = N;
  //   if (N.getOpcode() == ISD::FrameIndex) {
  //     int FI = cast<FrameIndexSDNode>(N)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //   }
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(ARM_AM::add, 0),MVT::i32);
  //   return true;
  // }

  // // If the RHS is +/- imm8, fold into addr mode.
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/1,
  //                             -256 + 1, 256, RHSC)) { // 8 bits.
  //   Base = N.getOperand(0);
  //   if (Base.getOpcode() == ISD::FrameIndex) {
  //     int FI = cast<FrameIndexSDNode>(Base)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //   }
  //   Offset = CurDAG->getRegister(0, MVT::i32);

  //   ARM_AM::AddrOpc AddSub = ARM_AM::add;
  //   if (RHSC < 0) {
  //     AddSub = ARM_AM::sub;
  //     RHSC = -RHSC;
  //   }
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(AddSub, RHSC),MVT::i32);
  //   return true;
  // }

  // Base = N.getOperand(0);
  // Offset = N.getOperand(1);
  // Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(ARM_AM::add, 0), MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectAddrMode3Offset(SDNode *Op, SDValue N,
                                            SDValue &Offset, SDValue &Opc) {
  // unsigned Opcode = Op->getOpcode();
  // ISD::MemIndexedMode AM = (Opcode == ISD::LOAD)
  //   ? cast<LoadSDNode>(Op)->getAddressingMode()
  //   : cast<StoreSDNode>(Op)->getAddressingMode();
  // ARM_AM::AddrOpc AddSub = (AM == ISD::PRE_INC || AM == ISD::POST_INC)
  //   ? ARM_AM::add : ARM_AM::sub;
  // int Val;
  // if (isScaledConstantInRange(N, /*Scale=*/1, 0, 256, Val)) { // 12 bits.
  //   Offset = CurDAG->getRegister(0, MVT::i32);
  //   Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(AddSub, Val), MVT::i32);
  //   return true;
  // }

  // Offset = N;
  // Opc = CurDAG->getTargetConstant(ARM_AM::getAM3Opc(AddSub, 0), MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectAddrMode5(SDValue N,
                                      SDValue &Base, SDValue &Offset) {
  // if (!CurDAG->isBaseWithConstantOffset(N)) {
  //   Base = N;
  //   if (N.getOpcode() == ISD::FrameIndex) {
  //     int FI = cast<FrameIndexSDNode>(N)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //   } else if (N.getOpcode() == ARMISD::Wrapper &&
  //              !(Subtarget->useMovt() &&
  //                N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
  //     Base = N.getOperand(0);
  //   }
  //   Offset = CurDAG->getTargetConstant(ARM_AM::getAM5Opc(ARM_AM::add, 0),
  //                                      MVT::i32);
  //   return true;
  // }

  // // If the RHS is +/- imm8, fold into addr mode.
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/4,
  //                             -256 + 1, 256, RHSC)) {
  //   Base = N.getOperand(0);
  //   if (Base.getOpcode() == ISD::FrameIndex) {
  //     int FI = cast<FrameIndexSDNode>(Base)->getIndex();
  //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
  //   }

  //   ARM_AM::AddrOpc AddSub = ARM_AM::add;
  //   if (RHSC < 0) {
  //     AddSub = ARM_AM::sub;
  //     RHSC = -RHSC;
  //   }
  //   Offset = CurDAG->getTargetConstant(ARM_AM::getAM5Opc(AddSub, RHSC),
  //                                      MVT::i32);
  //   return true;
  // }

  // Base = N;
  // Offset = CurDAG->getTargetConstant(ARM_AM::getAM5Opc(ARM_AM::add, 0),
  //                                    MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectAddrMode6(SDNode *Parent, SDValue N, SDValue &Addr,
                                      SDValue &Align) {
  // Addr = N;

  // unsigned Alignment = 0;
  // if (LSBaseSDNode *LSN = dyn_cast<LSBaseSDNode>(Parent)) {
  //   // This case occurs only for VLD1-lane/dup and VST1-lane instructions.
  //   // The maximum alignment is equal to the memory size being referenced.
  //   unsigned LSNAlign = LSN->getAlignment();
  //   unsigned MemSize = LSN->getMemoryVT().getSizeInBits() / 8;
  //   if (LSNAlign >= MemSize && MemSize > 1)
  //     Alignment = MemSize;
  // } else {
  //   // All other uses of addrmode6 are for intrinsics.  For now just record
  //   // the raw alignment value; it will be refined later based on the legal
  //   // alignment operands for the intrinsic.
  //   Alignment = cast<MemIntrinsicSDNode>(Parent)->getAlignment();
  // }

  // Align = CurDAG->getTargetConstant(Alignment, MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectAddrMode6Offset(SDNode *Op, SDValue N,
                                            SDValue &Offset) {
  // LSBaseSDNode *LdSt = cast<LSBaseSDNode>(Op);
  // ISD::MemIndexedMode AM = LdSt->getAddressingMode();
  // if (AM != ISD::POST_INC)
  //   return false;
  // Offset = N;
  // if (ConstantSDNode *NC = dyn_cast<ConstantSDNode>(N)) {
  //   if (NC->getZExtValue() * 8 == LdSt->getMemoryVT().getSizeInBits())
  //     Offset = CurDAG->getRegister(0, MVT::i32);
  // }
  return true;
}

bool ARMInvISelDAG::SelectAddrModePC(SDValue N,
                                       SDValue &Offset, SDValue &Label) {
  // if (N.getOpcode() == ARMISD::PIC_ADD && N.hasOneUse()) {
  //   Offset = N.getOperand(0);
  //   SDValue N1 = N.getOperand(1);
  //   Label = CurDAG->getTargetConstant(cast<ConstantSDNode>(N1)->getZExtValue(),
  //                                     MVT::i32);
  //   return true;
  // }

  return false;
}


//===----------------------------------------------------------------------===//
//                         Thumb Addressing Modes
//===----------------------------------------------------------------------===//

bool ARMInvISelDAG::SelectThumbAddrModeRR(SDValue N,
                                            SDValue &Base, SDValue &Offset){
  if (N.getOpcode() != ISD::ADD && !CurDAG->isBaseWithConstantOffset(N)) {
    ConstantSDNode *NC = dyn_cast<ConstantSDNode>(N);
    if (!NC || !NC->isNullValue())
      return false;

    Base = Offset = N;
    return true;
  }

  Base = N.getOperand(0);
  Offset = N.getOperand(1);
  return true;
}

bool
ARMInvISelDAG::SelectThumbAddrModeRI(SDValue N, SDValue &Base,
                                       SDValue &Offset, unsigned Scale) {
  if (Scale == 4) {
    SDValue TmpBase, TmpOffImm;
    if (SelectThumbAddrModeSP(N, TmpBase, TmpOffImm))
      return false;  // We want to select tLDRspi / tSTRspi instead.

    if (N.getOpcode() == ARMISD::Wrapper &&
        N.getOperand(0).getOpcode() == ISD::TargetConstantPool)
      return false;  // We want to select tLDRpci instead.
  }

  if (!CurDAG->isBaseWithConstantOffset(N))
    return false;

  // Thumb does not have [sp, r] address mode.
  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  RegisterSDNode *RHSR = dyn_cast<RegisterSDNode>(N.getOperand(1));
  if ((LHSR && LHSR->getReg() == ARM::SP) ||
      (RHSR && RHSR->getReg() == ARM::SP))
    return false;

  // FIXME: Why do we explicitly check for a match here and then return false?
  // Presumably to allow something else to match, but shouldn't this be
  // documented?
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), Scale, 0, 32, RHSC))
  //   return false;

  Base = N.getOperand(0);
  Offset = N.getOperand(1);
  return true;
}

bool
ARMInvISelDAG::SelectThumbAddrModeRI5S1(SDValue N,
                                          SDValue &Base,
                                          SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 1);
}

bool
ARMInvISelDAG::SelectThumbAddrModeRI5S2(SDValue N,
                                          SDValue &Base,
                                          SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 2);
}

bool
ARMInvISelDAG::SelectThumbAddrModeRI5S4(SDValue N,
                                          SDValue &Base,
                                          SDValue &Offset) {
  return SelectThumbAddrModeRI(N, Base, Offset, 4);
}

bool
ARMInvISelDAG::SelectThumbAddrModeImm5S(SDValue N, unsigned Scale,
                                          SDValue &Base, SDValue &OffImm) {
  if (Scale == 4) {
    SDValue TmpBase, TmpOffImm;
    if (SelectThumbAddrModeSP(N, TmpBase, TmpOffImm))
      return false;  // We want to select tLDRspi / tSTRspi instead.

    if (N.getOpcode() == ARMISD::Wrapper &&
        N.getOperand(0).getOpcode() == ISD::TargetConstantPool)
      return false;  // We want to select tLDRpci instead.
  }

  if (!CurDAG->isBaseWithConstantOffset(N)) {
    // if (N.getOpcode() == ARMISD::Wrapper &&
    //     !(Subtarget->useMovt() &&
    //       N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
    //   Base = N.getOperand(0);
    // } else {
      Base = N;
    // }

    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  RegisterSDNode *RHSR = dyn_cast<RegisterSDNode>(N.getOperand(1));
  if ((LHSR && LHSR->getReg() == ARM::SP) ||
      (RHSR && RHSR->getReg() == ARM::SP)) {
    ConstantSDNode *LHS = dyn_cast<ConstantSDNode>(N.getOperand(0));
    ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1));
    unsigned LHSC = LHS ? LHS->getZExtValue() : 0;
    unsigned RHSC = RHS ? RHS->getZExtValue() : 0;

    // Thumb does not have [sp, #imm5] address mode for non-zero imm5.
    if (LHSC != 0 || RHSC != 0) return false;

    Base = N;
    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  // If the RHS is + imm5 * scale, fold into addr mode.
  // int RHSC;
  // if (isScaledConstantInRange(N.getOperand(1), Scale, 0, 32, RHSC)) {
  //   Base = N.getOperand(0);
  //   OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
  //   return true;
  // }

  Base = N.getOperand(0);
  OffImm = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool
ARMInvISelDAG::SelectThumbAddrModeImm5S4(SDValue N, SDValue &Base,
                                           SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 4, Base, OffImm);
}

bool
ARMInvISelDAG::SelectThumbAddrModeImm5S2(SDValue N, SDValue &Base,
                                           SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 2, Base, OffImm);
}

bool
ARMInvISelDAG::SelectThumbAddrModeImm5S1(SDValue N, SDValue &Base,
                                           SDValue &OffImm) {
  return SelectThumbAddrModeImm5S(N, 1, Base, OffImm);
}

bool ARMInvISelDAG::SelectThumbAddrModeSP(SDValue N,
                                            SDValue &Base, SDValue &OffImm) {
  if (N.getOpcode() == ISD::FrameIndex) {
    int FI = cast<FrameIndexSDNode>(N)->getIndex();
    Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
    OffImm = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  if (!CurDAG->isBaseWithConstantOffset(N))
    return false;

  RegisterSDNode *LHSR = dyn_cast<RegisterSDNode>(N.getOperand(0));
  if (N.getOperand(0).getOpcode() == ISD::FrameIndex ||
      (LHSR && LHSR->getReg() == ARM::SP)) {
    // If the RHS is + imm8 * scale, fold into addr mode.
    // int RHSC;
    // if (isScaledConstantInRange(N.getOperand(1), /*Scale=*/4, 0, 256, RHSC)) {
    //   Base = N.getOperand(0);
    //   if (Base.getOpcode() == ISD::FrameIndex) {
    //     int FI = cast<FrameIndexSDNode>(Base)->getIndex();
    //     Base = CurDAG->getTargetFrameIndex(FI, TLI.getPointerTy());
    //   }
    //   OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
    //   return true;
    // }
  }

  return false;
}


//===----------------------------------------------------------------------===//
//                        Thumb 2 Addressing Modes
//===----------------------------------------------------------------------===//


bool ARMInvISelDAG::SelectT2ShifterOperandReg(SDValue N, SDValue &BaseReg,
                                                SDValue &Opc) {
  // ARM_AM::ShiftOpc ShOpcVal = ARM_AM::getShiftOpcForNode(N.getOpcode());

  // // Don't match base register only case. That is matched to a separate
  // // lower complexity pattern with explicit register operand.
  // if (ShOpcVal == ARM_AM::no_shift) return false;

  // BaseReg = N.getOperand(0);
  // unsigned ShImmVal = 0;
  // if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //   ShImmVal = RHS->getZExtValue() & 31;
  //   Opc = getI32Imm(ARM_AM::getSORegOpc(ShOpcVal, ShImmVal));
  //   return true;
  // }

  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeImm12(SDValue N,
                                            SDValue &Base, SDValue &OffImm) {
  // Match simple R + imm12 operands.

  // Base only.
  if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
      !CurDAG->isBaseWithConstantOffset(N)) {
    if (N.getOpcode() == ISD::FrameIndex) {
      // Match frame index.
      int FI = cast<FrameIndexSDNode>(N)->getIndex();
      Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
      return true;
    }

    // if (N.getOpcode() == ARMISD::Wrapper &&
    //            !(Subtarget->useMovt() &&
    //              N.getOperand(0).getOpcode() == ISD::TargetGlobalAddress)) {
    //   Base = N.getOperand(0);
    //   if (Base.getOpcode() == ISD::TargetConstantPool)
    //     return false;  // We want to select t2LDRpci instead.
    // } else
      Base = N;
    OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }

  if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
    if (SelectT2AddrModeImm8(N, Base, OffImm))
      // Let t2LDRi8 handle (R - imm8).
      return false;

    int RHSC = (int)RHS->getZExtValue();
    if (N.getOpcode() == ISD::SUB)
      RHSC = -RHSC;

    if (RHSC >= 0 && RHSC < 0x1000) { // 12 bits (unsigned)
      Base   = N.getOperand(0);
      if (Base.getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(Base)->getIndex();
        Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      }
      OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
      return true;
    }
  }

  // Base only.
  Base = N;
  OffImm  = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool ARMInvISelDAG::SelectT2AddrModeImm8(SDValue N,
                                           SDValue &Base, SDValue &OffImm) {
  // Match simple R - imm8 operands.
  if (N.getOpcode() != ISD::ADD && N.getOpcode() != ISD::SUB &&
      !CurDAG->isBaseWithConstantOffset(N))
    return false;

  if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
    int RHSC = (int)RHS->getSExtValue();
    if (N.getOpcode() == ISD::SUB)
      RHSC = -RHSC;

    if ((RHSC >= -255) && (RHSC < 0)) { // 8 bits (always negative)
      Base = N.getOperand(0);
      if (Base.getOpcode() == ISD::FrameIndex) {
        int FI = cast<FrameIndexSDNode>(Base)->getIndex();
        Base = CurDAG->getTargetFrameIndex(FI, TLI->getPointerTy());
      }
      OffImm = CurDAG->getTargetConstant(RHSC, MVT::i32);
      return true;
    }
  }

  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeImm8Offset(SDNode *Op, SDValue N,
                                                 SDValue &OffImm){
  // unsigned Opcode = Op->getOpcode();
  // ISD::MemIndexedMode AM = (Opcode == ISD::LOAD)
  //   ? cast<LoadSDNode>(Op)->getAddressingMode()
  //   : cast<StoreSDNode>(Op)->getAddressingMode();
  // int RHSC;
  // if (isScaledConstantInRange(N, /*Scale=*/1, 0, 0x100, RHSC)) { // 8 bits.
  //   OffImm = ((AM == ISD::PRE_INC) || (AM == ISD::POST_INC))
  //     ? CurDAG->getTargetConstant(RHSC, MVT::i32)
  //     : CurDAG->getTargetConstant(-RHSC, MVT::i32);
  //   return true;
  // }

  return false;
}

bool ARMInvISelDAG::SelectT2AddrModeSoReg(SDValue N,
                                            SDValue &Base,
                                            SDValue &OffReg, SDValue &ShImm) {
  // (R - imm8) should be handled by t2LDRi8. The rest are handled by t2LDRi12.
  // if (N.getOpcode() != ISD::ADD && !CurDAG->isBaseWithConstantOffset(N))
  //   return false;

  // // Leave (R + imm12) for t2LDRi12, (R - imm8) for t2LDRi8.
  // if (ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(N.getOperand(1))) {
  //   int RHSC = (int)RHS->getZExtValue();
  //   if (RHSC >= 0 && RHSC < 0x1000) // 12 bits (unsigned)
  //     return false;
  //   else if (RHSC < 0 && RHSC >= -255) // 8 bits
  //     return false;
  // }

  // // Look for (R + R) or (R + (R << [1,2,3])).
  // unsigned ShAmt = 0;
  // Base   = N.getOperand(0);
  // OffReg = N.getOperand(1);

  // // Swap if it is ((R << c) + R).
  // // ARM_AM::ShiftOpc ShOpcVal = ARM_AM::getShiftOpcForNode(OffReg.getOpcode());
  // // if (ShOpcVal != ARM_AM::lsl) {
  // //   ShOpcVal = ARM_AM::getShiftOpcForNode(Base.getOpcode());
  // //   if (ShOpcVal == ARM_AM::lsl)
  // //     std::swap(Base, OffReg);
  // // }

  // if (ShOpcVal == ARM_AM::lsl) {
  //   // Check to see if the RHS of the shift is a constant, if not, we can't fold
  //   // it.
  //   if (ConstantSDNode *Sh = dyn_cast<ConstantSDNode>(OffReg.getOperand(1))) {
  //     ShAmt = Sh->getZExtValue();
  //     if (ShAmt < 4 && isShifterOpProfitable(OffReg, ShOpcVal, ShAmt))
  //       OffReg = OffReg.getOperand(0);
  //     else {
  //       ShAmt = 0;
  //       ShOpcVal = ARM_AM::no_shift;
  //     }
  //   } else {
  //     ShOpcVal = ARM_AM::no_shift;
  //   }
  // }

  // ShImm = CurDAG->getTargetConstant(ShAmt, MVT::i32);

  return true;
}

//===--------------------------------------------------------------------===//

/// getAL - Returns a ARMCC::AL immediate node.
static inline SDValue getAL(SelectionDAG *CurDAG) {
  return CurDAG->getTargetConstant((uint64_t)ARMCC::AL, MVT::i32);
}

SDNode *ARMInvISelDAG::SelectARMIndexedLoad(SDNode *N) {
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::MemIndexedMode AM = LD->getAddressingMode();
  if (AM == ISD::UNINDEXED)
    return NULL;
  EVT LoadedVT = LD->getMemoryVT();
  SDValue Offset, AMOpc;
  bool isPre = (AM == ISD::PRE_INC) || (AM == ISD::PRE_DEC);
  unsigned Opcode = 0;
  bool Match = false;
  if (LoadedVT == MVT::i32 && isPre &&
      SelectAddrMode2OffsetImmPre(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = ARM::LDR_PRE_IMM;
    Match = true;
  } else if (LoadedVT == MVT::i32 && !isPre &&
      SelectAddrMode2OffsetImm(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = ARM::LDR_POST_IMM;
    Match = true;
  } else if (LoadedVT == MVT::i32 &&
      SelectAddrMode2OffsetReg(N, LD->getOffset(), Offset, AMOpc)) {
    Opcode = isPre ? ARM::LDR_PRE_REG : ARM::LDR_POST_REG;
    Match = true;

  } else if (LoadedVT == MVT::i16 &&
             SelectAddrMode3Offset(N, LD->getOffset(), Offset, AMOpc)) {
    Match = true;
    Opcode = (LD->getExtensionType() == ISD::SEXTLOAD)
      ? (isPre ? ARM::LDRSH_PRE : ARM::LDRSH_POST)
      : (isPre ? ARM::LDRH_PRE : ARM::LDRH_POST);
  } else if (LoadedVT == MVT::i8 || LoadedVT == MVT::i1) {
    if (LD->getExtensionType() == ISD::SEXTLOAD) {
      if (SelectAddrMode3Offset(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = isPre ? ARM::LDRSB_PRE : ARM::LDRSB_POST;
      }
    } else {
      if (isPre &&
          SelectAddrMode2OffsetImmPre(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = ARM::LDRB_PRE_IMM;
      } else if (!isPre &&
                  SelectAddrMode2OffsetImm(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = ARM::LDRB_POST_IMM;
      } else if (SelectAddrMode2OffsetReg(N, LD->getOffset(), Offset, AMOpc)) {
        Match = true;
        Opcode = isPre ? ARM::LDRB_PRE_REG : ARM::LDRB_POST_REG;
      }
    }
  }

  if (Match) {
    if (Opcode == ARM::LDR_PRE_IMM || Opcode == ARM::LDRB_PRE_IMM) {
      SDValue Chain = LD->getChain();
      SDValue Base = LD->getBasePtr();
      SDValue Ops[]= { Base, AMOpc, getAL(CurDAG),
                       CurDAG->getRegister(0, MVT::i32), Chain };
      return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32,
                                    MVT::i32, MVT::Other, Ops);
    } else {
      SDValue Chain = LD->getChain();
      SDValue Base = LD->getBasePtr();
      SDValue Ops[]= { Base, Offset, AMOpc, getAL(CurDAG),
                       CurDAG->getRegister(0, MVT::i32), Chain };
      return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32,
                                    MVT::i32, MVT::Other, Ops);
    }
  }

  return NULL;
}

SDNode *ARMInvISelDAG::SelectT2IndexedLoad(SDNode *N) {
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::MemIndexedMode AM = LD->getAddressingMode();
  if (AM == ISD::UNINDEXED)
    return NULL;

  EVT LoadedVT = LD->getMemoryVT();
  bool isSExtLd = LD->getExtensionType() == ISD::SEXTLOAD;
  SDValue Offset;
  bool isPre = (AM == ISD::PRE_INC) || (AM == ISD::PRE_DEC);
  unsigned Opcode = 0;
  bool Match = false;
  if (SelectT2AddrModeImm8Offset(N, LD->getOffset(), Offset)) {
    switch (LoadedVT.getSimpleVT().SimpleTy) {
    case MVT::i32:
      Opcode = isPre ? ARM::t2LDR_PRE : ARM::t2LDR_POST;
      break;
    case MVT::i16:
      if (isSExtLd)
        Opcode = isPre ? ARM::t2LDRSH_PRE : ARM::t2LDRSH_POST;
      else
        Opcode = isPre ? ARM::t2LDRH_PRE : ARM::t2LDRH_POST;
      break;
    case MVT::i8:
    case MVT::i1:
      if (isSExtLd)
        Opcode = isPre ? ARM::t2LDRSB_PRE : ARM::t2LDRSB_POST;
      else
        Opcode = isPre ? ARM::t2LDRB_PRE : ARM::t2LDRB_POST;
      break;
    default:
      return NULL;
    }
    Match = true;
  }

  if (Match) {
    SDValue Chain = LD->getChain();
    SDValue Base = LD->getBasePtr();
    SDValue Ops[]= { Base, Offset, getAL(CurDAG),
                     CurDAG->getRegister(0, MVT::i32), Chain };
    return CurDAG->getMachineNode(Opcode, SDLoc(N), MVT::i32, MVT::i32,
                                  MVT::Other, Ops);
  }

  return NULL;
}

void ARMInvISelDAG::InvLoadOrStoreMultiple(SDNode *N, bool Ld, bool Inc, bool B,
  bool WB) {
  // Pattern:  (STMDB_UPD:void GPR:$Rn, pred:$p, reglist:$regs,
  //                           variable_ops)
  // Emits: for each $reg in $regs:
  //          (store $reg, $Rn+offset($reg))
  //        (sub $Rn, sizeof($regs)*regsize)
  // Same for LDMIA, except we add instead of sub and we load instead of
  // store.

  //   xxM type cond   base write-back, {register list}
  // Stack     Other
  // LDMED     LDMIB     Pre-incremental load
  // LDMFD     LDMIA     Post-incremental load
  // LDMEA     LDMDB     Pre-decremental load
  // LDMFA     LDMDA     Post-decremental load
  // STMFA     STMIB     Pre-incremental store
  // STMEA     STMIA     Post-incremental store
  // STMFD     STMDB     Pre-decremental store
  // STMED     STMDA     Post-decremental store

  // UPD refers to write-back in LLVM-speak.
  // the ED/FD/EA/FA don't seem to be in LLVM from what I have seen.
  // writeback is defined as a ! in ASM.

  // Op[0] is Chain
  // Op[1] is $Rn
  // Op[2] is $p (14)
  // Op[3] is predicate (%noreg)
  SDValue Chain = N->getOperand(0);
  SDValue Ptr = N->getOperand(1);
  SDValue PrevPtr = Ptr;
  SDValue UsePtr;
  SDValue Offset = CurDAG->getConstant(4, EVT(MVT::i32), false);
  SDLoc SL(N);
  SDVTList VTList = CurDAG->getVTList(MVT::i32);
  EVT LdType = N->getValueType(0); // Note: unused in the store case...
  unsigned ImmSum = 0;
  uint16_t MathOpc = (Inc) ? ISD::ADD : ISD::SUB;
  for (unsigned i = 4; i < N->getNumOperands(); ++i) {
    // Semantics of Store is that it takes a chain, value, and pointer.
    // NOTE: There is a 4th oper which is "undef" from what we see.
    SDValue Val = N->getOperand(i);

    // We create an instruction for each oper to do the $Rn+Offset
    // that becomes the Ptr op for the store instruction
    // LLVM Uses a "FrameIndex" when going forward, which we could do
    // but better to move this to later in our pipeline.
    // NOTE: Need to validate that this is the semantics of the command!
    Ptr = CurDAG->getNode(MathOpc, SL, VTList, Ptr,
      Offset);

    // FIXME: the 4 and 0 here are really specific -- double check these
    // are always good.
    ImmSum += 4;
    Value *NullPtr = 0;
    MachineMemOperand* MMO =
      new MachineMemOperand(MachinePointerInfo(NullPtr, ImmSum),
        MachineMemOperand::MOStore, 4, 0);

    // Before or after behavior
    UsePtr = (B) ? PrevPtr : Ptr;

    SDValue ResNode;
    if (Ld) {
      // This is a special case, because we have to flip CopyFrom/To
      if (Val->hasNUsesOfValue(1, 0)) {
        Chain = Val->getOperand(0);
      }

      // Check if we are loading into PC, if we are, emit a return.
      RegisterSDNode *RegNode =
        dyn_cast<RegisterSDNode>(Val->getOperand(1));
      const MCRegisterInfo *RI =
        Dec->getDisassembler()->getMCDirector()->getMCRegisterInfo();
      if (RI->isSubRegisterEq(RI->getProgramCounter(), RegNode->getReg())) {
        ResNode = CurDAG->getNode(ARMISD::RET_FLAG, SL, MVT::Other,
          CurDAG->getRoot());
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);
        CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
        CurDAG->setRoot(ResNode);
        return;
      } else {
        ResNode = CurDAG->getLoad(LdType, SL, Chain, UsePtr,
          MachinePointerInfo::getConstantPool(), false, false, true, 0);
        SDValue C2R = CurDAG->getCopyToReg(ResNode.getValue(1), SL,
          RegNode->getReg(), ResNode);
        Chain = C2R;
        // If CopyFromReg has only 1 use, replace it
        if (Val->hasNUsesOfValue(1, 0)) {
          CurDAG->ReplaceAllUsesOfValueWith(Val, ResNode);
          CurDAG->ReplaceAllUsesOfValueWith(Val.getValue(1),
            ResNode.getValue(1));
          CurDAG->DeleteNode(Val.getNode());
        }
      }
    } else {
      ResNode = CurDAG->getStore(Chain, SL, Val, UsePtr, MMO);
      Chain = ResNode;
    }
    if (ResNode.getNumOperands() > 1) {
      FixChainOp(ResNode.getNode());
    }
    PrevPtr = Ptr;
  }

  // NOTE: This gets handled automagically because DAG represents it, but leave
  // for now in case we need it.
  // Writeback operation
  // if (WB) {
  //   RegisterSDNode *WBReg =
  //     dyn_cast<RegisterSDNode>(N->getOperand(1)->getOperand(1));
  //   Chain = CurDAG->getCopyToReg(Ptr, SL, WBReg->getReg(), Chain);
  // }

  // Pass the chain to the last chain node
  CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);

  // Pass the last math operation to any uses of Rn
  CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);
}

bool ARMInvISelDAG::SelectCMOVPred(SDValue N, SDValue &Pred, SDValue &Reg) {
  const ConstantSDNode *CN = cast<ConstantSDNode>(N);
  Pred = CurDAG->getTargetConstant(CN->getZExtValue(), MVT::i32);
  Reg = CurDAG->getRegister(ARM::CPSR, MVT::i32);
  return true;
}


} // end fracture namespace
