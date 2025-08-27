#include "LuaManager.hpp"

gdbw::LuaManager::LuaManager()
{
    m_luastate = luaL_newstate();
    luaL_openlibs(m_luastate);
    LoadPlugins();
}

gdbw::LuaManager::~LuaManager()
{
    lua_close(m_luastate);
}

void gdbw::LuaManager::EnterInterpreter()
{
    int error;
    char buff[256] = { 0 };

    while (fgets(buff, sizeof(buff)-1, stdin) != NULL) {
        error = luaL_loadbuffer(m_luastate, buff, strlen(buff), "line") ||
            lua_pcall(m_luastate, 0, 0, 0);
        if (error) {
            fprintf(stderr, "%s", lua_tostring(m_luastate, -1));
            lua_pop(m_luastate, 1);  /* pop error message from the stack */
        }
    }
}

bool gdbw::LuaManager::Prompt()
{
    std::string commandline;
    std::string command;
    std::string args;

    RunCommand("prompt", "");
    std::getline(std::cin, commandline);
    if (std::cin.fail() || std::cin.eof()) std::cin.clear(); // reset cin state

    if (commandline == "")
        commandline = m_lastcommandline;
    else
        m_lastcommandline = commandline;

    if (commandline.contains(" "))
    {
        command = commandline.substr(0, commandline.find(" "));
        args = commandline.substr(commandline.find(" ") + 1);
    }
    else
    {
        command = commandline;
        args = "";
    }

    auto plugin = m_plugins.find(command);
    if (plugin != m_plugins.end())
        RunCommand(m_plugins[command]["name"], args);
    else if (command == "quit" || command == "q")
        ExitProcess(0); // TODO: change to something like g_dbg->Stop();
    else
        printf("Unknown command\n");

    return false;
}

void gdbw::LuaManager::RegisterGlobalFunction(LUA_FUNCTION func, const char* name)
{
    lua_pushcfunction(m_luastate, func);
    lua_setglobal(m_luastate, name);
}

std::expected<bool, std::string> gdbw::LuaManager::LoadPlugins()
{
    // Figure out plugin directory
    wchar_t path[FILENAME_MAX] = { 0 };
    GetModuleFileNameW(nullptr, path, FILENAME_MAX);
    auto plugin_dir = std::filesystem::path(path).parent_path().append("plugins");

    if (!std::filesystem::is_directory(plugin_dir))
        return std::unexpected("failed to locate plugin directory");

    // process every file in plugins directory
    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir))
    {
        // skip non-.lua files
        if (!entry.path().has_extension() || entry.path().extension().compare(".lua"))
            continue;
        LoadPlugin(entry.path().string());
    }

    return true;
}

inline void gdbw::LuaManager::LoadPlugin(std::filesystem::path filepath)
{
    auto plugin_name = filepath.filename().replace_extension().string();

    if (luaL_loadfile(m_luastate, filepath.string().c_str()) || lua_pcall(m_luastate, 0, 0, 0))
    {
        printf("Error loading plugin (%s): %s\n", plugin_name.c_str(), lua_tostring(m_luastate, -1));
        return;
    }

    std::string error;
    auto iscommand = GetField<bool>(plugin_name.c_str(), "iscommand");
    if (!iscommand)
    {
        error = iscommand.error();
        goto ERROR_LOADING;
    }

    if (*iscommand == true)
    {
        auto help = GetField<std::string>(plugin_name.c_str(), "help");
        if (!help)
        {
            error = help.error();
            goto ERROR_LOADING;
        }

        // alias processing
        auto alias_result = GetField<std::vector<std::string>>(plugin_name.c_str(), "alias");
        if (!alias_result)
        {
            error = alias_result.error();
            goto ERROR_LOADING;
        }
        for (auto alias : *alias_result)
        {
            m_plugins.insert({
                alias, {
                    {"name", plugin_name},
                    {"help", (*help)}
                }
            });
        }

        return;
    }
    else return; // not a command.

ERROR_LOADING:
    printf("Error loading plugin %s: %s\n", plugin_name.c_str(), error.c_str());
}

inline void gdbw::LuaManager::RunCommand(std::string command, std::string args)
{
    lua_getglobal(m_luastate, command.c_str());
    if (!lua_istable(m_luastate, -1))
    {
        printf("Error running plugin (%s): %s\n", command.c_str(), lua_tostring(m_luastate, -1));
        return;
    }

    // push field value to stack
    lua_pushstring(m_luastate, "command");
    lua_gettable(m_luastate, -2); // get command_name["command"]

    if (lua_isnil(m_luastate, -1))
    {
        lua_pop(m_luastate, -1); // remove result from stack
        lua_pop(m_luastate, -1); // remove table from stack
        printf("Error running command (%s): field 'command' does not exist\n", command.c_str());
        return;
    }

    if (!lua_isfunction(m_luastate, -1))
    {
        lua_pop(m_luastate, -1); // remove result from stack
        lua_pop(m_luastate, -1); // remove table from stack
        printf("Error running command (%s): field 'command' is not a function\n", command.c_str());
        return;
    }

    lua_getglobal(m_luastate, command.c_str());
    lua_pushstring(m_luastate, args.c_str());
    if (lua_pcall(m_luastate, 2, 0, 0))
    {
        printf("Error running command (%s): %s\n", command.c_str(), lua_tostring(m_luastate, -1));
        lua_pop(m_luastate, -1);
        lua_pop(m_luastate, -1);
        lua_pop(m_luastate, -1);
        return;
    }

    lua_pop(m_luastate, -1);
    lua_pop(m_luastate, -1);
    return;
}

std::expected<bool, std::string> gdbw::LuaManager::FieldIsFunction(const char* table_name, const char* key)
{
    lua_getglobal(m_luastate, table_name);
    if (!lua_istable(m_luastate, -1)) return std::unexpected("failed to locate plugin table");

    // push field value to stack
    lua_pushstring(m_luastate, key);
    lua_gettable(m_luastate, -2); // get table[key]

    bool result = false;
    printf("lua type: %s\n", lua_typename(m_luastate, lua_type(m_luastate, -1)));
    if (lua_isfunction(m_luastate, -1))
        result = true;

    lua_pop(m_luastate, -1); // remove result from stack
    lua_pop(m_luastate, -1); // remove table from stack
    return result;
}
