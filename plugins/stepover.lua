stepover = {
    iscommand=true;
    alias={"stepover","so","ni"};
    help="usage: stepover";
}

function stepover:parseargs(args)
    local parser = ArgumentParser;
    parser:init("stepover", "step over the next instruction", false)
    return parser:ParseArgs(args)
end

function stepover:command(args)
    local namespace = stepover:parseargs(args)
    if namespace == nil then return end
    info.displayed = false
    StepOver()
end