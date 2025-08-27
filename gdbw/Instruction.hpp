#pragma once
#include <vector>
#include "thirdparty/capstone/capstone/capstone.h"
#include "LuaManager.hpp"

namespace gdbw
{
	class Instruction
	{
	public:
		Instruction(cs_insn* instruction);
		~Instruction();
		// Convert a vector of Instructions to a lua table and push it to the stack
		// used to return instruction(s) as a binding return value
		static void CreateInstructionsTable(lua_State* L, std::vector<Instruction*> insns);
	private:
		uint64_t m_address;
		char m_mnemonic[32]; // capstone size
		char m_opstr[160];   // capstone size
		uint8_t m_bytes[24]; // capstone size
		uint64_t m_size; // technically only need uint8 but lua uses longlong
	};
}