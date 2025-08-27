#include "Instruction.hpp"

gdbw::Instruction::Instruction(cs_insn* instruction)
{
	m_address = instruction->address;
	memcpy(m_mnemonic, instruction->mnemonic, sizeof(m_mnemonic));
	memcpy(m_opstr, instruction->op_str, sizeof(m_opstr));
	memcpy(m_bytes, instruction->bytes, sizeof(m_bytes));
	m_size = instruction->size;
}

gdbw::Instruction::~Instruction()
{
}

void gdbw::Instruction::CreateInstructionsTable(lua_State* L, std::vector<Instruction*> insns)
{
	// create parent table
	lua_createtable(L, insns.size(), 0);

	for (size_t i = 0; i < insns.size(); i++)
	{
		// child table (Instruction)
		lua_createtable(L, 0, 5);

		lua_pushinteger(L, insns[i]->m_address);
		lua_setfield(L, -2, "address");

		lua_pushlstring(L, (const char*)insns[i]->m_bytes, sizeof(insns[i]->m_bytes));
		lua_setfield(L, -2, "bytes");

		lua_pushstring(L, insns[i]->m_mnemonic);
		lua_setfield(L, -2, "mnemonic");

		lua_pushstring(L, insns[i]->m_opstr);
		lua_setfield(L, -2, "opstr");

		lua_pushinteger(L, insns[i]->m_size);
		lua_setfield(L, -2, "size");

		// Child index
		lua_rawseti(L, -2, i + 1);
	}
}
