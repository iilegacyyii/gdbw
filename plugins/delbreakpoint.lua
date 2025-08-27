delbreakpoint = {
    iscommand=true;
    alias={"delbreakpoint","delbp","del"};
    help="usage: delbreakpoint <id>";
}

function delbreakpoint:parseargs(args)
    local parser = ArgumentParser
    parser:init("delbreakpoint", "remove a breakpoint given an id", false)
    parser:AddArgument("id", "breakpoint id", true, "store", Evaluate)
    return parser:ParseArgs(args)
end

function delbreakpoint:command(args)
    local namespace = delbreakpoint:parseargs(args)
    if namespace == nil then return end

    local id = namespace["id"]
    BreakpointRemove(id)
    printf("Breakpoint %d removed", id)
end