breakpointenable = {
    iscommand=true;
    alias={"breakpointenable","enable"};
    help="usage: breakpointenable <id>";
}

function breakpointenable:parseargs(args)
    local parser = ArgumentParser
    parser:init("breakpointenable", "enable a breakpoint given an id", false)
    parser:AddArgument("id", "breakpoint id", true, "store", math.tointeger)
    return parser:ParseArgs(args)
end

function breakpointenable:command(args)
    local namespace = breakpointenable:parseargs(args)
    if namespace == nil then return end

    local id = namespace["id"]
    if id == nil then return end

    local success
    local _
    success, _ = pcall(function(i) return BreakpointSetFlags(i, 6) end, id)
    if success == false then
        printf("Breakpoint with id %d does not exist", id)
        return
    end
    
    printf("Breakpoint %d enabled", id)
end