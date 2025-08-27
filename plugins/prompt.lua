prompt = {
    alias={"prompt"};
    iscommand=true;
    help="prompt, pls dont run";
}

function prompt:command(args)
    info:command("prompt")
    io.write(colour.BOLD .. "gdbw> " .. colour.DEFAULT)
end