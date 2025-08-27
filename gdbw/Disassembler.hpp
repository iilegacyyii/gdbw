#pragma once
#include <expected>
#include <vector>
#include "thirdparty/capstone/capstone/capstone.h"
#include "Instruction.hpp"

namespace gdbw
{
	class Disassembler
	{
	public:
		Disassembler(cs_arch arch, cs_mode mode);
		~Disassembler();
		// Disassemble a region of memory, stopping at the first invalid instruction.
		// Returns vector of Instructions.
		std::expected<std::vector<Instruction*>, std::string> Disasm(
			const uint8_t* code, size_t len, size_t count, uint64_t address = 0x1000);
	private:
		cs_arch m_arch;
		cs_mode m_mode;
	};
}


