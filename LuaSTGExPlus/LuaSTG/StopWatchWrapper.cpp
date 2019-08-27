#include "LuaWrapper\LuaWrapper.hpp"
#include "AppFrame.h"

using namespace std;
using namespace LuaSTGPlus;

void Fancy2dStopWatchWrapper::Register(lua_State* L)LNOEXCEPT
{
	struct WrapperImplement
	{
		static int Reset(lua_State* L)
		{
			fcyStopWatch* p = static_cast<fcyStopWatch*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_STOPWATCH));
			p->Reset();
			return 1;
		}
		static int Pause(lua_State* L)
		{
			fcyStopWatch* p = static_cast<fcyStopWatch*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_STOPWATCH));
			p->Pause();
			return 1;
		}
		static int Resume(lua_State* L)
		{
			fcyStopWatch* p = static_cast<fcyStopWatch*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_STOPWATCH));
			p->Resume();
			return 1;
		}
		static int GetElapsed(lua_State* L)
		{
			fcyStopWatch* p = static_cast<fcyStopWatch*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_STOPWATCH));
			fDouble tCross = p->GetElapsed();
			lua_pushnumber(L, tCross);
			return 1;
		}
		static int Meta_ToString(lua_State* L)LNOEXCEPT
		{
			fcyStopWatch* p = static_cast<fcyStopWatch*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_STOPWATCH));
			lua_pushfstring(L, "lstg.StopWatch object");
			return 1;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ "Reset", &WrapperImplement::Reset },
		{ "Pause", &WrapperImplement::Pause },
		{ "Resume", &WrapperImplement::Resume },
		{ "GetElapsed", &WrapperImplement::GetElapsed },
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ NULL, NULL }
	};

	luaL_openlib(L, LUASTG_LUA_TYPENAME_STOPWATCH, tMethods, 0);  // t
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_STOPWATCH);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

fcyStopWatch * Fancy2dStopWatchWrapper::CreateAndPush(lua_State * L)
{
	fcyStopWatch* p = static_cast<fcyStopWatch*>(lua_newuserdata(L, sizeof(fcyStopWatch)));
	new(p) fcyStopWatch();  // 构造
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_STOPWATCH);
	lua_setmetatable(L, -2);
	return p;
}
