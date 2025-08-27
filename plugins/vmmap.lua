vmmap = {
    iscommand = true;
    alias={"vmmap"};
    help="usage: vmmap [address]";
}

function vmmap:parseargs(args)
    local parser = ArgumentParser
    parser:init("vmmap", "search and display virtual memory pages", false)
    parser:AddArgument("address", "address to filter for", false, "store", Evaluate)
    return parser:ParseArgs(args)
end

function vmmap:prot2colour(prot)
    local out = "";
    if prot & 0xFF == PAGE_NOACCESS then
        out = colour.DARKGRAY
    elseif prot & 0xFF == PAGE_READONLY then
        out = colour.DEFAULT
    elseif prot & 0xFF == PAGE_READWRITE then
        out = colour.MAGENTA
    elseif prot & 0xFF == PAGE_WRITECOPY then
        out = colour.MAGENTA
    elseif prot & 0xFF == PAGE_EXECUTE then
        out = colour.RED
    elseif prot & 0xFF == PAGE_EXECUTE_READ then
        out = colour.RED
    elseif prot & 0xFF == PAGE_EXECUTE_READWRITE then
        out = colour.UNDERLINE .. colour.RED
    elseif prot & 0xFF == PAGE_EXECUTE_WRITECOPY then
        out = colour.UNDERLINE .. colour.RED
    else
        out = colour.DEFAULT
    end
    return out
end

function vmmap:command(args)
    local namespace = vmmap:parseargs(args)
    if namespace == nil then return end
    
    local regions = GetVMRegions()
    -- `vmmap [address]`
    local address = namespace["address"]
    if address ~= nil then
        local found = false
        for i, region in pairs(regions) do
            if (address >= region.baseaddress) and (address < region.baseaddress + region.size) then
                found = true
                if i == 0 then
                    regions = {regions[i], regions[i+1]}
                    break
                else
                    regions = {regions[i-1], regions[i], regions[i+1]}
                    break
                end
            end
        end

        if found == false then
            print("Could not find region with specified address")
            return
        end
    end

    -- legend
    printf("LEGEND: " .. colour.YELLOW .. "STACK" .. colour.DEFAULT .. " | " .. colour.RED .. "CODE" .. colour.DEFAULT .. " | " .. colour.MAGENTA .. "DATA" .. colour.DEFAULT .. " | " .. colour.RED .. colour.UNDERLINE .. "RWX" .. colour.DEFAULT .. " | " .. "RODATA")

    printf("%s\t%s %s %s %s", string.lpad("Start", 18, " "), string.lpad("End", 18, " "), string.lpad("Prot", 6, " "), string.lpad("Size", 8, " "), "Name")
    -- To grab the stack we need to know what page range stack pointer is in
    local ctx;
    local sp;
    if info.targetis64bit then
        ctx = GetContext64()
        sp = ctx.rsp
    else
        ctx = GetContext32()
        sp = ctx.esp
    end
    for i, region in pairs(regions) do
        -- Do the color first. The color depends on the protections of the page
        local _colour;
        if ((sp > region.baseaddress) and (sp < (region.baseaddress + region.size))) then
            _colour = colour.YELLOW
        else
            -- TODO: if there's an RWX page, this underlines the entire line of output
            -- fix
            _colour = vmmap:prot2colour(region.protections)
        end
        local base_str = string.lpad(string.format("0x%x", region.baseaddress), 18, " ")
        local end_str = string.lpad(string.format("0x%x", region.baseaddress + region.size), 18, " ")
        local prot_str = string.lpad(memory:prot2str(region.protections), 6, " ")
        local size_str = string.lpad(string.format("%x", region.size), 8, " ")

        local success, name = pcall(function(a) return AddressToModuleName(a) end, region.baseaddress)
        if success == false then
            name = ""
        end

        printf("%s%s\t%s %s %s %s%s", _colour, base_str, end_str, prot_str, size_str, name, colour.DEFAULT)
    end
end