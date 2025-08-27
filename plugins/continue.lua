continue = {
    iscommand = true;
    alias={"continue", "c"};
    help="usage: continue"
}

function continue:parseargs(args)
    local parser = ArgumentParser;
    parser:init("continue", "continue execution", false)
    return parser:ParseArgs(args)
end

function continue:command(args)
    local namespace = continue:parseargs(args)
    if namespace == nil then return end
    info.displayed = false
    Continue()
end