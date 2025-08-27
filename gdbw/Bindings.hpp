#pragma once
#include "DebugEngine.hpp"
#include "Disassembler.hpp"
#include "MemoryRegion.hpp"

extern gdbw::DE::Engine* g_dbg;

// Set a field in a table, assumes table is currently at the top of the lua stack.
static inline void setfieldi(lua_State* L, const char* key, size_t value, int idx = -2)
{
	lua_pushinteger(L, value);
	lua_setfield(L, idx, key);
}

namespace gdbw::bindings
{
	static int AddressToSymbol(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);

		auto result = g_dbg->GetSymbolManager()->SymbolFromAddress(address);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		auto symbol = *result;
		lua_createtable(L, 0, 5);

		lua_pushinteger(L, symbol->Address());
		lua_setfield(L, -2, "address");
		lua_pushinteger(L, symbol->Displacement());
		lua_setfield(L, -2, "displacement");
		lua_pushinteger(L, symbol->Flags());
		lua_setfield(L, -2, "flags");
		lua_pushinteger(L, symbol->ModBase());
		lua_setfield(L, -2, "modbase");
		lua_pushlstring(L, symbol->Name(), strlen(symbol->Name()));
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, symbol->Size());
		lua_setfield(L, -2, "size");

		return 1;
	}

	static int AddressToModuleName(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);
		if (address == LUA_TNIL)
		{
			lua_pushnil(L);
			luaL_error(L, "Address must be integer");
			return 2;
		}

		auto result = g_dbg->AddressToModule(address);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		auto name = *result;
		lua_pushstring(L, name.c_str());
		return 1;
	}

	static int BreakpointAdd(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);
		if (address == LUA_TNIL)
		{
			lua_pushnil(L);
			luaL_error(L, "Address must be integer");
			return 2;
		}

		auto result = g_dbg->BreakpointAdd(address);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		lua_pushinteger(L, *result);
		return 1;
	}

	static int BreakpointRemove(lua_State* L)
	{
		size_t id = luaL_checkinteger(L, 1);

		auto result = g_dbg->BreakpointRemove(id);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		return 0;
	}

	static int BreakpointGetAll(lua_State* L)
	{
		std::vector<PDEBUG_BREAKPOINT> bps = g_dbg->GetBreakpoints();

		lua_createtable(L, (int)bps.size(), 0);
		for (size_t i = 0; i < bps.size(); i++)
		{
			PDEBUG_BREAKPOINT bp = bps[i];
			ULONG64 address = 0;
			ULONG bpflags = 0;
			int enabled = 0;
			if (bp == nullptr) continue;
			auto hr = bp->GetOffset(&address);
			if (FAILED(hr))
			{
				lua_pushnil(L);
				luaL_error(L, std::format("Failed to get address for breakpoint {}", i).c_str());
				return 2;
			}
			hr = bp->GetFlags(&bpflags);
			if (FAILED(hr))
			{
				lua_pushnil(L);
				luaL_error(L, std::format("Failed to get flags for breakpoint {}", i).c_str());
				return 2;
			}

			enabled = bpflags & DEBUG_BREAKPOINT_ENABLED;

			// child table (Breakpoint)
			lua_createtable(L, 0, 3);

			lua_pushinteger(L, i);
			lua_setfield(L, -2, "id");
			lua_pushinteger(L, address);
			lua_setfield(L, -2, "address");
			lua_pushboolean(L, enabled);
			lua_setfield(L, -2, "enabled");

			// push breakpoint index (in parent table)
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int ConsoleCols(lua_State* L)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		int cols;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;

		lua_pushinteger(L, cols);
		return 1;
	}

	static int ConsoleRows(lua_State* L)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		int rows;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

		lua_pushinteger(L, rows);
		return 1;
	}

	static int Continue(lua_State* L)
	{
		g_dbg->SetState(DE::State::RUN);
		return 0;
	}

	static int Disassemble(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);
		ULONG len = (ULONG)luaL_checkinteger(L, 2);
		size_t count = luaL_checkinteger(L, 3);

		unsigned char* code = (unsigned char*)calloc(len, 1);
		auto readmem_result = g_dbg->ReadVMUncached(address, &len, code);
		if (!readmem_result)
		{
			lua_pushnil(L);
			luaL_error(L, readmem_result.error().c_str());
			free(code);
			return 2;
		}

		// TODO: when debugging syswow binary and executing 64-bit code, this assumes 32-bit code is also 64bit.
		cs_mode mode = g_dbg->Is64BitTarget() ? cs_mode::CS_MODE_64 : cs_mode::CS_MODE_32;
		auto dis = gdbw::Disassembler(cs_arch::CS_ARCH_X86, mode);

		auto disasm_result = dis.Disasm((const uint8_t*)code, len, count, address);
		if (!disasm_result)
		{
			lua_pushnil(L);
			luaL_error(L, readmem_result.error().c_str());
			free(code);
			return 2;
		}
		auto insns = *disasm_result;
		Instruction::CreateInstructionsTable(L, insns);

		free(code);
		return 1; // returns a single table
	}

	static int Evaluate(lua_State* L)
	{
		PSTR expression = (PSTR)luaL_checkstring(L, 1);
		if (expression == LUA_TNIL)
		{
			lua_pushnil(L);
			luaL_error(L, "Must provide an expression (e.g. rax+0x20)");
			return 2;
		}

		std::string expr = std::format("{}", expression);
		auto result = g_dbg->Evaluate((char*)expr.c_str());
		if (!result)
		{
			lua_pushnil(L);
			return 1;
		}

		lua_pushinteger(L, *result);
		return 1;
	}

	static int GetCommands(lua_State* L)
	{
		// {"disassemble": {"disassemble", "disas", "disasm"}, ...}
		std::map<std::string, std::vector<std::string>> aliases;
		// {"disas": {"name":"disassemble", "help": "helphere"}, ...}
		auto commands = g_dbg->GetLuaManager()->GetCommands();
		
		// grab all aliases
		for (auto command : commands)
		{
			const std::string& alias = command.first;
			const std::string& name = command.second["name"];
			aliases[name].push_back(alias);
		}

		// create table (dictionary)
		lua_createtable(L, 0, aliases.size());

		for (auto& command : aliases)
		{
			const std::string& name = command.first;
			const std::string& help = commands[name]["help"];

			// child table (Command)
			lua_createtable(L, 0, 3); // name, help, alias

			// set "name"
			lua_pushstring(L, name.c_str());
			lua_setfield(L, -2, "name");

			// set "help"
			lua_pushstring(L, help.c_str());
			lua_setfield(L, -2, "help");

			// child table (aliases)
			lua_createtable(L, command.second.size(), 0);
			for (size_t j = 0; j < command.second.size(); j++)
			{
				lua_pushstring(L, command.second[j].c_str());
				lua_rawseti(L, -2, j + 1);
			}
			lua_setfield(L, -2, "alias");

			// set into dictionary by command name
			lua_setfield(L, -2, name.c_str());
		}

		return 1;
	}

	static int GetContext32(lua_State* L)
	{
		const std::set<const char*> regs = {
			"eax","ebx","ecx","edx","esi","edi","eip","esp","ebp"
		};

		auto result = g_dbg->GetRegisters(regs);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		lua_createtable(L, 0, regs.size());
		auto registers = *result;
		for (auto pair : registers)
			setfieldi(L, pair.first.c_str(), pair.second);

		return 1;
	}

	static int GetContext64(lua_State* L)
	{
		const std::set<const char*> regs = {
			"rax","rbx","rcx","rdx","rsi","rdi","rip","rsp","rbp",
			"r8","r9","r10","r11","r12","r13","r14","r15"
		};

		auto result = g_dbg->GetRegisters(regs);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		lua_createtable(L, 0, regs.size());
		auto registers = *result;
		for (auto pair : registers)
			setfieldi(L, pair.first.c_str(), pair.second);
		
		return 1;
	}


	static int GetVMRegion(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);

		MEMORY_BASIC_INFORMATION64 mbi = { 0 };

		g_dbg->QueryVM(address, &mbi);
		MemoryRegion region(&mbi);

		lua_createtable(L, 0, 4);

		lua_pushinteger(L, region.BaseAddress());
		lua_setfield(L, -2, "baseaddress");

		lua_pushinteger(L, region.Protections());
		lua_setfield(L, -2, "protections");

		lua_pushinteger(L, region.Size());
		lua_setfield(L, -2, "size");

		lua_pushinteger(L, region.State());
		lua_setfield(L, -2, "state");

		return 1;
	}

	static int GetVMRegions(lua_State* L)
	{
		MEMORY_BASIC_INFORMATION64 mbi = { 0 };
		std::vector<MemoryRegion*> regions(0);

		g_dbg->QueryVM(0, &mbi);
		while (g_dbg->QueryVM(mbi.BaseAddress + mbi.RegionSize, &mbi))
		{
			if (mbi.State & MEM_COMMIT)
				regions.push_back(new MemoryRegion(&mbi));
		}

		MemoryRegion::CreateMemoryRegionTable(L, regions);

		for (auto region : regions)
			delete region;

		return 1;
	}

	static int Is64BitTarget(lua_State* L)
	{
		int value = g_dbg->Is64BitTarget();
		lua_pushboolean(L, value);
		return 1;
	}

	static int StepInto(lua_State* L)
	{
		g_dbg->SetState(DE::State::STEP_INTO);
		return 0;
	}

	static int StepOver(lua_State* L)
	{
		g_dbg->SetState(DE::State::STEP_OVER);
		return 0;
	}

	static int ReadMemory(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);
		ULONG len = (ULONG)luaL_checkinteger(L, 2);

		unsigned char* out = (unsigned char*)calloc(len, 1);
		auto result = g_dbg->ReadVMUncached(address, &len, out);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			free(out);
			return 2;
		}

		lua_pushlstring(L, (const char*)out, len);
		free(out);
		return 1;
	}

	static int WriteMemory(lua_State* L)
	{
		size_t address = luaL_checkinteger(L, 1);
		ULONG len = (ULONG)luaL_checkinteger(L, 2);
		size_t literal_len = 0;
		const char* data = luaL_checklstring(L, 3, &literal_len);

		if (len > literal_len)
		{
			lua_pushnil(L);
			luaL_error(L, "len > provided data length");
			return 2;
		}

		auto result = g_dbg->WriteVMUncached(address, &len, (unsigned char*)data);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		return 0;
	}

	static int SymbolNameToSymbol(lua_State* L)
	{
		const char* symname = luaL_checkstring(L, 1);
		if (symname == LUA_TNIL)
		{
			lua_pushnil(L);
			luaL_error(L, "symbol name must be provided");
			return 2;
		}

		auto result = g_dbg->GetSymbolManager()->SymbolFromName(symname);
		if (!result)
		{
			lua_pushnil(L);
			luaL_error(L, result.error().c_str());
			return 2;
		}

		auto symbol = *result;
		lua_createtable(L, 0, 5);

		lua_pushinteger(L, symbol->Address());
		lua_setfield(L, -2, "address");
		lua_pushinteger(L, symbol->Displacement());
		lua_setfield(L, -2, "displacement");
		lua_pushinteger(L, symbol->Flags());
		lua_setfield(L, -2, "flags");
		lua_pushinteger(L, symbol->ModBase());
		lua_setfield(L, -2, "modbase");
		lua_pushlstring(L, symbol->Name(), strlen(symbol->Name()));
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, symbol->Size());
		lua_setfield(L, -2, "size");

		return 1;
	}
}