#include "Disassembler.hpp"

gdbw::Disassembler::Disassembler(cs_arch arch, cs_mode mode)
{
	m_arch = arch;
	m_mode = mode;
}

gdbw::Disassembler::~Disassembler()
{
}

std::expected<std::vector<gdbw::Instruction*>, std::string> gdbw::Disassembler::Disasm(
	const uint8_t* code, size_t len, size_t count, uint64_t address)
{
	auto result = std::vector<Instruction*>(0);
	csh handle;
	cs_insn* insn;

	if (count < 0)
		return std::unexpected("Disassembler.Disasm cannot disassemble <0 instructions");

	if (cs_open(m_arch, m_mode, &handle) != cs_err::CS_ERR_OK)
		return std::unexpected("Disassembler.Disasm failed to open capstone handle");

	count = cs_disasm(handle, code, len, address, count, &insn);
	if (count == 0)
	{
		std::string err = std::format("Disassembler.Disasm cs_disasm failed (0x{:#x})", (int)cs_errno(handle));
		cs_free(insn, count);
		cs_close(&handle);
		return std::unexpected(err);
	}

	result.reserve(count);
	for (size_t i = 0; i < count; i++)
		result.push_back(new Instruction(&insn[i]));

	cs_free(insn, count);
	cs_close(&handle);
	return result;
}


