search = {
    alias={"search", "locate"};
    iscommand=true;
    help="usage: search <search_base> <pattern_string>";
}

function search:parseargs(args)
    local parser = ArgumentParser;
    parser:init("search", "search for a pattern of bytes", false)
    parser:AddArgument("searchbase", "base address of search", false, "store", Evaluate)
    parser:AddArgument("pattern", "pattern to search for (e.g. AABB)", true, "store", nil)
    return parser:ParseArgs(args)
end

function search:command(args)
    local namespace = search:parseargs(args)
    if namespace == nil then return end

    local result = SearchVM(namespace["searchbase"], namespace["pattern"])
    if (result == 0) then
        printf("Unable to find '%s' within 4mb of search base", namespace["pattern"])
        return
    end
    
    printf("Found '%s' @ %s", namespace["pattern"], address2hex(result))
    examine:command(address2hex(result) .. " -f qword -c 4")
end