#include "Symbols.hpp"

gdbw::SymbolManager::~SymbolManager()
{
	SymCleanup(m_hdebuggee);
	CloseHandle(m_hdebuggee);
}

std::expected<bool, std::string> gdbw::SymbolManager::Init(HANDLE debuggee)
{
	// We should duplicate the handle to ensure it is unique
	// https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-syminitialize
	if (!DuplicateHandle(
		GetCurrentProcess(), debuggee, GetCurrentProcess(), &m_hdebuggee,
		NULL, FALSE, DUPLICATE_SAME_ACCESS))
	{
		return std::unexpected(std::format("Error duplicating debugge handle for symbol resolution ({:#x})", GetLastError()));
	}
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES);
	
	if (!SymInitialize(m_hdebuggee, NULL, TRUE))
		return std::unexpected(std::format("Error during SymInitialize: ({:#x})", GetLastError()));

	return true;
}

std::expected<gdbw::Symbol*, std::string> gdbw::SymbolManager::SymbolFromAddress(DWORD64 address)
{
	DWORD64 displacement = 0;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
	PSYMBOL_INFO syminfo = (PSYMBOL_INFO)buffer;
	syminfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	syminfo->MaxNameLen = MAX_SYM_NAME;

	if (!SymFromAddr(m_hdebuggee, address, &displacement, syminfo))
		return std::unexpected(std::format("SymFromAddr failed with code ({:#x})", GetLastError()));

	// often occurs, not sure why. So try and get it again via SymGetModuleBase
	if (syminfo->ModBase == 0)
		syminfo->ModBase = SymGetModuleBase64(m_hdebuggee, address);

	Symbol* symbol = new Symbol(syminfo, displacement);
	return symbol;
}

std::expected<gdbw::Symbol*, std::string> gdbw::SymbolManager::SymbolFromName(PCSTR name)
{
	ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
	PSYMBOL_INFO syminfo = (PSYMBOL_INFO)buffer;

	syminfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	syminfo->MaxNameLen = MAX_SYM_NAME;

	if (!SymFromName(m_hdebuggee, name, syminfo))
		return std::unexpected(std::format("SymFromAddr failed with code ({:#x})", GetLastError()));

	auto result = SymbolFromAddress(syminfo->Address);
	if (!result)
		return std::unexpected(result.error());

	return *result;
}

std::expected<bool, std::string> gdbw::SymbolManager::RefreshModuleList(void)
{
	if (!SymRefreshModuleList(m_hdebuggee))
		return std::unexpected(std::format("SymRefreshModuleList failed with code ({:#x})", GetLastError()));
	return true;
}

gdbw::Symbol::Symbol(PSYMBOL_INFO syminfo, DWORD64 displacement)
{
	m_displacement = displacement;
	m_syminfo = (PSYMBOL_INFO)calloc(sizeof(SYMBOL_INFO) + MAX_SYM_NAME, 1);
	memcpy(m_syminfo, syminfo, sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
}

gdbw::Symbol::~Symbol()
{
	if (m_syminfo)
		free(m_syminfo);
}
