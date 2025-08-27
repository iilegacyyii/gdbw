--- Hacky argparse implementation
argparse = {
    iscommand=false;
}

---@class ArgumentParser

ArgumentParser = {}

---Initialize a new ArgumentParser
---@param command_name string
---@param description string
---@param exit_on_help boolean
function ArgumentParser:init(command_name, description, exit_on_help)
    self.FlagArguments = {}
    local help_flag = {"-h", "--help"}
    self.FlagArguments[help_flag] = {
        action="store_true",
        help="show this help message",
        required=false,
        value=false
    }

    self.PositionalArguments = {}
    self.command_name = command_name
    self.description = description
    self.exit_on_help = exit_on_help
end

---Add an argument to the parser
---@param name_or_flags string|table
---@param help string
---@param required boolean
---@param action string
---@param typecallback function|nil
function ArgumentParser:AddArgument(name_or_flags, help, required, action, typecallback)
    local is_name = false
    local is_flag = false

    --Ensure name_or_flags is valid format
    if type(name_or_flags) == "string" then
        if self:IsValidArgName(name_or_flags) then
            is_name = true
        elseif self:IsValidFlagName(name_or_flags) then
            is_flag = true
        end
    elseif type(name_or_flags) == "table" then
        for i, val in pairs(name_or_flags) do
            if not self:IsValidFlagName(val) then
                print("Error: ArgumentParser.AddArgument passed invalid flags (" .. name_or_flags .. "). Flags must be in the format '-f' or '--flag'")
                return
            end
        end
        is_flag = true
    end

    --Valid format, create new argument
    if is_flag then
        local new_arg = {action=action, help=help, required=required, value=nil, typecallback=typecallback}
        if action == "store_true" then new_arg.value = false end
        self.FlagArguments[name_or_flags] = new_arg
    elseif is_name then
        local new_arg = {name=name_or_flags, help=help, value=nil, typecallback=typecallback}
        table.insert(self.PositionalArguments, new_arg)
    else
        print("Error: ArgumentParser.AddArgument passed invalid arg or flag name(s). Flags should follow the format '-f' or '--flag' and positionals can only contain A-z")
        return
    end
end

---Get flag name
---@param flag string
---@return table|string|nil
function ArgumentParser:GetFlagName(flag)
    for flag_name, vals in pairs(self.FlagArguments) do
        if type(flag_name) == "string" and flag_name == flag then
            return flag_name
        elseif type(flag_name) == "table" and table.indexOf(flag_name, flag) ~= nil then
            return flag_name
        end
    end
    return nil
end

---Get long flag name
---@param flag string|table
---@return string|nil
function ArgumentParser:GetLongFlagName(flag)
    if type(flag) == "string" then return flag end

    local idx
    if string.len(flag[2]) > string.len(flag[1]) then idx = 2 else idx = 1 end
    if type(flag) == "table" then return flag[idx] end

    return nil
end

---Returns true if given parameter is a valid argument name e.g. 'file'
---@param arg_name table|string
---@return boolean
function ArgumentParser:IsValidArgName(arg_name)
    if type(arg_name) ~= "string" then return false end
    local match = string.match(arg_name, "%a+")
    if match == nil then return false end
    if string.len(match) == string.len(arg_name) then return true end
    return false
end

---Returns true if given parameter is a valid argument name e.g. '--help'
---@param flag_name table|string
---@return boolean
function ArgumentParser:IsValidFlagName(flag_name)
    if type(flag_name) ~= "string" then return false end
    local match = string.match(flag_name, "-+%a+")
    if match == nil then return false end
    if string.len(match) == string.len(flag_name) then return true end
    return false
end

---Parse arguments given a commandline string e.g. "--flag 1 -h"
---@param commandline_str string
function ArgumentParser:ParseArgs(commandline_str)
    local namespace = {}

    -- Split by space for initial tokenising
    local commandline = {}
    for token in string.gmatch(commandline_str, "%S+") do table.insert(commandline, token) end

    local tokenised = {}
    local in_string = false
    local string_start = 0
    for i = 1, table.len(commandline), 1 do
        if (not in_string) and string.len(commandline[i]) ~= 0 then
            if commandline[i][1] ~= string.char(34) then
                table.insert(tokenised, commandline[i])
            else
                -- If token has no spaces and is string (e.g. "" or "help")
                if commandline[i][-1] == string.char(34) then
                    if string.len(commandline[i]) == 2 then
                        table.insert(tokenised, "")
                    else
                        table.insert(tokenised, string.sub(commandline[i], 2, -1))
                    end
                else
                    in_string = true
                    string_start = i
                end
            end
        elseif in_string and string.len(commandline[i]) ~= 0 then
            if commandline[i][-1] == string.char(34) then
                -- Remove quotes from string when tokenising
                table.insert(tokenised, string.sub(table.concat(commandline, " ", string_start, i), 2, -2))
                in_string = false
            end
        end
    end

    -- If still in string by end of command, quotes are mismatched
    if in_string then
        print("Mismatched quotes in command")
        return nil
    end

    -- Set all flags to false or nil (store_true vs store) in the namespace
    for arg, val in pairs(self.FlagArguments) do
        local arg_name = self:GetLongFlagName(arg)
        if arg_name == nil then
            print("Error: Argparse.ParseArgs, idk how we errored here")
            return nil
        end
        if self.FlagArguments[arg].action == "store_true" then
            namespace[arg_name] = false
        elseif self.FlagArguments[arg].action == "store" then
            namespace[arg_name] = nil
        end
    end

    -- If commandline params empty, return the namespace
    if table.len(tokenised) == 0 then
        return namespace
    end

    -- Assign argument values.
    -- 1. If token is not found prior, assumed positional and value will be popped
    -- 2. If flag found, next token is value and both will be popped
    -- 3. If flag action is store_true, will just set the value to true
    -- 4. Once parsed, if any values left over, invalid usage
    -- 5. Once parsed, if not all positional values have a value, invalid usage
    local positionals_found = 0
    local last_was_flag = false
    local last_flag = ""
    local flag_name
    for i = 1, table.len(tokenised), 1 do
        -- handle 2.
        if last_was_flag then
            flag_name = self:GetLongFlagName(last_flag)
            if flag_name == nil then
                print("Invalid flag specified: " .. last_flag)
                return nil
            end
            namespace[flag_name] = tokenised[i]
            last_was_flag = false
            goto continue
        end
        -- handle 3 & 2
        if self:IsValidFlagName(tokenised[i]) then
            local retrieved_flag_name = self:GetFlagName(tokenised[i])
            if retrieved_flag_name ~= nil then
                -- handle store_true
                if self.FlagArguments[retrieved_flag_name].action == "store_true" then
                    local long_flag_name = self:GetLongFlagName(retrieved_flag_name)
                    namespace[long_flag_name] = true
                    goto continue
                end
                -- flag is not store_true, next token is value
                last_was_flag = true
                last_flag = retrieved_flag_name
            else
                -- invalid flag specified
                print("Invalid flag specified: " .. tokenised[i])
                return nil
            end
            goto continue
        end

        -- If we reach this point, last value was not a flag, nor was the current value a flag
        -- Therefore, handle
        if positionals_found == table.len(self.PositionalArguments) then
            print("Too many positional arguments specified: Unknown: " .. tokenised[i])
            return nil
        end
        -- We need +1 here for lua indexing, but also dont want to break our count for the rest
        -- of processing
        namespace[self.PositionalArguments[positionals_found+1].name] = tokenised[i]
        positionals_found = positionals_found + 1
        ::continue::
    end

    -- Show help before processing any positionals
    if namespace["--help"] then
        return self.ShowHelp(self)
    end

    if positionals_found < table.len(self.PositionalArguments) then
        local missing_args = {}
        for arg, val in pairs(self.PositionalArguments) do
            table.insert(missing_args, val.name)
        end
        print("Missing positional arguments: " .. table.concat(missing_args, ", "))
        return nil
    end

    -- run callback function if necessary
    for flag, value in pairs(namespace) do
        local found = false
        local data
        -- search flag arguments
        for arg_name, arg_data in pairs(self.FlagArguments) do
            if flag == self:GetLongFlagName(arg_name) then
                found = true
                data = arg_data
            elseif flag == arg_name then
                found = true
                data = arg_data
            end
            if found then goto foundflag end
        end
        -- search positional arguments
        for i, arg_data in pairs(self.PositionalArguments) do
            if flag == arg_data.name then
                found = true
                data = arg_data
            end
            if found then goto foundflag end
        end

        ::foundflag::
        if data.typecallback ~= nil then
            namespace[flag] = data.typecallback(value)
        end
    end

    return namespace
end

function ArgumentParser:ShowHelp()
    local text = colour.BOLD .. self.command_name .. colour.DEFAULT
    text = text .. "\n" .. "usage: " .. self.command_name
    for i, positional in pairs(self.PositionalArguments) do
        text = text .. " <" .. positional.name .. ">"
    end
    for flag, vals in pairs(self.FlagArguments) do
        text = text .. " [" .. self.GetLongFlagName(self, flag) .. "]"
    end

    text = text .. "\n"
    text = text .. "\n" .. self.description
    text = text .. "\n"
    text = text .. "\npositional arguments:"
    for i, positional in pairs(self.PositionalArguments) do
        text = text .. "\n" .. string.rpad(positional.name, 20, " ") .. " " .. positional.help
    end
    text = text .. "\n\noptions:"
    for flag, vals in pairs(self.FlagArguments) do
        local flags
        if type(flag) == "table" then
            flags = table.concat(flag, ", ")
        elseif type(flag) == "string" then
            flags = flag
        end
        text = text .. "\n" .. string.rpad(flags, 20, " ") .. " " .. self.FlagArguments[flag].help
    end

    if self.exit_on_help then
        print(text)
        return nil
    end

    print(text)
end