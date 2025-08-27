#pragma once
#include <expected>
#include <print>
#include <windows.h>
#include <DbgHelp.h>

namespace gdbw
{
	class Symbol
	{
	public:
		Symbol(PSYMBOL_INFO syminfo, DWORD64 displacement);
		~Symbol();

		inline DWORD64 Address(void) { return m_syminfo->Address; }
		inline DWORD64 Displacement(void) { return m_displacement; }
		inline ULONG Flags(void) { return m_syminfo->Flags; }
		inline ULONG64 ModBase(void) { return m_syminfo->ModBase; }
		inline CHAR* Name(void) { return m_syminfo->Name; }
		inline ULONG Size(void) { return m_syminfo->Size; }
	private:
		PSYMBOL_INFO m_syminfo = nullptr;
		DWORD64 m_displacement = 0;
	};

	class SymbolManager
	{
	public:
		SymbolManager() = default;
		~SymbolManager();

		std::expected<bool, std::string> Init(HANDLE debuggee);
		std::expected<Symbol*, std::string> SymbolFromAddress(DWORD64 address);
		std::expected<Symbol*, std::string> SymbolFromName(PCSTR name);
		std::expected<bool, std::string> RefreshModuleList(void);
	private:
		HANDLE m_hdebuggee;
	};
}


