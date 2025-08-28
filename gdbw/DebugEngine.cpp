#include "DebugEngine.hpp"

gdbw::DE::Engine::~Engine()
{
	// remove breakpoints (only freed once RemoveBreakpoint is called)
	for (auto bp : m_breakpoints)
	{
		if (bp != nullptr)
			m_control->RemoveBreakpoint(bp);
	}

	// end debug session
	if (m_client)
		m_client->EndSession(DEBUG_END_ACTIVE_TERMINATE);

	// release interfaces (other than client)
	if (m_control)
	{
		m_control->Release();
		m_control = nullptr;
	}
	if (m_registers)
	{
		m_registers->Release();
		m_registers = nullptr;
	}
	if (m_symbols)
	{
		m_symbols->Release();
		m_symbols = nullptr;
	}
	if (m_dataspaces)
	{
		m_dataspaces->Release();
		m_dataspaces = nullptr;
	}
	if (m_systemobjects)
	{
		m_systemobjects->Release();
		m_systemobjects = nullptr;
	}

	// remove callbacks
	if (m_eventcallbacks)
	{
		m_client->SetEventCallbacks(nullptr);
		delete m_eventcallbacks;
	}
	if (m_iocallbacks)
	{
		m_client->SetInputCallbacks(nullptr);
		m_client->SetOutputCallbacks(nullptr);
		delete m_iocallbacks;
	}

	if (m_symmanager)
		delete m_symmanager;

	// release client
	if (m_client)
	{
		m_client->Release();
		m_client = nullptr;
	}

	// remove lua reference
	if (m_lua) m_lua = nullptr;

	if (m_hdebuggee)
		CloseHandle(m_hdebuggee);
}

std::expected<bool, std::string> gdbw::DE::Engine::Init(gdbw::LuaManager* lua)
{
	m_lua = lua;

	m_breakpoints = std::vector<PDEBUG_BREAKPOINT>(0);

	HRESULT hr;
	hr = DebugCreate(__uuidof(IDebugClient), (void**)&m_client);
	RTN_IF_ERR_HR(hr, "DebugCreate");

	hr = m_client->QueryInterface(__uuidof(IDebugControl3), (void**)&m_control);
	RTN_IF_ERR_HR(hr, "QueryInterface[IDebugControl3]");

	hr = m_client->QueryInterface(__uuidof(IDebugRegisters2), (void**)&m_registers);
	RTN_IF_ERR_HR(hr, "QueryInterface[IDebugRegisters]");

	hr = m_client->QueryInterface(__uuidof(IDebugSymbols3), (void**)&m_symbols);
	RTN_IF_ERR_HR(hr, "QueryInterface[IDebugSymbols3]");

	hr = m_client->QueryInterface(__uuidof(IDebugDataSpaces2), (void**)&m_dataspaces);
	RTN_IF_ERR_HR(hr, "QueryInterface[IDebugDataSpaces2]");

	hr = m_client->QueryInterface(__uuidof(IDebugSystemObjects4), (void**)&m_systemobjects);
	RTN_IF_ERR_HR(hr, "QueryInterface[IDebugSystemObjects4]");

	// Setup client event callbacks
	m_eventcallbacks = new EventCallbacks();
	hr = m_client->SetEventCallbacks(m_eventcallbacks);
	RTN_IF_ERR_HR(hr, "SetEventCallbacks");
	
	// Setup io callbacks
	m_iocallbacks = new IOCallbacks();
	hr = m_client->SetInputCallbacks(m_iocallbacks);
	RTN_IF_ERR_HR(hr, "SetInputCallbacks");
	
	hr = m_client->SetOutputCallbacks(m_iocallbacks);
	RTN_IF_ERR_HR(hr, "SetOutputCallbacks");
	
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::Attach(DWORD pid, bool break_on_entry)
{
	HRESULT hr;
	if (break_on_entry)
	{
		hr = m_control->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
		RTN_IF_ERR_HR(hr, "IDebugControl[AddEngineOptions]");
	}

	hr = m_client->AttachProcess(NULL, pid, NULL);
	RTN_IF_ERR_HR(hr, "IDebugClient[AttachProcess]");

	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::CreateAndAttach(PSTR commandline, bool break_on_entry)
{
	HRESULT hr;
	if (break_on_entry)
	{
		hr = m_control->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
		RTN_IF_ERR_HR(hr, "IDebugControl[AddEngineOptions]");
	}

	ULONG flags = DEBUG_ONLY_THIS_PROCESS;
	hr = m_client->CreateProcessAndAttach(NULL, commandline, flags, NULL, NULL);
	RTN_IF_ERR_HR(hr, "IDebugClient[CreateProcessAndAttach]");

	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::EnterDebugLoop(void)
{
	auto result = WaitAndHandleDebugEvent(true);
	if (!result) return std::unexpected(result.error());

	while (*result)
	{
		result = WaitAndHandleDebugEvent(false);
		if (!result) return std::unexpected(result.error());
	}
	return true;
}

std::expected<std::string, std::string> gdbw::DE::Engine::AddressToModule(ULONG64 address)
{
	ULONG64 modbase = 0;
	auto hr = m_symbols->GetModuleByOffset(address, 0, NULL, &modbase);
	RTN_IF_ERR_HR(hr, "Could not locate module containing the specified address");
	char imagename[256] = { 0 };
	char loadedname[256] = { 0 };
	hr = m_symbols->GetModuleNames(DEBUG_ANY_ID, modbase, imagename, 256, 0, NULL, 0, NULL, loadedname, 256, 0);
	if (hr != S_OK && hr != S_FALSE)
		return std::unexpected("Failed to retrieve specified module name");
	std::string result = loadedname[0] ? loadedname : imagename;
	return result;
}

std::expected<ULONG, std::string> gdbw::DE::Engine::BreakpointAdd(size_t address)
{
	PDEBUG_BREAKPOINT bp = nullptr;
	ULONG desired_id = m_breakpoints.size();
	
	auto hr = m_control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, desired_id, &bp);
	RTN_IF_ERR_HR(hr, "IDebugControl->AddBreakpoint");
	m_breakpoints.push_back(bp);

	hr = bp->SetOffset(address);
	RTN_IF_ERR_HR(hr, "IDebugBreakpoint->SetOffset[address]");
	hr = bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);
	RTN_IF_ERR_HR(hr, "IDebugBreakpoint->AddFlags[DEBUG_BREAKPOINT_ENABLED]");
	return desired_id;
}

std::expected<bool, std::string> gdbw::DE::Engine::BreakpointSetFlags(size_t id, ULONG flags)
{
	if (id >= m_breakpoints.size()
		|| id < 0
		|| m_breakpoints[id] == nullptr)
		return std::unexpected("Invalid breakpoint id");

	auto hr = m_breakpoints[id]->SetFlags(flags);
	RTN_IF_ERR_HR(hr, "IDebugBreakpoint->SetFlags");
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::BreakpointRemove(size_t id)
{
	if (id >= m_breakpoints.size()
		|| id < 0
		|| m_breakpoints[id] == nullptr)
		return std::unexpected("Invalid breakpoint id");

	auto hr = m_control->RemoveBreakpoint(m_breakpoints[id]);
	RTN_IF_ERR_HR(hr, "IDebugControl->RemoveBreakpoint");
	m_breakpoints[id] = nullptr;
	return true;
}

std::expected<ULONG64, std::string> gdbw::DE::Engine::Evaluate(PSTR expression)
{
	DEBUG_VALUE val = { 0 };
	ULONG flags = 0;
	m_control->GetExpressionSyntax(&flags);
	auto hr = m_control->Evaluate(expression, DEBUG_VALUE_INT64, &val, NULL);
	RTN_IF_ERR_HR(hr, "IDebugControl[Evaluate]");
	return val.I64;
}

std::expected<bool, std::string> gdbw::DE::Engine::Interrupt(ULONG flags)
{
	auto hr = m_control->SetInterrupt(flags);
	RTN_IF_ERR_HR(hr, "Engine.Interrupt[SetInterrupt]");
	return true;
}

std::expected<std::map<std::string, size_t>, std::string> gdbw::DE::Engine::GetRegisters(std::set<const char*> regs)
{
	std::map<std::string, size_t> registers;
	DEBUG_VALUE val = { 0 };
	ULONG idx = 0;

	for (auto name : regs)
	{
		RTN_IF_ERR_HR(m_registers->GetIndexByName(name, &idx), "Engine.GetContext");
		RTN_IF_ERR_HR(m_registers->GetValue(idx, &val), "Engine.GetContext");
		registers[name] = val.I64;
	}
	return registers;
}

std::expected<bool, std::string> gdbw::DE::Engine::QueryVM(ULONG64 address, PMEMORY_BASIC_INFORMATION64 mbi)
{
	auto hr = m_dataspaces->QueryVirtual(address, mbi);
	RTN_IF_ERR_HR(hr, "IDebugDataSpaces2[QueryVirtual]");
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::ReadVMUncached(ULONG64 address, PULONG len, PVOID out)
{
	ULONG bytesread = 0;
	auto hr = m_dataspaces->ReadVirtualUncached(address, out, *len, &bytesread);
	RTN_IF_ERR_HR(hr, "Engine.ReadVMUncached");
	if (bytesread != *len)
		*len = bytesread;
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::WriteVMUncached(ULONG64 address, PULONG len, PVOID in)
{
	ULONG byteswritten = 0;
	auto hr = m_dataspaces->WriteVirtualUncached(address, in, *len, &byteswritten);
	RTN_IF_ERR_HR(hr, "Engine.WriteVMUncached");
	if (byteswritten != *len)
		*len = byteswritten;
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::WaitAndHandleDebugEvent(bool firstevent)
{
	// Always running until WaitForEvent returns
	m_state = State::RUN;

	// Wait for event e.g. bp/user interrupt/...
	auto hr = m_control->WaitForEvent(0, INFINITE);

	// Get execution status (ensure debugee is running etc.)
	ULONG exec_status = 0;
	hr = m_control->GetExecutionStatus(&exec_status);
	RTN_IF_ERR_HR(hr, "GetExecutionStatus");

	// We're no longer attached
	if (exec_status == DEBUG_STATUS_NO_DEBUGGEE)
		return false;

	if (hr == E_PENDING) // Exit interrupt was issued. Target not available
		return false;
	else if (hr != S_OK) // If wait for event failed, and we're still attached, unexpected error.
		return std::unexpected("Engine error: unknown execution status occured");

	// Handle first event if necessary
	if (firstevent)
	{
		auto firstevent_result = HandleFirstEvent();
		if (!firstevent_result)
			return std::unexpected(firstevent_result.error());
	}

	// Sync the executing processor type w/effective...
	// this way if syswow is in play we will be able to use
	// 32-bit addrs for 32-bit parts and vice versa
	ULONG executing_type = 0;
	hr = m_control->GetExecutingProcessorType(&executing_type);
	RTN_IF_ERR_HR(hr, "IDebugControl[GetExecutingProcessorType]");
	hr = m_control->SetEffectiveProcessorType(executing_type);
	RTN_IF_ERR_HR(hr, "IDebugControl[SetEffectiveProcessorType]");
	uint8_t old_bitness = m_debuggeebitness;
	m_debuggeebitness = executing_type == IMAGE_FILE_MACHINE_AMD64 ? 64 : 32;

	if (m_debuggeebitness != old_bitness)
		m_symmanager->RefreshModuleList();

	// For any break status event the debugger is suspended, so run the prompt
	if (exec_status == DEBUG_STATUS_BREAK)
	{
		m_state = State::SUSPEND;
		while (m_state == State::SUSPEND)
			if (m_lua->Prompt()) break;

		ULONG new_status = DEBUG_STATUS_GO;
		if (m_state == State::STEP_INTO) new_status = DEBUG_STATUS_STEP_INTO;
		else if (m_state == State::STEP_OVER) new_status = DEBUG_STATUS_STEP_OVER;
		else if (m_state == State::STOP) return false;

		hr = m_control->SetExecutionStatus(new_status);
		RTN_IF_ERR_HR(hr, "SetExecutionStatus");
	}
	return true;
}

std::expected<bool, std::string> gdbw::DE::Engine::HandleFirstEvent()
{	
	// Now we're attached - initialize the symbol manager
	if (m_symmanager == nullptr)
	{
		// Get handle to debuggee (need this for bindings & duplicating for sym resolution)
		ULONG pid = -1;
		auto hr = m_systemobjects->GetCurrentProcessSystemId(&pid);
		RTN_IF_ERR_HR(hr, "IDebugSystemObjects4[GetCurrentProcessSystemId]");
		m_hdebuggee = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (!m_hdebuggee)
			return std::unexpected("Failed to open handle to target process");

		// Initialize symbol manager
		m_symmanager = new SymbolManager();
		auto symmanager_result = m_symmanager->Init(m_hdebuggee);
		if (!symmanager_result)
			return std::unexpected(symmanager_result.error());
	}

	// set evaluation base to 10, rather than 16
	// This prevents issues such as `main+35` evaluating to `main+0x35`
	m_control->SetRadix(10);

	// TODO: since we're doing this anyways for expression evaluation,
	// ...we may as well move all symbol management to `m_symbols`? 
	m_control->Execute(DEBUG_OUTCTL_IGNORE, ".reload /f", DEBUG_EXECUTE_NOT_LOGGED);
	return true;
}
