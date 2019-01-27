#include "game/state/luagamestate.h"
#include "framework/configfile.h"
#include "framework/luaframework.h"
#include "game/state/gamestate.h"
#include "game/state/luagamestate_support.h"
#include "game/state/luagamestate_support_generated.h"

namespace OpenApoc
{

LuaGameState::LuaGameState() : L(luaL_newstate()) {}

LuaGameState::~LuaGameState() { lua_close(L); }

bool LuaGameState::pushHook(const char *hookName)
{
	lua_getglobal(L, "OpenApoc");
	lua_getfield(L, -1, "hook");
	lua_remove(L, -2); // remove openapoc table
	int type = lua_getfield(L, -1, hookName);
	lua_remove(L, -2); // remove hook table
	if (type == LUA_TFUNCTION || type == LUA_TTABLE || type == LUA_TUSERDATA)
	{
		// leave callable object on the stack
		return true;
	}
	else
	{
		// remove non-callable object from stack
		lua_pop(L, 1);
		return false;
	}
}

void LuaGameState::init(GameState &game)
{
	luaL_openlibs(L);

	lua_createtable(L, 0, 3);

	// create a table for hooks
	lua_createtable(L, 0, 0);
	lua_setfield(L, -2, "hook");

	// create enum table
	pushLuaGamestateEnums(L);
	lua_setfield(L, -2, "enum");

	// create framework table
	pushLuaFramework(L);
	lua_setfield(L, -2, "Framework");

	// add global GameState object
	pushToLua(L, game);
	lua_setfield(L, -2, "GameState");
	lua_setglobal(L, "OpenApoc");

	// add a pointer to the gamestate in the registry
	// since some methods may require it and it would be
	// awkward to get it from the global variable
	lua_pushlightuserdata(L, (void *)&game);
	lua_setfield(L, LUA_REGISTRYINDEX, "OpenApoc.GameState");

	pushLuaDebugTraceback(L);
	for (const UString &s : config().getString("OpenApoc.Mod.ScriptsList").split(";"))
	{
		if (s.empty())
			continue;

		// this is only true if loadfile or pcall raised an error
		if (luaL_loadfile(L, s.cStr()) || lua_pcall(L, 0, 0, -2))
		{
			handleLuaError(L);
		}
	}
	lua_pop(L, 1); // pop debug.traceback function

	this->initOk = true;
}

LuaGameState::operator bool() const { return this->L != nullptr && this->initOk; }

} // namespace OpenApoc
