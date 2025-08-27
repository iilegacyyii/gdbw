#pragma once
#include <expected>
#include <map>
#include <set>
#include <string>
#include <print>
#include <DbgEng.h>
#include "LuaManager.hpp"
#include "Symbols.hpp"

#define RTN_IF_ERR_HR(hr, funcname) if (FAILED(hr)) return std::unexpected(std::format(funcname " failed with hr={:#x}", hr))

namespace gdbw {};
namespace gdbw::DE {};

namespace gdbw::DE
{
	enum class State
	{
		NONE = 0,
		RUN,
		SUSPEND,
		RESUME,
		STEP_INTO,
		STEP_OVER,
		STOP
	};

	class EventCallbacks : public DebugBaseEventCallbacks
	{
	public:
		virtual ~EventCallbacks() { Release(); }

		ULONG STDMETHODCALLTYPE AddRef() override
		{
			return 1;
		}

		ULONG STDMETHODCALLTYPE Release() override
		{
			return 0;
		}

		HRESULT GetInterestMask(ULONG* mask) override
		{
			*mask = DEBUG_EVENT_BREAKPOINT
				| DEBUG_EVENT_EXCEPTION
				| DEBUG_EVENT_CREATE_THREAD
				| DEBUG_EVENT_EXIT_THREAD
				| DEBUG_EVENT_CREATE_PROCESS
				| DEBUG_EVENT_EXIT_PROCESS
				| DEBUG_EVENT_LOAD_MODULE
				| DEBUG_EVENT_UNLOAD_MODULE
				| DEBUG_EVENT_SYSTEM_ERROR
				| DEBUG_EVENT_SESSION_STATUS
				| DEBUG_EVENT_CHANGE_DEBUGGEE_STATE
				| DEBUG_EVENT_CHANGE_ENGINE_STATE
				| DEBUG_EVENT_CHANGE_SYMBOL_STATE;

			return S_OK;
		}

		HRESULT Breakpoint(PDEBUG_BREAKPOINT bp) override
		{
			ULONG id = 0;
			auto hr = bp->GetId(&id);
			// Just want to warn, not exit. This should never be hit - if it is there's
			// an issue with the [Add/Remove]Breakpoint bindings
			if (FAILED(hr)) std::println("Warning: Failed to get breakpoint id, hr={:#x}", hr);
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT Exception(PEXCEPTION_RECORD64 exception, ULONG fistchance) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT CreateThread(ULONG64 handle, ULONG64 dataoffset, ULONG64 startoffset) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT ExitThread(ULONG exitcode) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT CreateProcess(
			ULONG64 imagehandle, ULONG64 handle, ULONG64 baseoffset, ULONG modulesize,
			PCSTR modulename, PCSTR imagename, ULONG checksum, ULONG timestamp,
			ULONG64 initialthreadhandle, ULONG64 threaddataoffset, ULONG64 startoffset) override
		{
			return DEBUG_STATUS_GO;
		}

		HRESULT ExitProcess(ULONG exitcode) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT LoadModule(
			ULONG64 imagehandle, ULONG64 baseoffset, ULONG modulesize,
			PCSTR ModuleName, PCSTR imagename, ULONG checksum, ULONG timestamp) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT UnloadModule(PCSTR imagename, ULONG64 baseoffset) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT SystemError(ULONG error, ULONG level) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT SessionStatus(ULONG status) override
		{
			return S_OK;
		}

		HRESULT ChangeDebuggeeState(ULONG flags, ULONG64 argument) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}

		HRESULT ChangeEngineState(ULONG flags, ULONG64 argument) override
		{
			return 0;
		}

		HRESULT ChangeSymbolState(ULONG flags, ULONG64 argument) override
		{
			return DEBUG_STATUS_NO_CHANGE;
		}
	}; // end of EventCallbacks class
	
	class IOCallbacks : public IDebugInputCallbacks, public IDebugOutputCallbacks
	{
	public:
		IOCallbacks() {}

		virtual ~IOCallbacks() {
			Release();
		}

		ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
		ULONG STDMETHODCALLTYPE Release() override { return 0; }

		HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, PVOID* pinterface) override
		{
			if (IsEqualIID(iid, __uuidof(IDebugInputCallbacks)))
			{
				*pinterface = static_cast<IDebugInputCallbacks*>(this);
				AddRef();
				return S_OK;
			}
			if (IsEqualIID(iid, __uuidof(IDebugOutputCallbacks)))
			{
				*pinterface = static_cast<IDebugOutputCallbacks*>(this);
				AddRef();
				return S_OK;
			}
			return E_NOINTERFACE;
		}

		HRESULT Output(ULONG mask, PCSTR text) override
		{
			return S_OK;
		}

		HRESULT StartInput(ULONG bufsize) override
		{
			return S_OK;
		}

		HRESULT EndInput() override
		{
			return S_OK;
		}
	};

	class Engine
	{
	public:
		Engine() = default;
		~Engine();
		// Initialize debugger, Constructor does not do this!
		std::expected<bool, std::string> Init(gdbw::LuaManager* lua);
		// Attach to a process given a pid
		std::expected<bool, std::string> Attach(DWORD pid, bool break_on_entry = true);
		// Create and attach to a new process given a command line
		std::expected<bool, std::string> CreateAndAttach(PSTR commandline, bool break_on_entry = true);
		// Enter debug loop
		std::expected<bool, std::string> EnterDebugLoop(void);

		//
		// Useful functions for bindings
		//

		// Set current state, useful for stepinto, stepover etc.
		inline void SetState(State s) { m_state = s; }
		// Get a pointer to the lua manager
		inline LuaManager* GetLuaManager(void) { return m_lua; }
		// Get a pointer to the symbol manager
		inline SymbolManager* GetSymbolManager(void) { return m_symmanager; }
		// Get breakpoints
		inline std::vector<PDEBUG_BREAKPOINT> GetBreakpoints(void) { return m_breakpoints; }
		// Check if debuggee is 64bit. Returns true if so
		inline bool Is64BitTarget(void) { return m_debuggeebitness == 64; }

		// Get a module name from its base address
		std::expected<std::string, std::string> AddressToModule(ULONG64 address);
		// Add a breakpoint
		std::expected<ULONG, std::string> BreakpointAdd(size_t address);
		// Remove a breakpoint
		std::expected<bool, std::string> BreakpointRemove(size_t id);
		// Evaluate an expression (windbg format)
		std::expected<ULONG64, std::string> Evaluate(PSTR expression);
		// Set an interrupt, useful for breaking into the debugger
		std::expected<bool, std::string> Interrupt(ULONG flags);
		// Get all registers
		std::expected<std::map<std::string, size_t>, std::string> GetRegisters(std::set<const char*> regs);
		// Query virtual memory
		std::expected<bool, std::string> QueryVM(ULONG64 address, PMEMORY_BASIC_INFORMATION64 mbi);
		// Read virtual memory (uncached)
		std::expected<bool, std::string> ReadVMUncached(ULONG64 address, PULONG len, PVOID out);
		// Write virtual memory (uncached)
		std::expected<bool, std::string> WriteVMUncached(ULONG64 address, PULONG len, PVOID in);
	private:
		// Handle a single iteration of the debug loop (including prompt)
		// Returns false if debugger should detach and exit.
		// Rf firstevent is true, further engine initialisation will take place after the first WaitForEvent call.
		std::expected<bool, std::string> WaitAndHandleDebugEvent(bool firstevent);
		// To be called upon first attach, gets target information to be used in commands.
		std::expected<bool, std::string> HandleFirstEvent();
		State m_state = State::NONE;
		HANDLE m_hdebuggee = INVALID_HANDLE_VALUE;
		uint8_t m_debuggeebitness = 0;
		std::vector<PDEBUG_BREAKPOINT> m_breakpoints;
		LuaManager* m_lua = nullptr;
		SymbolManager* m_symmanager = nullptr; // Initialized in EnterDebugLoop since we need a handle
		IDebugClient* m_client = nullptr;
		IDebugControl3* m_control = nullptr;
		IDebugRegisters2* m_registers = nullptr;
		IDebugSymbols3* m_symbols = nullptr;
		IDebugDataSpaces2* m_dataspaces = nullptr;
		IDebugSystemObjects4* m_systemobjects = nullptr;
		EventCallbacks* m_eventcallbacks = nullptr;
		IOCallbacks* m_iocallbacks = nullptr;
	};
}


