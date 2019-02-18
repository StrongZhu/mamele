// license:BSD-3-Clause
// copyright-holders:David Haywood
/*********************************\

 ARCtangent A4 disassembler

\*********************************/

#ifndef MAME_CPU_ARC_ARCDASM_H
#define MAME_CPU_ARC_ARCDASM_H

#pragma once

class arc_disassembler : public util::disasm_interface
{
public:
	arc_disassembler() = default;
	virtual ~arc_disassembler() = default;

	virtual u32 opcode_alignment() const override;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) override;

private:
	static const char *const basic[0x20];
	static const char *conditions[0x20];
	static const char *delaytype[0x4];
	static const char *regnames[0x40];

};

#endif
