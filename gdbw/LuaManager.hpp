#pragma once
#include <algorithm>
#include <cstring>
#include <expected>
#include <filesystem>
#include <iostream>
#include <map>
#include <windows.h>
#include "thirdparty/lua/include/lua.hpp"

typedef int(__stdcall* LUA_FUNCTION)(lua_State* L);

namespace gdbw
{
	class LuaManager
	{
	public:
		LuaManager();
		~LuaManager();
		// Drop in to a lua interpreter using the current lua state. Primarily used for debug purposes.
		void EnterInterpreter();
		// Get a reference to the commands list
		inline const std::map<std::string, std::map<std::string, std::string>>& GetCommands() { return m_plugins; }
		// Prompt user for command(s), returns true when the user has finished providing input.
		bool Prompt();
		// Register a C[++] function as a globally available function in the lua state 
		void RegisterGlobalFunction(LUA_FUNCTION func, const char* name);
	private:
		// Load all plugins for the debugger
		std::expected<bool, std::string> LoadPlugins();
		inline void LoadPlugin(std::filesystem::path filepath);
		inline void RunCommand(std::string command, std::string args);
		std::expected<bool, std::string> FieldIsFunction(const char* table_name, const char* key);
		std::map<std::string, std::map<std::string, std::string>> m_plugins;
		lua_State* m_luastate;
		std::string m_lastcommandline;

		// Get a field from a lua table
		template<typename T>
		std::expected<T, std::string> GetField(const char* table_name, const char* key)
		{
			lua_getglobal(m_luastate, table_name);
			if (!lua_istable(m_luastate, -1)) return std::unexpected("failed to locate plugin table");

			// push field value to stack
			lua_pushstring(m_luastate, key);
			lua_gettable(m_luastate, -2); // get table[key]

			T result;
			if (lua_isnil(m_luastate, -1))
			{
				lua_pop(m_luastate, -1); // remove result from stack
				lua_pop(m_luastate, -1); // remove table from stack
				return std::unexpected(std::format("field {} does not exist", key));
			}

			if constexpr (std::is_same_v<T, size_t> || std::is_same_v<T, int64_t>)
			{
				if (lua_isinteger(m_luastate, -1))
					result = lua_tointeger(m_luastate, -1);
				else goto INVALID_TYPE;
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				if (lua_isboolean(m_luastate, -1))
					result = (bool)lua_toboolean(m_luastate, -1);
				else goto INVALID_TYPE;
			}
			else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, unsigned char*> || std::is_same_v<T, std::string>)
			{
				if (lua_isstring(m_luastate, -1))
					result = lua_tostring(m_luastate, -1);
				else goto INVALID_TYPE;
			}
			else if constexpr (std::is_same_v<T, std::vector<std::string>>)
			{
				if (!lua_istable(m_luastate, -1)) 
					goto INVALID_TYPE;
				size_t indexes = lua_rawlen(m_luastate, -1);
				result.reserve(indexes);
				for (size_t i = 0; i < indexes; i++)
				{
					int luaidx = i + 1;
					lua_pushinteger(m_luastate, luaidx);
					lua_gettable(m_luastate, -2);
					if (lua_type(m_luastate, -1) == LUA_TNIL) break;
					result.push_back(luaL_checkstring(m_luastate, -1));
					lua_pop(m_luastate, 1);
				}
			}
			else
			{
				lua_pop(m_luastate, -1); // remove result from stack
				lua_pop(m_luastate, -1); // remove table from stack
				return std::unexpected(std::format("GetField does not support type {}", typeid(T).name()));
			}

			//lua_pop(m_luastate, -1); // remove result from stack
			//lua_pop(m_luastate, -1); // remove table from stack
			return result;

		INVALID_TYPE:
			//lua_pop(m_luastate, -1); // remove result from stack
			//lua_pop(m_luastate, -1); // remove table from stack
			return std::unexpected(std::format("unexpected field type for key {}", key));

		}
	};
}

