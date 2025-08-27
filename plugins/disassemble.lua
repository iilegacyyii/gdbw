disassemble = {
    iscommand=true;
    alias={"disassemble","disas"};
    help="usage: disassemble [address]";
}

function disassemble:parseargs(args)
    local parser = ArgumentParser
    parser:init("disassemble", "disassemble a given function", false)
    parser:AddArgument("address", "address or name of function to disassemble", false, "store", Evaluate)
    return parser:ParseArgs(args)
end

function disassemble:print_instructions(instructions, ip)
    for i, insn in pairs(instructions) do
        ---@type Symbol
        local symbol
        local success
        local disasm_line
        success, symbol = pcall(function(a) return AddressToSymbol(a) end, insn.address)
        if success then
            disasm_line = string.format("%s <%s+%d> \t%s\t\t%s", address2hex(insn.address), symbol.name, symbol.displacement, insn.mnemonic, insn.opstr)
        else
            disasm_line = string.format("%s \t%s\t\t%s", address2hex(insn.address), insn.mnemonic, insn.opstr)
        end
        if insn.address == ip then
            disasm_line = colour.GREEN .. disasm_line .. colour.DEFAULT
        end
        print(disasm_line)
    end
end

function disassemble:command(args)
    local namespace = disassemble:parseargs(args)
    if namespace == nil then return end

    local instructions
    local size = 15*16 -- max asm instruction size == 15 bytes
    local count = 16

    local ip
    if Is64BitTarget() then
        ip = GetContext64().rip
    else
        ip = GetContext32().eip
    end

    local address = namespace["address"]
    if address == nil then
        print("Invalid address or symbol name provided, assuming instruction pointer")
        instructions = Disassemble(ip, size, count)
        goto disas
    elseif type(address) == "table" then
        ---@type Symbol
        local symbol = address
        instructions = Disassemble(symbol.address, symbol.size, 0)
    else
        instructions = Disassemble(address, size, count)
    end

    ::disas::
    disassemble:print_instructions(instructions, ip)
end