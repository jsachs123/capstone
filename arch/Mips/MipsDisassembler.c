//===- MipsDisassembler.cpp - Disassembler for Mips -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Mips Disassembler.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2014 */

#ifdef CAPSTONE_HAS_MIPS

#include <stdio.h>
#include <string.h>

#include "../../inttypes.h"

#include "../../utils.h"

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

#include "../../MathExtras.h"

//#include "Mips.h"
//#include "MipsRegisterInfo.h"
//#include "MipsSubtarget.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCInst.h"
//#include "llvm/MC/MCSubtargetInfo.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"

// Forward declare these because the autogenerated code will reference them.
// Definitions are further down.
static DecodeStatus DecodeGPR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeGPR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodePtrRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeDSPRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeCCRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFGRCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeHWRegsRegisterClass(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeAFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128BRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128HRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128WRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128DRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeCOP2RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeJumpTarget(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget21(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBranchTarget26(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

// DecodeBranchTargetMM - Decode microMIPS branch offset, which is
// shifted left by 1 bit.
static DecodeStatus DecodeBranchTargetMM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder);

// DecodeJumpTargetMM - Decode microMIPS jump target, which is
// shifted left by 1 bit.
static DecodeStatus DecodeJumpTargetMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMSA128Mem(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMImm12(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeMemMMImm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeFMem(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSpecial3LlSc(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

// Decode the immediate field of an LSA instruction which
// is off by one.
static DecodeStatus DecodeLSAImm(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeInsSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeExtSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm19Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeSimm18Lsl3(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder);

/// INSVE_[BHWD] have an implicit operand that the generated decoder doesn't
/// handle.
static DecodeStatus DecodeINSVE_DF_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeAddiGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeDaddiGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBlezlGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBgtzlGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBgtzGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);

static DecodeStatus DecodeBlezGroupBranch_4(MCInst *MI,
		uint32_t insn, uint64_t Address, MCRegisterInfo *Decoder);


#define GET_SUBTARGETINFO_ENUM
#include "MipsGenSubtargetInfo.inc"

// Hacky: enable all features for disassembler
static uint64_t getFeatureBits(int mode)
{
	uint64_t Bits = (uint64_t)-1;	// include every features by default

	// ref: MipsGenDisassemblerTables.inc::checkDecoderPredicate()
	// some features are mutually execlusive
	if (mode & CS_MODE_16) {
		//Bits &= ~Mips_FeatureMips32r2;
		//Bits &= ~Mips_FeatureMips32;
		//Bits &= ~Mips_FeatureFPIdx;
		//Bits &= ~Mips_FeatureBitCount;
		//Bits &= ~Mips_FeatureSwap;
		//Bits &= ~Mips_FeatureSEInReg;
		//Bits &= ~Mips_FeatureMips64r2;
		//Bits &= ~Mips_FeatureFP64Bit;
	} else if (mode & CS_MODE_32) {
		Bits &= ~Mips_FeatureMips16;
		Bits &= ~Mips_FeatureFP64Bit;
		Bits &= ~Mips_FeatureMips32r6;
		Bits &= ~Mips_FeatureMips64r6;
	} else if (mode & CS_MODE_64) {
		Bits &= ~Mips_FeatureMips16;
		Bits &= ~Mips_FeatureMips64r6;
		Bits &= ~Mips_FeatureMips64r6;
	}

	if (mode & CS_MODE_MICRO) {
		Bits |= Mips_FeatureMicroMips;
		Bits &= ~Mips_FeatureMips4_32r2;
		Bits &= ~Mips_FeatureMips2;
	} else {
		Bits &= ~Mips_FeatureMicroMips;
	}

	return Bits;
}

#include "MipsGenDisassemblerTables.inc"

#define GET_REGINFO_ENUM
#include "MipsGenRegisterInfo.inc"

#define GET_REGINFO_MC_DESC
#include "MipsGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "MipsGenInstrInfo.inc"

void Mips_init(MCRegisterInfo *MRI)
{
	// InitMCRegisterInfo(MipsRegDesc, 317,
	// RA, PC,
	// MipsMCRegisterClasses, 34,
	// MipsRegUnitRoots, 196,
	// MipsRegDiffLists,
	// MipsRegStrings,
	// MipsSubRegIdxLists, 12,
	// MipsSubRegIdxRanges,   MipsRegEncodingTable);

	// InitMCRegisterInfo(MipsRegDesc, 386,
	//		RA, PC,
	//		MipsMCRegisterClasses, 47,
	//		MipsRegUnitRoots, 265,
	//		MipsRegDiffLists,
	//		MipsRegStrings,
	//		MipsSubRegIdxLists, 12,
	//		MipsSubRegIdxRanges, MipsRegEncodingTable);

	MCRegisterInfo_InitMCRegisterInfo(MRI, MipsRegDesc, 386,
			0, 0, 
			MipsMCRegisterClasses, 47,
			0, 0, 
			MipsRegDiffLists,
			0, 
			MipsSubRegIdxLists, 12,
			0);
}

/// readInstruction - read four bytes from the MemoryObject
/// and return 32 bit word sorted according to the given endianess
static DecodeStatus readInstruction32(unsigned char *code, uint32_t *insn, bool isBigEndian, bool isMicroMips)
{
	// We want to read exactly 4 Bytes of data.
	if (isBigEndian) {
		// Encoded as a big-endian 32-bit word in the stream.
		*insn = (code[3] <<  0) |
			(code[2] <<  8) |
			(code[1] << 16) |
			(code[0] << 24);
	} else {
		// Encoded as a small-endian 32-bit word in the stream.
		// Little-endian byte ordering:
		//   mips32r2:   4 | 3 | 2 | 1
		//   microMIPS:  2 | 1 | 4 | 3
		if (isMicroMips) {
			*insn = (code[2] <<  0) |
				(code[3] <<  8) |
				(code[0] << 16) |
				(code[1] << 24);
		} else {
			*insn = (code[0] <<  0) |
				(code[1] <<  8) |
				(code[2] << 16) |
				(code[3] << 24);
		}
	}

	return MCDisassembler_Success;
}

static DecodeStatus MipsDisassembler_getInstruction(int mode, MCInst *instr,
		const uint8_t *code, size_t code_len,
		uint16_t *Size,
		uint64_t Address, bool isBigEndian, MCRegisterInfo *MRI)
{
	uint32_t Insn;
	DecodeStatus Result;

	if (code_len < 4)
		// not enough data
		return MCDisassembler_Fail;

	if (instr->flat_insn->detail) {
		memset(instr->flat_insn->detail, 0, sizeof(cs_detail));
	}

	Result = readInstruction32((unsigned char*)code, &Insn, isBigEndian,
			mode & CS_MODE_MICRO);
	if (Result == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	if (mode & CS_MODE_MICRO) {
		// Calling the auto-generated decoder function.
		Result = decodeInstruction(DecoderTableMicroMips32, instr, Insn, Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
		return MCDisassembler_Fail;
	}

	if (((mode & CS_MODE_32) == 0) && ((mode & CS_MODE_MIPS3) == 0)) {	// COP3
		// DEBUG(dbgs() << "Trying COP3_ table (32-bit opcodes):\n");
		Result = decodeInstruction(DecoderTableCOP3_32, instr, Insn, Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	if (((mode & CS_MODE_MIPS32R6) != 0) && ((mode & CS_MODE_MIPSGP64) != 0)) {
		// DEBUG(dbgs() << "Trying Mips32r6_64r6 (GPR64) table (32-bit opcodes):\n");
		Result = decodeInstruction(DecoderTableMips32r6_64r6_GP6432, instr, Insn,
				Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	if ((mode & CS_MODE_MIPS32R6) != 0) {
		// DEBUG(dbgs() << "Trying Mips32r6_64r6 table (32-bit opcodes):\n");
		Result = decodeInstruction(DecoderTableMips32r6_64r632, instr, Insn,
				Address, MRI, mode);
		if (Result != MCDisassembler_Fail) {
			*Size = 4;
			return Result;
		}
	}

	// Calling the auto-generated decoder function.
	Result = decodeInstruction(DecoderTableMips32, instr, Insn, Address, MRI, mode);
	if (Result != MCDisassembler_Fail) {
		*Size = 4;
		return Result;
	}

	return MCDisassembler_Fail;
}

bool Mips_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *instr,
		uint16_t *size, uint64_t address, void *info)
{
	cs_struct *handle = (cs_struct *)(uintptr_t)ud;

	DecodeStatus status = MipsDisassembler_getInstruction(handle->mode, instr,
			code, code_len,
			size,
			address, handle->big_endian, (MCRegisterInfo *)info);

	return status == MCDisassembler_Success;
}

static DecodeStatus Mips64Disassembler_getInstruction(int mode, MCInst *instr,
		const uint8_t *code, size_t code_len,
		uint16_t *Size,
		uint64_t Address, bool isBigEndian, MCRegisterInfo *MRI)
{
	uint32_t Insn;

	DecodeStatus Result = readInstruction32((unsigned char*)code, &Insn, isBigEndian, false);
	if (Result == MCDisassembler_Fail)
		return MCDisassembler_Fail;

	if (instr->flat_insn->detail) {
		memset(instr->flat_insn->detail, 0, sizeof(cs_detail));
	}

	// Calling the auto-generated decoder function.
	Result = decodeInstruction(DecoderTableMips6432, instr, Insn, Address, MRI, mode);
	if (Result != MCDisassembler_Fail) {
		*Size = 4;
		return Result;
	}
	// If we fail to decode in Mips64 decoder space we can try in Mips32
	Result = decodeInstruction(DecoderTableMips32, instr, Insn, Address, MRI, mode);
	if (Result != MCDisassembler_Fail) {
		*Size = 4;
		return Result;
	}

	return MCDisassembler_Fail;
}

bool Mips64_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *instr,
		uint16_t *size, uint64_t address, void *info)
{
	cs_struct *handle = (cs_struct *)(uintptr_t)ud;

	DecodeStatus status = Mips64Disassembler_getInstruction(handle->mode, instr,
			code, code_len,
			size,
			address, handle->big_endian, (MCRegisterInfo *)info);

	return status == MCDisassembler_Success;
}

static unsigned getReg(MCRegisterInfo *MRI, unsigned RC, unsigned RegNo)
{
	//MipsDisassemblerBase *Dis = static_cast<const MipsDisassemblerBase*>(D);
	//return *(Dis->getRegInfo()->getRegClass(RC).begin() + RegNo);
	MCRegisterClass *rc = MCRegisterInfo_getRegClass(MRI, RC);
	return rc->RegsBegin[RegNo];
}

#define nullptr NULL

static DecodeStatus DecodeINSVE_DF_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	typedef DecodeStatus (*DecodeFN)(MCInst *, unsigned, uint64_t, MCRegisterInfo *);
	// The size of the n field depends on the element size
	// The register class also depends on this.
	uint32_t tmp = fieldFromInstruction(insn, 17, 5);
	unsigned NSize = 0;
	DecodeFN RegDecoder = nullptr;
	if ((tmp & 0x18) == 0x00) { // INSVE_B
		NSize = 4;
		RegDecoder = DecodeMSA128BRegisterClass;
	} else if ((tmp & 0x1c) == 0x10) { // INSVE_H
		NSize = 3;
		RegDecoder = DecodeMSA128HRegisterClass;
	} else if ((tmp & 0x1e) == 0x18) { // INSVE_W
		NSize = 2;
		RegDecoder = DecodeMSA128WRegisterClass;
	} else if ((tmp & 0x1f) == 0x1c) { // INSVE_D
		NSize = 1;
		RegDecoder = DecodeMSA128DRegisterClass;
	} //else llvm_unreachable("Invalid encoding");

	//assert(NSize != 0 && RegDecoder != nullptr);

	// $wd
	tmp = fieldFromInstruction(insn, 6, 5);
	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;
	// $wd_in
	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;
	// $n
	tmp = fieldFromInstruction(insn, 16, NSize);
	MCOperand_CreateImm0(MI, tmp);
	// $ws
	tmp = fieldFromInstruction(insn, 11, 5);
	if (RegDecoder(MI, tmp, Address, Decoder) == MCDisassembler_Fail)
		return MCDisassembler_Fail;
	// $n2
	MCOperand_CreateImm0(MI, 0);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeAddiGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the ADDI instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b001000 sssss ttttt iiiiiiiiiiiiiiii
	//      BOVC if rs >= rt
	//      BEQZALC if rs == 0 && rt != 0
	//      BEQC if rs < rt && rs != 0

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;
	bool HasRs = false;

	if (Rs >= Rt) {
		MCInst_setOpcode(MI, Mips_BOVC);
		HasRs = true;
	} else if (Rs != 0 && Rs < Rt) {
		MCInst_setOpcode(MI, Mips_BEQC);
		HasRs = true;
	} else
		MCInst_setOpcode(MI, Mips_BEQZALC);

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeDaddiGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the ADDI instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b011000 sssss ttttt iiiiiiiiiiiiiiii
	//      BNVC if rs >= rt
	//      BNEZALC if rs == 0 && rt != 0
	//      BNEC if rs < rt && rs != 0

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;
	bool HasRs = false;

	if (Rs >= Rt) {
		MCInst_setOpcode(MI, Mips_BNVC);
		HasRs = true;
	} else if (Rs != 0 && Rs < Rt) {
		MCInst_setOpcode(MI, Mips_BNEC);
		HasRs = true;
	} else
		MCInst_setOpcode(MI, Mips_BNEZALC);

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBlezlGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the BLEZL instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b010110 sssss ttttt iiiiiiiiiiiiiiii
	//      Invalid if rs == 0
	//      BLEZC   if rs == 0  && rt != 0
	//      BGEZC   if rs == rt && rt != 0
	//      BGEC    if rs != rt && rs != 0  && rt != 0

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;
	bool HasRs = false;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BLEZC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BGEZC);
	else {
		HasRs = true;
		MCInst_setOpcode(MI, Mips_BGEC);
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBgtzlGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the BGTZL instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b010111 sssss ttttt iiiiiiiiiiiiiiii
	//      Invalid if rs == 0
	//      BGTZC   if rs == 0  && rt != 0
	//      BLTZC   if rs == rt && rt != 0
	//      BLTC    if rs != rt && rs != 0  && rt != 0

	bool HasRs = false;

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BGTZC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BLTZC);
	else {
		MCInst_setOpcode(MI, Mips_BLTC);
		HasRs = true;
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));
	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBgtzGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the BGTZ instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b000111 sssss ttttt iiiiiiiiiiiiiiii
	//      BGTZ    if rt == 0
	//      BGTZALC if rs == 0 && rt != 0
	//      BLTZALC if rs != 0 && rs == rt
	//      BLTUC   if rs != 0 && rs != rt

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;
	bool HasRs = false;
	bool HasRt = false;

	if (Rt == 0) {
		MCInst_setOpcode(MI, Mips_BGTZ);
		HasRs = true;
	} else if (Rs == 0) {
		MCInst_setOpcode(MI, Mips_BGTZALC);
		HasRt = true;
	} else if (Rs == Rt) {
		MCInst_setOpcode(MI, Mips_BLTZALC);
		HasRs = true;
	} else {
		MCInst_setOpcode(MI, Mips_BLTUC);
		HasRs = true;
		HasRt = true;
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	if (HasRt)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBlezGroupBranch_4(MCInst *MI, uint32_t insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	// If we are called then we can assume that MIPS32r6/MIPS64r6 is enabled
	// (otherwise we would have matched the BLEZL instruction from the earlier
	// ISA's instead).
	//
	// We have:
	//    0b000110 sssss ttttt iiiiiiiiiiiiiiii
	//      Invalid   if rs == 0
	//      BLEZALC   if rs == 0  && rt != 0
	//      BGEZALC   if rs == rt && rt != 0
	//      BGEUC     if rs != rt && rs != 0  && rt != 0

	uint32_t Rs = fieldFromInstruction(insn, 21, 5);
	uint32_t Rt = fieldFromInstruction(insn, 16, 5);
	uint32_t Imm = (uint32_t)SignExtend64(fieldFromInstruction(insn, 0, 16), 16) << 2;
	bool HasRs = false;

	if (Rt == 0)
		return MCDisassembler_Fail;
	else if (Rs == 0)
		MCInst_setOpcode(MI, Mips_BLEZALC);
	else if (Rs == Rt)
		MCInst_setOpcode(MI, Mips_BGEZALC);
	else {
		HasRs = true;
		MCInst_setOpcode(MI, Mips_BGEUC);
	}

	if (HasRs)
		MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rs));

	MCOperand_CreateReg0(MI, getReg(Decoder, Mips_GPR32RegClassID, Rt));

	MCOperand_CreateImm0(MI, Imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCPU16RegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	return MCDisassembler_Fail;
}

static DecodeStatus DecodeGPR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPR64RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGPR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_GPR32RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodePtrRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	if (Inst->csh->mode & CS_MODE_N64)
		return DecodeGPR64RegisterClass(Inst, RegNo, Address, Decoder);

	return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeDSPRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	return DecodeGPR32RegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGR64RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFGR32RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGR32RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeCCRRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_CCRRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FCCRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeFGRCCRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_FGRCCRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	if (MCInst_getOpcode(Inst) == Mips_SC){
		MCOperand_CreateReg0(Inst, Reg);
	}

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128Mem(MCInst *Inst, unsigned Insn,
		uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(fieldFromInstruction(Insn, 16, 10), 10);
	unsigned Reg = fieldFromInstruction(Insn, 6, 5);
	unsigned Base = fieldFromInstruction(Insn, 11, 5);

	Reg = getReg(Decoder, Mips_MSA128BRegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	// MCOperand_CreateImm0(Inst, Offset);

	// The immediate field of an LD/ST instruction is scaled which means it must
	// be multiplied (when decoding) by the size (in bytes) of the instructions'
	// data format.
	// .b - 1 byte
	// .h - 2 bytes
	// .w - 4 bytes
	// .d - 8 bytes
	switch(MCInst_getOpcode(Inst)) {
		default:
			//assert (0 && "Unexpected instruction");
			return MCDisassembler_Fail;
			break;
		case Mips_LD_B:
		case Mips_ST_B:
			MCOperand_CreateImm0(Inst, Offset);
			break;
		case Mips_LD_H:
		case Mips_ST_H:
			MCOperand_CreateImm0(Inst, Offset << 1);
			break;
		case Mips_LD_W:
		case Mips_ST_W:
			MCOperand_CreateImm0(Inst, Offset << 2);
			break;
		case Mips_LD_D:
		case Mips_ST_D:
			MCOperand_CreateImm0(Inst, Offset << 3);
			break;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMImm12(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0x0fff, 12);
	unsigned Reg = fieldFromInstruction(Insn, 21, 5);
	unsigned Base = fieldFromInstruction(Insn, 16, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	if (MCInst_getOpcode(Inst) == Mips_SC_MM)
		MCOperand_CreateReg0(Inst, Reg);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemMMImm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 21, 5);
	unsigned Base = fieldFromInstruction(Insn, 16, 5);

	Reg = getReg(Decoder, Mips_GPR32RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFMem(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Offset = SignExtend32(Insn & 0xffff, 16);
	unsigned Reg = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Reg = getReg(Decoder, Mips_FGR64RegClassID, Reg);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	MCOperand_CreateReg0(Inst, Reg);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSpecial3LlSc(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int64_t Offset = SignExtend64((Insn >> 7) & 0x1ff, 9);
	unsigned Rt = fieldFromInstruction(Insn, 16, 5);
	unsigned Base = fieldFromInstruction(Insn, 21, 5);

	Rt = getReg(Decoder, Mips_GPR32RegClassID, Rt);
	Base = getReg(Decoder, Mips_GPR32RegClassID, Base);

	if (MCInst_getOpcode(Inst) == Mips_SC_R6 ||
			MCInst_getOpcode(Inst) == Mips_SCD_R6) {
		MCOperand_CreateReg0(Inst, Rt);
	}

	MCOperand_CreateReg0(Inst, Rt);
	MCOperand_CreateReg0(Inst, Base);
	MCOperand_CreateImm0(Inst, Offset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeHWRegsRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	// Currently only hardware register 29 is supported.
	if (RegNo != 29)
		return  MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, Mips_HWR29);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeAFGR64RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 30 || RegNo % 2)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_AFGR64RegClassID, RegNo /2);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeACC64DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_ACC64DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeHI32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_HI32DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeLO32DSPRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo >= 4)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_LO32DSPRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128BRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128BRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128HRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128HRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128WRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128WRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSA128DRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSA128DRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMSACtrlRegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 7)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_MSACtrlRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCOP2RegisterClass(MCInst *Inst,
		unsigned RegNo, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, Mips_COP2RegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = (SignExtend32(Offset, 16) << 2) + 4;
	MCOperand_CreateImm0(Inst, BranchOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeJumpTarget(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 2;
	MCOperand_CreateImm0(Inst, JumpOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget21(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 21) << 2;

	MCOperand_CreateImm0(Inst, BranchOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTarget26(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 26) << 2;

	MCOperand_CreateImm0(Inst, BranchOffset);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeBranchTargetMM(MCInst *Inst,
		unsigned Offset, uint64_t Address, MCRegisterInfo *Decoder)
{
	int32_t BranchOffset = SignExtend32(Offset, 16) << 1;
	MCOperand_CreateImm0(Inst, BranchOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeJumpTargetMM(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	unsigned JumpOffset = fieldFromInstruction(Insn, 0, 26) << 1;
	MCOperand_CreateImm0(Inst, JumpOffset);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm16(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 16));
	return MCDisassembler_Success;
}

static DecodeStatus DecodeLSAImm(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	// We add one to the immediate field as it was encoded as 'imm - 1'.
	MCOperand_CreateImm0(Inst, Insn + 1);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeInsSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	// First we need to grab the pos(lsb) from MCInst.
	int Pos = (int)MCOperand_getImm(MCInst_getOperand(Inst, 2));
	int Size = (int) Insn - Pos + 1;
	MCOperand_CreateImm0(Inst, SignExtend32(Size, 16));
	return MCDisassembler_Success;
}

static DecodeStatus DecodeExtSize(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	int Size = (int) Insn  + 1;
	MCOperand_CreateImm0(Inst, SignExtend32(Size, 16));
	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm19Lsl2(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 19) << 2);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeSimm18Lsl3(MCInst *Inst,
		unsigned Insn, uint64_t Address, MCRegisterInfo *Decoder)
{
	MCOperand_CreateImm0(Inst, SignExtend32(Insn, 18) << 3);
	return MCDisassembler_Success;
}

#endif
