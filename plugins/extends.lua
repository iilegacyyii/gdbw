extends = {
    iscommand=false;
}

---
--- GLOBALS
---

function address2hex(address)
    if Is64BitTarget() then
        return string.format("0x%016x", address)
    else
        return string.format("0x%08x", address)
    end
end

function printf(format, ...)
    print(string.format(format, ...))
end

---
--- string extensions
---

function string.lpad(s, l, c)
	local res = string.rep(c or ' ', l - #s) .. s

	return res, res ~= s
end

function string.rpad (s, l, c)
	local res = s .. string.rep(c or ' ', l - #s)
	return res, res ~= s
end

function string.pad(s, l, c)
	c = c or ' '
	local res1, stat1 = string.rpad(s, math.floor((l / 2) + string.len(s)/2), c)
	local res2, stat2 = string.lpad(res1,  l, c)
	return res2, stat1 or stat2
end

---
--- table extensions
---

--- Allows for str[idx]
getmetatable('').__index = function(str,i) return string.sub(str,i,i) end

---Gets the index of a value in a list
---If value is not found, returns nil
---@param table table
---@param v any
---@return number|nil
function table.indexOf(table, v)
    for _, value in pairs(table) do
        if value == v then
            return _
        end
    end
    return nil
end

---Returns true if a table contains the given key
---@param table table
---@param k any
---@return boolean
function table.hasKey(table, k)
    for key, _ in pairs(table) do
        if k == key then
            return true
        end
    end
    return false
end

function table.len(table)
    local count = 0
    for _ in pairs(table) do count = count + 1 end
    return count
end