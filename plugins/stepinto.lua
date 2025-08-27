stepinto = {
    iscommand=true;
    alias={"stepinto","si"};
    help="usage: stepinto";
}

function stepinto:parseargs(args)
    local parser = ArgumentParser;
    parser:init("stepinto", "single step into the next instruction", false)
    return parser:ParseArgs(args)
end

function stepinto:command(args)
    local namespace = stepinto:parseargs(args)
    if namespace == nil then return end
    info.displayed = false
    StepInto()
end