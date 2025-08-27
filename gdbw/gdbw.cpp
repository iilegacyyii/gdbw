#include "DebugEngine.hpp"
#include "Bindings.hpp"
#include "thirdparty/argparse/argparse.hpp"

gdbw::DE::Engine* g_dbg;

argparse::ArgumentParser* parse_args(int argc, char** argv)
{
	auto parser = new argparse::ArgumentParser("gdbw", "0.1.0");
	parser->add_description("gdb for windows 'but scriptable' by (0xLegacyy & Zopazz)");
	// Add attach and file arguments (mutually exclusive and at least one is required)
	auto& group = parser->add_mutually_exclusive_group(true);
	group.add_argument("-a", "--attach")
		.help("attach to a process via pid (e.g. 12004)")
		.metavar("pid");
	group.add_argument("-f", "--file")
		.help("debug a binary on disk (e.g. C:\\tmp\\DebugMe.exe)")
		.metavar("path");

	try
	{
		parser->parse_args(argc, argv);
	}
	catch (const std::exception& err)
	{
		std::println("Error parsing arguments: {}", err.what());
		exit(0);
	}
	return parser;
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT)
	{
		auto result = g_dbg->Interrupt(DEBUG_INTERRUPT_ACTIVE);
		if (!result)
			std::println("Error breaking: {}", result.error());
		return TRUE;
	}
	return FALSE;
}

int main(int argc, char** argv)
{
	auto args = parse_args(argc, argv);

	auto lua = new gdbw::LuaManager();

	// Register bindings
	lua->RegisterGlobalFunction(gdbw::bindings::AddressToModuleName, "AddressToModuleName");
	lua->RegisterGlobalFunction(gdbw::bindings::AddressToSymbol, "AddressToSymbol");
	lua->RegisterGlobalFunction(gdbw::bindings::BreakpointAdd, "BreakpointAdd");
	lua->RegisterGlobalFunction(gdbw::bindings::BreakpointRemove, "BreakpointRemove");
	lua->RegisterGlobalFunction(gdbw::bindings::BreakpointGetAll, "BreakpointGetAll");
	lua->RegisterGlobalFunction(gdbw::bindings::ConsoleCols, "ConsoleCols");
	lua->RegisterGlobalFunction(gdbw::bindings::ConsoleRows, "ConsoleRows");
	lua->RegisterGlobalFunction(gdbw::bindings::Continue, "Continue");
	lua->RegisterGlobalFunction(gdbw::bindings::Disassemble, "Disassemble");
	lua->RegisterGlobalFunction(gdbw::bindings::Evaluate, "Evaluate");
	lua->RegisterGlobalFunction(gdbw::bindings::Is64BitTarget, "Is64BitTarget");
	lua->RegisterGlobalFunction(gdbw::bindings::GetCommands, "GetCommands");
	lua->RegisterGlobalFunction(gdbw::bindings::GetContext32, "GetContext32");
	lua->RegisterGlobalFunction(gdbw::bindings::GetContext64, "GetContext64");
	lua->RegisterGlobalFunction(gdbw::bindings::GetVMRegions, "GetVMRegions");
	lua->RegisterGlobalFunction(gdbw::bindings::ReadMemory, "ReadMemory");
	lua->RegisterGlobalFunction(gdbw::bindings::StepInto, "StepInto");
	lua->RegisterGlobalFunction(gdbw::bindings::StepOver, "StepOver");
	lua->RegisterGlobalFunction(gdbw::bindings::WriteMemory, "WriteMemory");
	lua->RegisterGlobalFunction(gdbw::bindings::SymbolNameToSymbol, "SymbolNameToSymbol");

	g_dbg = new gdbw::DE::Engine();
	
	auto init_result = g_dbg->Init(lua);
	if (!init_result)
	{
		std::println("Error during Engine.Init: {}", init_result.error());
		return 1;
	}

	// attach
	if (auto attach = args->present("-a"))
	{
		auto attach_result = g_dbg->Attach(std::stoi(*attach));
		if (!attach_result)
		{
			std::println("Error during Engine.Attach: {}", attach_result.error());
			return 2;
		}
	}
	else // --file
	{
		auto file = args->present("-f");
		// TODO: add --no-entrybreak flag
		auto attach_result = g_dbg->CreateAndAttach((PSTR)file->c_str());
		if (!attach_result)
		{
			std::println("Error during Engine.CreateAndAttach: {}", attach_result.error());
			return 3;
		}
	}

	// Set Ctrl+C handler
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
		std::println("Warning: could not register CTRL+C handler");

	auto debug_result = g_dbg->EnterDebugLoop();
	if (!debug_result)
	{
		std::println("Error during Engine.EnterDebugLoop: {}", debug_result.error());
		return 4;
	}

	return 0;
}