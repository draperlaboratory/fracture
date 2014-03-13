//===- X86ISD.h - SDNode Enums for X86 Decompiler  ===========-*- C++ -*-=//
//
//              Fracture: The Draper Decompiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// This file modifies a file originally found in LLVM.

//
//===----------------------------------------------------------------------===//
//
// Provides ISD SDNode enums for X86 targets.
//
//===----------------------------------------------------------------------===//

#ifndef X86ISD_H
#define X86ISD_H

using namespace llvm;

// NOTE: These are taken directly from lib/Target/X86/X86ISelLowering.h, but
// this file is not exposed to us..
// You need to update this file to match the llvm install.

namespace fracture {

namespace X86ISD {
    // X86 Specific DAG Nodes
    enum NodeType {
      // Start the numbering where the builtin ops leave off.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      /// BSF - Bit scan forward.
      /// BSR - Bit scan reverse.
      BSF,
      BSR,

      /// SHLD, SHRD - Double shift instructions. These correspond to
      /// X86::SHLDxx and X86::SHRDxx instructions.
      SHLD,
      SHRD,

      /// FAND - Bitwise logical AND of floating point values. This corresponds
      /// to X86::ANDPS or X86::ANDPD.
      FAND,

      /// FOR - Bitwise logical OR of floating point values. This corresponds
      /// to X86::ORPS or X86::ORPD.
      FOR,

      /// FXOR - Bitwise logical XOR of floating point values. This corresponds
      /// to X86::XORPS or X86::XORPD.
      FXOR,

      /// FANDN - Bitwise logical ANDNOT of floating point values. This
      /// corresponds to X86::ANDNPS or X86::ANDNPD.
      FANDN,

      /// FSRL - Bitwise logical right shift of floating point values. These
      /// corresponds to X86::PSRLDQ.
      FSRL,

      /// CALL - These operations represent an abstract X86 call
      /// instruction, which includes a bunch of information.  In particular the
      /// operands of these node are:
      ///
      ///     #0 - The incoming token chain
      ///     #1 - The callee
      ///     #2 - The number of arg bytes the caller pushes on the stack.
      ///     #3 - The number of arg bytes the callee pops off the stack.
      ///     #4 - The value to pass in AL/AX/EAX (optional)
      ///     #5 - The value to pass in DL/DX/EDX (optional)
      ///
      /// The result values of these nodes are:
      ///
      ///     #0 - The outgoing token chain
      ///     #1 - The first register result value (optional)
      ///     #2 - The second register result value (optional)
      ///
      CALL,

      /// RDTSC_DAG - This operation implements the lowering for
      /// readcyclecounter
      RDTSC_DAG,

      /// X86 compare and logical compare instructions.
      CMP, COMI, UCOMI,

      /// X86 bit-test instructions.
      BT,

      /// X86 SetCC. Operand 0 is condition code, and operand 1 is the EFLAGS
      /// operand, usually produced by a CMP instruction.
      SETCC,

      /// X86 Select
      SELECT,

      // Same as SETCC except it's materialized with a sbb and the value is all
      // one's or all zero's.
      SETCC_CARRY,  // R = carry_bit ? ~0 : 0

      /// X86 FP SETCC, implemented with CMP{cc}SS/CMP{cc}SD.
      /// Operands are two FP values to compare; result is a mask of
      /// 0s or 1s.  Generally DTRT for C/C++ with NaNs.
      FSETCC,

      /// X86 MOVMSK{pd|ps}, extracts sign bits of two or four FP values,
      /// result in an integer GPR.  Needs masking for scalar result.
      FGETSIGNx86,

      /// X86 conditional moves. Operand 0 and operand 1 are the two values
      /// to select from. Operand 2 is the condition code, and operand 3 is the
      /// flag operand produced by a CMP or TEST instruction. It also writes a
      /// flag result.
      CMOV,

      /// X86 conditional branches. Operand 0 is the chain operand, operand 1
      /// is the block to branch if condition is true, operand 2 is the
      /// condition code, and operand 3 is the flag operand produced by a CMP
      /// or TEST instruction.
      BRCOND,

      /// Return with a flag operand. Operand 0 is the chain operand, operand
      /// 1 is the number of bytes of stack to pop.
      RET_FLAG,

      /// REP_STOS - Repeat fill, corresponds to X86::REP_STOSx.
      REP_STOS,

      /// REP_MOVS - Repeat move, corresponds to X86::REP_MOVSx.
      REP_MOVS,

      /// GlobalBaseReg - On Darwin, this node represents the result of the popl
      /// at function entry, used for PIC code.
      GlobalBaseReg,

      /// Wrapper - A wrapper node for TargetConstantPool,
      /// TargetExternalSymbol, and TargetGlobalAddress.
      Wrapper,

      /// WrapperRIP - Special wrapper used under X86-64 PIC mode for RIP
      /// relative displacements.
      WrapperRIP,

      /// MOVDQ2Q - Copies a 64-bit value from the low word of an XMM vector
      /// to an MMX vector.  If you think this is too close to the previous
      /// mnemonic, so do I; blame Intel.
      MOVDQ2Q,

      /// MMX_MOVD2W - Copies a 32-bit value from the low word of a MMX
      /// vector to a GPR.
      MMX_MOVD2W,

      /// PEXTRB - Extract an 8-bit value from a vector and zero extend it to
      /// i32, corresponds to X86::PEXTRB.
      PEXTRB,

      /// PEXTRW - Extract a 16-bit value from a vector and zero extend it to
      /// i32, corresponds to X86::PEXTRW.
      PEXTRW,

      /// INSERTPS - Insert any element of a 4 x float vector into any element
      /// of a destination 4 x floatvector.
      INSERTPS,

      /// PINSRB - Insert the lower 8-bits of a 32-bit value to a vector,
      /// corresponds to X86::PINSRB.
      PINSRB,

      /// PINSRW - Insert the lower 16-bits of a 32-bit value to a vector,
      /// corresponds to X86::PINSRW.
      PINSRW, MMX_PINSRW,

      /// PSHUFB - Shuffle 16 8-bit values within a vector.
      PSHUFB,

      /// ANDNP - Bitwise Logical AND NOT of Packed FP values.
      ANDNP,

      /// PSIGN - Copy integer sign.
      PSIGN,

      /// BLENDV - Blend where the selector is a register.
      BLENDV,

      /// BLENDI - Blend where the selector is an immediate.
      BLENDI,

      // SUBUS - Integer sub with unsigned saturation.
      SUBUS,

      /// HADD - Integer horizontal add.
      HADD,

      /// HSUB - Integer horizontal sub.
      HSUB,

      /// FHADD - Floating point horizontal add.
      FHADD,

      /// FHSUB - Floating point horizontal sub.
      FHSUB,

      /// UMAX, UMIN - Unsigned integer max and min.
      UMAX, UMIN,

      /// SMAX, SMIN - Signed integer max and min.
      SMAX, SMIN,

      /// FMAX, FMIN - Floating point max and min.
      ///
      FMAX, FMIN,

      /// FMAXC, FMINC - Commutative FMIN and FMAX.
      FMAXC, FMINC,

      /// FRSQRT, FRCP - Floating point reciprocal-sqrt and reciprocal
      /// approximation.  Note that these typically require refinement
      /// in order to obtain suitable precision.
      FRSQRT, FRCP,

      // TLSADDR - Thread Local Storage.
      TLSADDR,

      // TLSBASEADDR - Thread Local Storage. A call to get the start address
      // of the TLS block for the current module.
      TLSBASEADDR,

      // TLSCALL - Thread Local Storage.  When calling to an OS provided
      // thunk at the address from an earlier relocation.
      TLSCALL,

      // EH_RETURN - Exception Handling helpers.
      EH_RETURN,

      // EH_SJLJ_SETJMP - SjLj exception handling setjmp.
      EH_SJLJ_SETJMP,

      // EH_SJLJ_LONGJMP - SjLj exception handling longjmp.
      EH_SJLJ_LONGJMP,

      /// TC_RETURN - Tail call return. See X86TargetLowering::LowerCall for
      /// the list of operands.
      TC_RETURN,

      // VZEXT_MOVL - Vector move to low scalar and zero higher vector elements.
      VZEXT_MOVL,

      // VZEXT - Vector integer zero-extend.
      VZEXT,

      // VSEXT - Vector integer signed-extend.
      VSEXT,

      // VTRUNC - Vector integer truncate.
      VTRUNC,

      // VTRUNC - Vector integer truncate with mask.
      VTRUNCM,

      // VFPEXT - Vector FP extend.
      VFPEXT,

      // VFPROUND - Vector FP round.
      VFPROUND,

      // VSHL, VSRL - 128-bit vector logical left / right shift
      VSHLDQ, VSRLDQ,

      // VSHL, VSRL, VSRA - Vector shift elements
      VSHL, VSRL, VSRA,

      // VSHLI, VSRLI, VSRAI - Vector shift elements by immediate
      VSHLI, VSRLI, VSRAI,

      // CMPP - Vector packed double/float comparison.
      CMPP,

      // PCMP* - Vector integer comparisons.
      PCMPEQ, PCMPGT,
      // PCMP*M - Vector integer comparisons, the result is in a mask vector.
      PCMPEQM, PCMPGTM,

      /// CMPM, CMPMU - Vector comparison generating mask bits for fp and
      /// integer signed and unsigned data types.
      CMPM,
      CMPMU,

      // ADD, SUB, SMUL, etc. - Arithmetic operations with FLAGS results.
      ADD, SUB, ADC, SBB, SMUL,
      INC, DEC, OR, XOR, AND,

      BZHI,   // BZHI - Zero high bits
      BEXTR,  // BEXTR - Bit field extract

      UMUL, // LOW, HI, FLAGS = umul LHS, RHS

      // MUL_IMM - X86 specific multiply by immediate.
      MUL_IMM,

      // PTEST - Vector bitwise comparisons.
      PTEST,

      // TESTP - Vector packed fp sign bitwise comparisons.
      TESTP,

      // TESTM, TESTNM - Vector "test" in AVX-512, the result is in a mask vector.
      TESTM,
      TESTNM,

      // OR/AND test for masks
      KORTEST,

      // Several flavors of instructions with vector shuffle behaviors.
      PALIGNR,
      PSHUFD,
      PSHUFHW,
      PSHUFLW,
      SHUFP,
      MOVDDUP,
      MOVSHDUP,
      MOVSLDUP,
      MOVLHPS,
      MOVLHPD,
      MOVHLPS,
      MOVLPS,
      MOVLPD,
      MOVSD,
      MOVSS,
      UNPCKL,
      UNPCKH,
      VPERMILP,
      VPERMV,
      VPERMV3,
      VPERMIV3,
      VPERMI,
      VPERM2X128,
      VBROADCAST,
      // masked broadcast
      VBROADCASTM,
      // Insert/Extract vector element
      VINSERT,
      VEXTRACT,

      // PMULUDQ - Vector multiply packed unsigned doubleword integers
      PMULUDQ,

      // FMA nodes
      FMADD,
      FNMADD,
      FMSUB,
      FNMSUB,
      FMADDSUB,
      FMSUBADD,

      // VASTART_SAVE_XMM_REGS - Save xmm argument registers to the stack,
      // according to %al. An operator is needed so that this can be expanded
      // with control flow.
      VASTART_SAVE_XMM_REGS,

      // WIN_ALLOCA - Windows's _chkstk call to do stack probing.
      WIN_ALLOCA,

      // SEG_ALLOCA - For allocating variable amounts of stack space when using
      // segmented stacks. Check if the current stacklet has enough space, and
      // falls back to heap allocation if not.
      SEG_ALLOCA,

      // WIN_FTOL - Windows's _ftol2 runtime routine to do fptoui.
      WIN_FTOL,

      // Memory barrier
      MEMBARRIER,
      MFENCE,
      SFENCE,
      LFENCE,

      // FNSTSW16r - Store FP status word into i16 register.
      FNSTSW16r,

      // SAHF - Store contents of %ah into %eflags.
      SAHF,

      // RDRAND - Get a random integer and indicate whether it is valid in CF.
      RDRAND,

      // RDSEED - Get a NIST SP800-90B & C compliant random integer and
      // indicate whether it is valid in CF.
      RDSEED,

      // PCMP*STRI
      PCMPISTRI,
      PCMPESTRI,

      // XTEST - Test if in transactional execution.
      XTEST,

      // ATOMADD64_DAG, ATOMSUB64_DAG, ATOMOR64_DAG, ATOMAND64_DAG,
      // ATOMXOR64_DAG, ATOMNAND64_DAG, ATOMSWAP64_DAG -
      // Atomic 64-bit binary operations.
      ATOMADD64_DAG = ISD::FIRST_TARGET_MEMORY_OPCODE,
      ATOMSUB64_DAG,
      ATOMOR64_DAG,
      ATOMXOR64_DAG,
      ATOMAND64_DAG,
      ATOMNAND64_DAG,
      ATOMMAX64_DAG,
      ATOMMIN64_DAG,
      ATOMUMAX64_DAG,
      ATOMUMIN64_DAG,
      ATOMSWAP64_DAG,

      // LCMPXCHG_DAG, LCMPXCHG8_DAG, LCMPXCHG16_DAG - Compare and swap.
      LCMPXCHG_DAG,
      LCMPXCHG8_DAG,
      LCMPXCHG16_DAG,

      // VZEXT_LOAD - Load, scalar_to_vector, and zero extend.
      VZEXT_LOAD,

      // FNSTCW16m - Store FP control world into i16 memory.
      FNSTCW16m,

      /// FP_TO_INT*_IN_MEM - This instruction implements FP_TO_SINT with the
      /// integer destination in memory and a FP reg source.  This corresponds
      /// to the X86::FIST*m instructions and the rounding mode change stuff. It
      /// has two inputs (token chain and address) and two outputs (int value
      /// and token chain).
      FP_TO_INT16_IN_MEM,
      FP_TO_INT32_IN_MEM,
      FP_TO_INT64_IN_MEM,

      /// FILD, FILD_FLAG - This instruction implements SINT_TO_FP with the
      /// integer source in memory and FP reg result.  This corresponds to the
      /// X86::FILD*m instructions. It has three inputs (token chain, address,
      /// and source type) and two outputs (FP value and token chain). FILD_FLAG
      /// also produces a flag).
      FILD,
      FILD_FLAG,

      /// FLD - This instruction implements an extending load to FP stack slots.
      /// This corresponds to the X86::FLD32m / X86::FLD64m. It takes a chain
      /// operand, ptr to load from, and a ValueType node indicating the type
      /// to load to.
      FLD,

      /// FST - This instruction implements a truncating store to FP stack
      /// slots. This corresponds to the X86::FST32m / X86::FST64m. It takes a
      /// chain operand, value to store, address, and a ValueType to store it
      /// as.
      FST,

      /// VAARG_64 - This instruction grabs the address of the next argument
      /// from a va_list. (reads and modifies the va_list in memory)
      VAARG_64

      // WARNING: Do not add anything in the end unless you want the node to
      // have memop! In fact, starting from ATOMADD64_DAG all opcodes will be
      // thought as target memory ops!
    };
  }
} // end fracture namespace


#endif
