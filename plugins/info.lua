info = {
    iscommand = true;
    alias = {"info", "i"};
    help = "usage: info [breakpoint|registername]";
    targetis64bit = true;
    displayed = false;
    virtual_map = nil;
    print_cache = nil;
    old_ctx = {
        rax = 0,
        rbx = 0;
        rcx = 0;
        rdx = 0;
        rdi = 0;
        rsi = 0;
        r8  = 0;
        r9  = 0;
        r10 = 0;
        r11 = 0;
        r12 = 0;
        r13 = 0;
        r14 = 0;
        r15 = 0;
        rbp = 0;
        rsp = 0;
        rip = 0;
    };
}

function info:get_gen_purp_registers()
    if info.targetis64bit then
        return {"rax", "rbx", "rcx", "rdx", "rdi", "rsi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp", "rsp", "rip"}
    else
        return {"eax", "ebx", "ecx", "edx", "edi", "esi", "ebp", "esp", "eip"}
    end
end

function info:get_stack(depth, ctx)
    local stack_start
    local ptr_size
    local vals = {}
    if info.targetis64bit then
        stack_start = ctx.rsp
        ptr_size = 8
    else
        stack_start = ctx.esp
        ptr_size = 4
    end

    for i = 1, depth, 1 do
        local toread = stack_start + ((i-1)*ptr_size)
        local rawmem = ReadMemory(toread, ptr_size)
        local memnum = 0
        for j = 1, ptr_size, 1 do
            memnum = memnum | (string.byte(rawmem, j) << ((j-1)*ptr_size))
        end
        vals[toread] = memnum
    end
    return vals
end

function info:create_border(name)
    local border_start = "["
    local border_end = "]"

    local border = colour.BLUE .. border_start
    local border_len = string.len(border_start) + string.len(border_end)
    local full_len = border_len + string.len(name)

    local _pad = 3
    if string.len(name) == _pad then
        _pad = 0
    end
    local cols = ConsoleCols()
    if cols < (border_len + _pad) then
        return colour.BLUE .. string.rpad("-", cols, '-') .. colour.DEFAULT
    elseif cols < (border_len + full_len) then
        local remaining_space = cols - border_len
        local name_in_border = string.sub(name, 1, remaining_space - _pad)
        border = border .. name_in_border .. "..." .. border_end .. colour.DEFAULT
    else
        local remaining_space = cols - border_len
        border = border .. string.pad(name, remaining_space, "-") .. border_end .. colour.DEFAULT
    end
    return border
end

function info:get_addr_colour(addr)
    for i, region in pairs(info.virtual_map) do
        if ((addr >= region.baseaddress) and (addr < (region.baseaddress + region.size))) then
            return vmmap:prot2colour(region.protections)
        end
        if (addr < region.baseaddress) then
            return colour.DEFAULT
        end
    end
end

function info:get_register_pprint(name, value, color)
    local to_print = color .. colour.BOLD .. string.upper(string.rpad(name , 4, " ")) .. colour.DEFAULT
    local addr_color = info:get_addr_colour(value)
    to_print = string.format("%s %s0x%x%s", to_print, addr_color, value, colour.DEFAULT);
    return to_print
end

function info:get_gen_purp_register_str(ctx)
    local to_print = info:create_border("REGISTERS") .. string.char(10);
    local state_changed = false
    for i, reg in pairs(info:get_gen_purp_registers()) do
        if ctx[reg] ~= info.old_ctx[reg] then
            state_changed = true
            to_print = to_print .. info:get_register_pprint("*" .. reg, ctx[reg], colour.RED) .. string.char(10)
        else
            to_print = to_print .. info:get_register_pprint(" " .. reg, ctx[reg], colour.WHITE) .. string.char(10)
        end
    end

    if state_changed then info.old_ctx = ctx end
    return to_print
end

function info:get_disasm_str(ctx)
    local to_print = info:create_border("DISASM") .. string.char(10)
    ---@type [Instruction]
    local instructions
    local ip
    if info.targetis64bit then ip = ctx.rip
    else ip = ctx.eip end
    instructions = Disassemble(ip, 15*16, 12)

    local prefix_strs = {}
    local mnemonic_strs = {}
    local operation_strs = {}

    local longest_prefix = 0
    local longest_mnemonic = 0
    local sz = 0

    for i, insn in pairs(instructions) do
        ---@type Symbol
        local symbol
        local success
        success, symbol = pcall(function(a) return AddressToSymbol(a) end, insn.address)
        if success then
            local prefix = string.format("%s <%s+%d> ",  address2hex(insn.address), symbol.name, symbol.displacement)
            if longest_prefix < string.len(prefix) then
                longest_prefix = string.len(prefix)
            end
            prefix_strs[i] = prefix
        else
            local prefix = string.format("%s ", address2hex(insn.address))
            if longest_prefix < string.len(prefix) then
                longest_prefix = string.len(prefix)
            end
            prefix_strs[i] = prefix
        end

        if string.len(insn.mnemonic) > longest_mnemonic then
            longest_mnemonic = string.len(insn.mnemonic)
        end
        mnemonic_strs[i] = insn.mnemonic
        operation_strs[i] = insn.opstr

        sz = sz + 1
    end
    for i = 1, sz, 1 do
        local disasm_line = string.rpad(prefix_strs[i], longest_prefix + 4, " ")
        disasm_line = disasm_line .. string.rpad(mnemonic_strs[i], longest_mnemonic + 4, " ")
        disasm_line = disasm_line .. operation_strs[i]

        if instructions[i].address == ip then
            disasm_line = colour.GREEN .. disasm_line .. colour.DEFAULT
        end
        to_print = to_print .. disasm_line .. string.char(10)
    end
    return to_print
end

function info:get_stack_str(ctx)
    local to_print = info:create_border("STACK") .. string.char(10)
    local stack_vals = info:get_stack(8, ctx)
    local ptr_size
    if info.targetis64bit then ptr_size = 8 else ptr_size = 4 end
    local i = 0
    for k, v in pairs(stack_vals) do
        local register_on_stack = ""
        for k1, v1 in pairs(ctx) do
            if v1 == k then
                register_on_stack = k1
            end
        end
        to_print = to_print .. string.format("%02x:%04x|", i, i*ptr_size)
        to_print = to_print .. string.format("%s%s0x%x%s ", string.pad(register_on_stack, 5, " "), colour.YELLOW,  k, colour.DEFAULT)
        to_print = to_print .. string.format(": %s0x%x%s", info:get_addr_colour(v), v, colour.DEFAULT) .. string.char(10)
        i = i + 1
    end
    return to_print
end

function info:print_breakpoints()
    for i, bp in pairs(BreakpointGetAll()) do
        local enabled_str = colour.RED .. "disabled" .. colour.DEFAULT
        if bp.enabled then
            enabled_str = colour.GREEN .. "enabled" .. colour.DEFAULT
        end
        printf("%d: %s %s", bp.id, address2hex(bp.address), enabled_str)
    end
end

---@param ctx Context64|Context32
function info:print_state(ctx)
    if info.displayed == true then
        print(info.print_cache)
        return
    end
    info.virtual_map = GetVMRegions()
    local to_print = info:get_gen_purp_register_str(ctx)
    to_print = to_print .. info:get_disasm_str(ctx)
    to_print = to_print .. info:get_stack_str(ctx)
    info.print_cache = to_print
    print(to_print)
end

function info:parseargs(args)
    local parser = ArgumentParser
    parser:init("info", "display information about the current context", false)
    parser:AddArgument("type", "type of information to display (e.g. breakpoint)", false, "store", nil)
    return parser:ParseArgs(args)
end

function info:command(args)
    local namespace = info:parseargs(args)
    if namespace == nil then return end

    info.targetis64bit = Is64BitTarget()

    local infotype = namespace["type"]
    if infotype == "breakpoint" or infotype == "bp" then
        info:print_breakpoints()
        return
    else
        local ctx
        if info.targetis64bit then ctx = GetContext64() else ctx = GetContext32() end
        -- user call
        if (infotype == nil) then
            info:print_state(ctx)
        -- prompt call
        elseif (infotype == "prompt") then
            if info.displayed == false then
                info:print_state(ctx)
                info.displayed = true
            end
            return
        else
            if (ctx[args] ~= nil) then
                print(address2hex(ctx[args]))
            else
                printf("Unknown register %s", args)
            end
        end
    end
end