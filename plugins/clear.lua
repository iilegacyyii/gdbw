clear = {
    iscommand=true;
    alias={"clear", "cls"};
    help="usage: clear"
}

function clear:parseargs(args)
    local parser = ArgumentParser;
    parser:init("clear", "clear the terminal", false)
    return parser:ParseArgs(args)
end

function clear:command(args)
    local namespace = clear:parseargs(args)
    if namespace == nil then return end

    if not os.execute("clear") then
        os.execute("cls")
    elseif not os.execute("cls") then
        for i = 1,25 do
            print("\n\n")
        end
    end
end