//===- ARMISD.h - SDNode Enums for ARM Decompiler  ===========-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Provides ISD SDNOde enums for the ARM targets.
//
//===----------------------------------------------------------------------===//

#ifndef ARMISD_H
#define ARMISD_H

using namespace llvm;

// NOTE: These are taken directly from lib/Target/ARM/ARMISelLowering.h, but
// this file is not exposed to us..
// You need to update this file to match the llvm install.

namespace fracture {

  namespace ARMISD {
    // ARM Specific DAG Nodes
    enum NodeType {
      // Start the numbering where the builtin ops and target ops leave off.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      Wrapper,      // Wrapper - A wrapper node for TargetConstantPool,
                    // TargetExternalSymbol, and TargetGlobalAddress.
      WrapperPIC,   // WrapperPIC - A wrapper node for TargetGlobalAddress in
                    // PIC mode.
      WrapperJT,    // WrapperJT - A wrapper node for TargetJumpTable

      // Add pseudo op to model memcpy for struct byval.
      COPY_STRUCT_BYVAL,

      CALL,         // Function call.
      CALL_PRED,    // Function call that's predicable.
      CALL_NOLINK,  // Function call with branch not branch-and-link.
      tCALL,        // Thumb function call.
      BRCOND,       // Conditional branch.
      BR_JT,        // Jumptable branch.
      BR2_JT,       // Jumptable branch (2 level - jumptable entry is a jump).
      RET_FLAG,     // Return with a flag operand.
      INTRET_FLAG,  // Interrupt return with an LR-offset and a flag operand.

      PIC_ADD,      // Add with a PC operand and a PIC label.

      CMP,          // ARM compare instructions.
      CMN,          // ARM CMN instructions.
      CMPZ,         // ARM compare that sets only Z flag.
      CMPFP,        // ARM VFP compare instruction, sets FPSCR.
      CMPFPw0,      // ARM VFP compare against zero instruction, sets FPSCR.
      FMSTAT,       // ARM fmstat instruction.

      CMOV,         // ARM conditional move instructions.

      BCC_i64,

      RBIT,         // ARM bitreverse instruction

      FTOSI,        // FP to sint within a FP register.
      FTOUI,        // FP to uint within a FP register.
      SITOF,        // sint to FP within a FP register.
      UITOF,        // uint to FP within a FP register.

      SRL_FLAG,     // V,Flag = srl_flag X -> srl X, 1 + save carry out.
      SRA_FLAG,     // V,Flag = sra_flag X -> sra X, 1 + save carry out.
      RRX,          // V = RRX X, Flag     -> srl X, 1 + shift in carry flag.

      ADDC,         // Add with carry
      ADDE,         // Add using carry
      SUBC,         // Sub with carry
      SUBE,         // Sub using carry

      VMOVRRD,      // double to two gprs.
      VMOVDRR,      // Two gprs to double.

      EH_SJLJ_SETJMP,         // SjLj exception handling setjmp.
      EH_SJLJ_LONGJMP,        // SjLj exception handling longjmp.

      TC_RETURN,    // Tail call return pseudo.

      THREAD_POINTER,

      DYN_ALLOC,    // Dynamic allocation on the stack.

      MEMBARRIER_MCR, // Memory barrier (MCR)

      PRELOAD,      // Preload

      WIN__CHKSTK,  // Windows' __chkstk call to do stack probing.

      VCEQ,         // Vector compare equal.
      VCEQZ,        // Vector compare equal to zero.
      VCGE,         // Vector compare greater than or equal.
      VCGEZ,        // Vector compare greater than or equal to zero.
      VCLEZ,        // Vector compare less than or equal to zero.
      VCGEU,        // Vector compare unsigned greater than or equal.
      VCGT,         // Vector compare greater than.
      VCGTZ,        // Vector compare greater than zero.
      VCLTZ,        // Vector compare less than zero.
      VCGTU,        // Vector compare unsigned greater than.
      VTST,         // Vector test bits.

      // Vector shift by immediate:
      VSHL,         // ...left
      VSHRs,        // ...right (signed)
      VSHRu,        // ...right (unsigned)

      // Vector rounding shift by immediate:
      VRSHRs,       // ...right (signed)
      VRSHRu,       // ...right (unsigned)
      VRSHRN,       // ...right narrow

      // Vector saturating shift by immediate:
      VQSHLs,       // ...left (signed)
      VQSHLu,       // ...left (unsigned)
      VQSHLsu,      // ...left (signed to unsigned)
      VQSHRNs,      // ...right narrow (signed)
      VQSHRNu,      // ...right narrow (unsigned)
      VQSHRNsu,     // ...right narrow (signed to unsigned)

      // Vector saturating rounding shift by immediate:
      VQRSHRNs,     // ...right narrow (signed)
      VQRSHRNu,     // ...right narrow (unsigned)
      VQRSHRNsu,    // ...right narrow (signed to unsigned)

      // Vector shift and insert:
      VSLI,         // ...left
      VSRI,         // ...right

      // Vector get lane (VMOV scalar to ARM core register)
      // (These are used for 8- and 16-bit element types only.)
      VGETLANEu,    // zero-extend vector extract element
      VGETLANEs,    // sign-extend vector extract element

      // Vector move immediate and move negated immediate:
      VMOVIMM,
      VMVNIMM,

      // Vector move f32 immediate:
      VMOVFPIMM,

      // Vector duplicate:
      VDUP,
      VDUPLANE,

      // Vector shuffles:
      VEXT,         // extract
      VREV64,       // reverse elements within 64-bit doublewords
      VREV32,       // reverse elements within 32-bit words
      VREV16,       // reverse elements within 16-bit halfwords
      VZIP,         // zip (interleave)
      VUZP,         // unzip (deinterleave)
      VTRN,         // transpose
      VTBL1,        // 1-register shuffle with mask
      VTBL2,        // 2-register shuffle with mask

      // Vector multiply long:
      VMULLs,       // ...signed
      VMULLu,       // ...unsigned

      UMLAL,        // 64bit Unsigned Accumulate Multiply
      SMLAL,        // 64bit Signed Accumulate Multiply

      // Operands of the standard BUILD_VECTOR node are not legalized, which
      // is fine if BUILD_VECTORs are always lowered to shuffles or other
      // operations, but for ARM some BUILD_VECTORs are legal as-is and their
      // operands need to be legalized.  Define an ARM-specific version of
      // BUILD_VECTOR for this purpose.
      BUILD_VECTOR,

      // Floating-point max and min:
      FMAX,
      FMIN,
      VMAXNM,
      VMINNM,

      // Bit-field insert
      BFI,

      // Vector OR with immediate
      VORRIMM,
      // Vector AND with NOT of immediate
      VBICIMM,

      // Vector bitwise select
      VBSL,

      // Vector load N-element structure to all lanes:
      VLD2DUP = ISD::FIRST_TARGET_MEMORY_OPCODE,
      VLD3DUP,
      VLD4DUP,

      // NEON loads with post-increment base updates:
      VLD1_UPD,
      VLD2_UPD,
      VLD3_UPD,
      VLD4_UPD,
      VLD2LN_UPD,
      VLD3LN_UPD,
      VLD4LN_UPD,
      VLD2DUP_UPD,
      VLD3DUP_UPD,
      VLD4DUP_UPD,

      // NEON stores with post-increment base updates:
      VST1_UPD,
      VST2_UPD,
      VST3_UPD,
      VST4_UPD,
      VST2LN_UPD,
      VST3LN_UPD,
      VST4LN_UPD
    };
  }
} // end fracture namespace


#endif
