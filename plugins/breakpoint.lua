breakpoint = {
    iscommand=true;
    alias={"breakpoint","bp"};
    help="usage: breakpoint <address>";
}

function breakpoint:parseargs(args)
    local parser = ArgumentParser
    parser:init("breakpoint", "add breakpoint at a given address", false)
    parser:AddArgument("address", "breakpoint address", true, "store", Evaluate)
    return parser:ParseArgs(args)
end

function breakpoint:command(args)
    local namespace = breakpoint:parseargs(args)
    if namespace == nil then return end

    local address = namespace["address"]
    local bpid = BreakpointAdd(address)
    printf("Breakpoint %d set @ 0x%x", bpid, address)
end