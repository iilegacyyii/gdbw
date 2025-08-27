examine = {
    iscommand=true;
    alias={"examine", "x"};
    help="usage: examine [address] [-c count]";
    valid_formats={};
}

function examine:parseargs(args)
    local parser = ArgumentParser;
    parser:init("examine", "display memory contents at a given address", false)
    parser:AddArgument("address", "address of memory content", true, "store", Evaluate)
    -- TODO: implement format specifiers
    --parser:AddArgument({"-f", "--format"}, "format to display data (e.g. gx)", false, "store", nil)
    parser:AddArgument({"-c", "--count"}, "number of items to display (e.g. 10)", false, "store", math.tointeger)
    return parser:ParseArgs(args)
end

function examine:command(args)
    local namespace = examine:parseargs(args)
    if namespace == nil then return end

    ---@type integer
    local address = namespace["address"]

    ---@type integer
    local count = 1
    if count ~= nil then count = namespace["--count"] end

    local ptr_size = 4
    if Is64BitTarget() then ptr_size = 8 end
    local to_print = ""
    for i = 0, count do
        local data = memory:readptr(address + (i*ptr_size))
        if data == nil then
            printf("Failed to read data at address %s", address2hex(address + (i*ptr_size)))
            goto output
        end
        to_print = to_print .. string.format("%02x:%04x| " .. colour.LIGHTBLUE .. "%s" .. colour.DEFAULT .. " : %s", i, i*ptr_size, address2hex(address + (i*ptr_size)), address2hex(data)) .. string.char(10)
    end
    ::output::
    print(to_print)
end