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
    case X86::JBE_1:{   //Jump short if Below or Equal
      SDValue Chain = N->getOperand(0);
      SDValue Target = N->getOperand(1);
      //SDValue EFLAGSCFR = N->getOperand(2);
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
    case X86::JAE_1:{   //Jump short if Above or Equal
      SDValue Chain = N->getOperand(0);
      uint64_t TarVal = N->getConstantOperandVal(1);
      SDValue Target = CurDAG->getConstant(TarVal, MVT::i32);
      //SDValue EFLAGSCFR = N->getOperand(2);
      SDLoc SL(N);

      // Calculate the Branch Target
      SDValue BT = CurDAG->getConstant(
          cast<ConstantSDNode>(Target)->getZExtValue(), Target.getValueType());

      // Condition Code is "Below or Equal" <=
      SDValue CC = CurDAG->getCondCode(ISD::SETGE);
      SDValue BrNode =
          CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, CC, BT, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);

      return NULL;
      break;
    }
    case X86::CMP32rm:
    case X86::CMP32mr:
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
      SDVTList SubVTList = CurDAG->getVTList(MVT::i32);
      SDValue Width = CurDAG->getConstant(4, MVT::i32);
      SDValue Addr = CurDAG->getNode(ISD::SUB, SL, SubVTList, ESP, Width);	//ESP -= 4;

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
    case X86::MOV32rr_REV:
    case X86::MOV64rr:
    case X86::MOV32rr:{
      // Note: Cannot use dummy arithmetic here because it will get folded
      // (removed) from the DAG. Instead we search for a CopyToReg, if it
      // exists we set it's debug location to the Mov, and if it doesn't we
      // print an error and do nothing.
      SDNode *C2R = NULL;
      for (SDNode::use_iterator I = N->use_begin(), E = N->use_end(); I != E; ++I) {
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
    case X86::JMP_1:{
      SDValue Chain = N->getOperand(0);
      uint64_t TarVal = N->getConstantOperandVal(1);
      SDValue Target = CurDAG->getConstant(TarVal, MVT::i32);

      SDLoc SL(N);
      SDValue BrNode = CurDAG->getNode(ISD::BR, SL, MVT::Other, Target, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);

      return NULL;
      break;
    }
    case X86::CALLpcrel32:{
      SDValue Chain = N->getOperand(0);
      uint64_t TarVal = N->getConstantOperandVal(1);
      SDValue Target = CurDAG->getConstant(TarVal, MVT::i32);
      //SDValue noReg = N->getOperand(2);

      SDLoc SL(N);

      SDValue CallNode = CurDAG->getNode(X86ISD::CALL, SL, MVT::Other, Target, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), CallNode);

      return NULL;
      break;
    }
    case X86::RETL:{
      SDValue Chain = N->getOperand(0);
      SDLoc SL(N);
      SDValue RetNode = CurDAG->getNode(X86ISD::RET_FLAG, SL, MVT::Other, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), RetNode);

      return NULL;
      break;
    }
    case X86::POP32r:{
      EVT LdType = N->getValueType(0);
      SDValue Chain = N->getOperand(0);
      SDValue Ptr = N->getOperand(1);   //ESP
      //SDValue PrevPtr = Ptr;
      SDValue Offset = CurDAG->getConstant(4, EVT(MVT::i32), false);

      SDLoc SL(N);

      SDVTList VTList = CurDAG->getVTList(MVT::i32);
      uint16_t MathOpc = ISD::ADD;
      Ptr = CurDAG->getNode(MathOpc, SL, VTList, Ptr, Offset);  //ESP += 4;

      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);

      const MachineSDNode *MN = dyn_cast<MachineSDNode>(N);
      MachineMemOperand *MMO = NULL;
      if (MN->memoperands_empty()) {
        errs() << "NO MACHINE OPS for STR_PRE_IMM!\n";
      } else {
        MMO = *(MN->memoperands_begin());
      }

      // Pass the chain to the last chain node
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Chain);

      // Pass the last math operation to any uses of Rn
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Ptr);

      SDValue Load = CurDAG->getLoad(LdType, SL, Chain, Ptr,
              MachinePointerInfo::getConstantPool(), false, false, true, 0);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), Load);

      return NULL;
      break;
    }
    case X86::JA_1:{    //JMP Short if Above
      SDValue Chain = N->getOperand(0);
      uint64_t TarVal = N->getConstantOperandVal(1);
      SDValue Target = CurDAG->getConstant(TarVal, MVT::i32);
      //SDValue EFLAGSCFR = N->getOperand(2);
      SDLoc SL(N);

      // Calculate the Branch Target
      SDValue BT = CurDAG->getConstant(
          cast<ConstantSDNode>(Target)->getZExtValue(), Target.getValueType());

      // Condition Code is "Below or Equal" <=
      SDValue CC = CurDAG->getCondCode(ISD::SETGT);
      SDValue BrNode =
          CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, CC, BT, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);

      return NULL;
      break;
    }
    case X86::JNE_1:{   //Jump short if Not Equal
      SDValue Chain = N->getOperand(0);
      uint64_t TarVal = N->getConstantOperandVal(1);
      SDValue Target = CurDAG->getConstant(TarVal, MVT::i32);
      //SDValue EFLAGSCFR = N->getOperand(2);
      SDLoc SL(N);

      // Calculate the Branch Target
      SDValue BT = CurDAG->getConstant(
          cast<ConstantSDNode>(Target)->getZExtValue(), Target.getValueType());

      // Condition Code is "Below or Equal" <=
      SDValue CC = CurDAG->getCondCode(ISD::SETNE);
      SDValue BrNode =
          CurDAG->getNode(ISD::BRCOND, SL, MVT::Other, CC, BT, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), BrNode);

      return NULL;
      break;
    }
    case X86::ADD32mi8:{
      SDValue Chain = N->getOperand(0);
      SDValue EBP = N->getOperand(1);
      uint64_t C2val = N->getConstantOperandVal(2);
      SDValue C2 = CurDAG->getConstant(C2val, MVT::i32);
      //3 is undef
      uint64_t Cn4val = N->getConstantOperandVal(4);
      SDValue Cn4 = CurDAG->getConstant(Cn4val, MVT::i32);
      //4 is undef
      //6 is Constant 1

      SDLoc SL(N);
      SDVTList VTList = CurDAG->getVTList(MVT::i32, MVT::Other);
      /* AJG: Not sure how fill in semantics yet.
      SDValue v2 = N->getOperand(2);


      SDValue Node = CurDAG->getNode(ISD::ADD, SL, VTList, v1, v2, Chain);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Node);
      */

      return NULL;
      break;
    }
    case X86::ADD32mr:{
      SDValue Chain = N->getOperand(0);
      SDValue EBP = N->getOperand(1);
      uint64_t C1val = N->getConstantOperandVal(2);
      SDValue C1 = CurDAG->getConstant(C1val, MVT::i32);
      //3 is noReg
      uint64_t Cn4val = N->getConstantOperandVal(4);
      SDValue Cn4 = CurDAG->getConstant(Cn4val, MVT::i32);
      //5 is noReg

      SDLoc SL(N);
      SDVTList VTList = CurDAG->getVTList(MVT::i32, MVT::Other);

      return NULL;
      break;
    }
    case X86::ADD32rr_REV:{ //Note: no chain, two input, two output
      SDValue ESI = N->getOperand(0);
      SDValue EAX = N->getOperand(1);

      SDLoc SL(N);
      SDVTList VTList = CurDAG->getVTList(MVT::i32, MVT::i32);

      SDValue Node = CurDAG->getNode(ISD::ADD, SL, VTList, EAX, ESI);
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), Node);

      return NULL;
      break;
    }
    case X86::LEAVE:{   //ESP = EBP
      SDValue Chain = N->getOperand(0);
      SDValue EBP = N->getOperand(1);
      SDValue ESP = N->getOperand(2);

      SDLoc SL(N);
      SDVTList VTList = CurDAG->getVTList(MVT::i32);
      SDValue Zero = CurDAG->getConstant(0, MVT::i32);
      //This should potentially be optimized to one instruction
      SDValue EBPZero = CurDAG->getNode(ISD::MUL, SL, VTList, EBP, Zero);  //EBP = 0;
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), EBPZero);
      VTList = CurDAG->getVTList(MVT::i32, MVT::i32, MVT::Other);
      SDValue EBPeqESP = CurDAG->getNode(ISD::ADD, SL, VTList, EBP, ESP, Chain);  //EBP += ESP;
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 1), EBPeqESP);

      /* AJG: Reduce above to something similar to this...
      SDVTList VTList = CurDAG->getVTList(MVT::i32, MVT::i32, MVT::Other);
      SDValue EBPeqESP = CurDAG->getNode(ISD::SETEQ, SL, VTList, EBP, ESP, Chain);  //EBP = ESP;
      CurDAG->ReplaceAllUsesOfValueWith(SDValue(N, 0), EBPeqESP);
      */

      return NULL;
      break;
    }
  }

  SDNode* TheRes = InvertCode(N);
  return TheRes;
}

}
