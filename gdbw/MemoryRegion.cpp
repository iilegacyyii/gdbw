#include "MemoryRegion.hpp"

MemoryRegion::MemoryRegion(PMEMORY_BASIC_INFORMATION64 mbi)
{
	m_baseaddress = mbi->BaseAddress;
	m_protections = mbi->Protect;
	m_size = mbi->RegionSize;
	m_state = mbi->State;
}

MemoryRegion::~MemoryRegion()
{
}

void MemoryRegion::CreateMemoryRegionTable(lua_State* L, std::vector<MemoryRegion*> regions)
{
	// create parent table
	lua_createtable(L, regions.size(), 0);

	for (size_t i = 0; i < regions.size(); i++)
	{
		// child table (Instruction)
		lua_createtable(L, 0, 4);

		lua_pushinteger(L, regions[i]->BaseAddress());
		lua_setfield(L, -2, "baseaddress");

		lua_pushinteger(L, regions[i]->Protections());
		lua_setfield(L, -2, "protections");

		lua_pushinteger(L, regions[i]->Size());
		lua_setfield(L, -2, "size");

		lua_pushinteger(L, regions[i]->State());
		lua_setfield(L, -2, "state");

		// Child index
		lua_rawseti(L, -2, i + 1);
	}
}
