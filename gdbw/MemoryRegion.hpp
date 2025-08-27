#pragma once
#include <Windows.h>
#include <expected>
#include <print>
#include "LuaManager.hpp"

class MemoryRegion
{
public:
	MemoryRegion(PMEMORY_BASIC_INFORMATION64 mbi);
	~MemoryRegion();

	static void CreateMemoryRegionTable(lua_State* L, std::vector<MemoryRegion*> regions);

	inline ULONG64 BaseAddress(void) const { return m_baseaddress; }
	inline DWORD Protections(void) const { return m_protections; }
	inline DWORD State(void) const { return m_state; }
	inline ULONG64 Size(void) const { return m_size; }
private:
	ULONG64 m_baseaddress = 0;
	DWORD m_protections = 0;
	DWORD m_state = 0;
	ULONG64 m_size = 0;
};

