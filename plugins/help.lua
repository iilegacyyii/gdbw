help={
    iscommand=true;
    alias={"help"};
    help="usage: help [command]";
}

function help:parseargs(args)
    local parser = ArgumentParser;
    parser:init("help", "displays help", false)
    parser:AddArgument("command", "display help for command(s)", false, "store", nil)
    return parser:ParseArgs(args)
end

function help:command(args)
    local namespace = help:parseargs(args)
    if namespace == nil then return end

    ---@type [Command]
    local commands = GetCommands()

    -- show help for all commands
    if namespace["command"] == nil then
        for name, command in pairs(commands) do
            printf("%s -> %s", colour.BOLD .. command.name .. colour.DEFAULT, command.help)
        end
    else -- help for a specific command
        local found = false
        for name, command in pairs(commands) do
            if namespace["command"] == name then
                found = true
                goto searchdone
            end
            for i, alias in pairs(command.alias) do
                if namespace["command"] == alias then
                    found = true
                    namespace["command"] = command.name
                    goto searchdone
                end
            end
        end
        ::searchdone::
        if found then
            _G[namespace["command"]]:command("-h")
        else
            printf("could not find command with name '%s'", namespace["command"])
        end
    end
end