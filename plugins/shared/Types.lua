---@meta

---@class Breakpoint Registered breakpoint information
---@field id integer breakpoint id
---@field address integer breakpoint address
---@field enabled boolean true if breakpoint enabled

---@class Command Registered debugger command
---@field name string command name (e.g. disassemble)
---@field alias table command alias(es) (e.g. {"disas","disassemble"})
---@field help string help string

---@class Context32 Register values for a thread
---@field eax integer
---@field ebx integer
---@field ecx integer
---@field edx integer
---@field esi integer
---@field edi integer
---@field eip integer
---@field esp integer
---@field ebp integer

---@class Context64 Register values for a thread
---@field rax integer
---@field rbx integer
---@field rcx integer
---@field rdx integer
---@field rsi integer
---@field rdi integer
---@field rip integer
---@field rsp integer
---@field rbp integer
---@field r8 integer
---@field r9 integer
---@field r10 integer
---@field r11 integer
---@field r12 integer
---@field r13 integer
---@field r14 integer
---@field r15 integer

---@class Instruction Defines a single disassembled instruction
---@field address integer
---@field mnemonic string
---@field opstr string
---@field bytes string
---@field size integer

---@class MemoryRegion Defines a single memory region
---@field baseaddress integer
---@field protections integer
---@field size integer
---@field state integer

---@class Symbol Defines a single symbol (e.g. a function)
---@field address integer
---@field displacement integer
---@field flags integer
---@field modbase integer
---@field name string
---@field size integer

-- # Global Functions

---Get a module name from a given address
---@param address integer
---@return string
function AddressToModuleName(address) end

---Get a symbol from a given address
---@param address integer
---@return Symbol
function AddressToSymbol(address) end

---Add a software breakpoint
---@param address integer breakpoint address
---@return integer breakpoint id
function BreakpointAdd(address) end

---Get all registered breakpoints
---@return [Breakpoint] Array of breakpoints
function BreakpointGetAll() end

---Remove a breakpoint
---@param id integer breakpoint id
function BreakpointRemove(id) end

---Get console cols
---@return integer
function ConsoleCols() end

---Get console rows
---@return integer
function ConsoleRows() end

---Inform the debugger that it should continue
function Continue() end

---Disassemble code at a given address
---@param address integer
---@param len integer
---@param instruction_count integer
---@return [Instruction] Array of instructions
function Disassemble(address, len, instruction_count) end

---Get all registered commands
---@return [Command] Array of commands
function GetCommands() end

---Evaluate an expression and get the returned integer, otherwise nil
---@param expression string
---@return integer|nil
function Evaluate(expression) end

---Returns true if debug target is 64-bit
---@return boolean
function Is64BitTarget() end

---Get a virtual memory region
---@param address integer
---@return MemoryRegion
function GetVMRegion(address) end

---Get a list of all virtual memory regions
---@return [MemoryRegion]
function GetVMRegions() end

---Retrieve thread context from the debugger
---@return Context32
function GetContext32() end

---Retrieve thread context from the debugger
---@return Context64
function GetContext64() end

---Read from debuggee memory
---@param address integer
---@param len integer
---@return string
function ReadMemory(address, len) end

---Step into
function StepInto() end

---Step over
function StepOver() end

---Get a symbol name given an address
---@param address integer
---@return Symbol
function SymbolNameToSymbol(address) end

---Write to debuggee memory
---@param address integer
---@param len integer
---@param data string
function WriteMemory(address, len, data) end