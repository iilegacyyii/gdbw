delbreakpoint = {
    iscommand=true;
    alias={"delbreakpoint","delbp","del"};
    help="usage: delbreakpoint <id>";
}

function delbreakpoint:parseargs(args)
    local parser = ArgumentParser
    parser:init("delbreakpoint", "remove a breakpoint given an id", false)
    parser:AddArgument("id", "breakpoint id", true, "store", math.tointeger)
    return parser:ParseArgs(args)
end

function delbreakpoint:command(args)
    local namespace = delbreakpoint:parseargs(args)
    if namespace == nil then return end

    local id = namespace["id"]
    if id == nil then return end

    local success
    local _
    success, _ = pcall(function(i) return BreakpointRemove(i) end, id)
    if success == false then
        printf("Breakpoint with id %d does not exist", id)
        return
    end
    
    printf("Breakpoint %d removed", id)
end