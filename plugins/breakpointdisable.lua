breakpointdisable = {
    iscommand=true;
    alias={"breakpointdisable","disable"};
    help="usage: breakpointdisable <id>";
}

function breakpointdisable:parseargs(args)
    local parser = ArgumentParser
    parser:init("breakpointdisable", "disable a breakpoint given an id", false)
    parser:AddArgument("id", "breakpoint id", true, "store", math.tointeger)
    return parser:ParseArgs(args)
end

function breakpointdisable:command(args)
    local namespace = breakpointdisable:parseargs(args)
    if namespace == nil then return end

    local id = namespace["id"]
    if id == nil then return end

    local success
    local _
    success, _ = pcall(function(i) return BreakpointSetFlags(i, 0) end, id)
    if success == false then
        printf("Breakpoint with id %d does not exist", id)
        return
    end
    
    printf("Breakpoint %d disabled", id)
end