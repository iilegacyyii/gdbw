memory = {
    iscommand=false;
}

PAGE_NOACCESS = 0x01
PAGE_READONLY = 0x02
PAGE_READWRITE = 0x04
PAGE_WRITECOPY = 0x08
PAGE_EXECUTE = 0x10
PAGE_EXECUTE_READ = 0x20
PAGE_EXECUTE_READWRITE = 0x40
PAGE_EXECUTE_WRITECOPY = 0x80
PAGE_GUARD = 0x100

---@param prot integer
---@return string
function memory:prot2str(prot)
    local out = "";
    if prot & 0xFF == PAGE_NOACCESS then
        out = "----"
    elseif prot & 0xFF == PAGE_READONLY then
        out = "-R--"
    elseif prot & 0xFF == PAGE_READWRITE then
        out = "-RW-"
    elseif prot & 0xFF == PAGE_WRITECOPY then
        out = "-RWC"
    elseif prot & 0xFF == PAGE_EXECUTE then
        out = "E---"
    elseif prot & 0xFF == PAGE_EXECUTE_READ then
        out = "ER--"
    elseif prot & 0xFF == PAGE_EXECUTE_READWRITE then
        out = "ERW-"
    elseif prot & 0xFF == PAGE_EXECUTE_WRITECOPY then
        out = "ERWC"
    else
        out = "????"
    end

    if prot & PAGE_GUARD == PAGE_GUARD then
        out = out .. "G"
    else
        out = out .. " "
    end
    return out
end

---Read from target memory. Returns nil on failure
---@param address integer
---@param size integer
---@return string|nil
function memory:read(address, size)
    local success
    local data
    success, data = pcall(function(a, s) return ReadMemory(a, s) end, address, size)
    if success == false then return nil end
    return data
end

---Read ptr_size integer from target memory. Returns nil on failure
---@param address integer
---@return integer|nil
function memory:readptr(address)
    local ptr_size = 4
    if Is64BitTarget() then ptr_size = 8 end
    local data = memory:read(address, ptr_size)
    if data == nil then return nil end

    if ptr_size == 4 then
        return string.unpack("<i4", data)
    end
    return string.unpack("<i8", data)
end

---Read qword from target memory. Returns nil on failure
---@param address integer
---@return integer|nil
function memory:readqword(address)
    local data = memory:read(address, 8)
    if data == nil then return nil end
    return string.unpack("<i8", data)
end