#include "LuaWrapper.h"
#include "AppFrame.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

using namespace std;
using namespace LuaSTGPlus;

void ColorWrapper::Register(lua_State* L)LNOEXCEPT
{
	struct WrapperImplement
	{
		static int ARGB(lua_State* L)LNOEXCEPT
		{
			fcyColor* p = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			lua_pushnumber(L, p->a);
			lua_pushnumber(L, p->r);
			lua_pushnumber(L, p->g);
			lua_pushnumber(L, p->b);
			return 4;
		}
		static int Meta_Eq(lua_State* L)LNOEXCEPT
		{
			fcyColor* pA = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			fcyColor* pB = static_cast<fcyColor*>(luaL_checkudata(L, 2, LUASTG_LUA_TYPENAME_COLOR));
			lua_pushboolean(L, pA->argb == pB->argb);
			return 1;
		}
		static int Meta_Add(lua_State * L)LNOEXCEPT
		{
			fcyColor* pA = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			fcyColor* pB = static_cast<fcyColor*>(luaL_checkudata(L, 2, LUASTG_LUA_TYPENAME_COLOR));
			fcyColor* pResult = CreateAndPush(L);
			pResult->Set(
				::min((int)pA->a + pB->a, 255),
				::min((int)pA->r + pB->r, 255),
				::min((int)pA->g + pB->g, 255),
				::min((int)pA->b + pB->b, 255)
			);
			return 1;
		}
		static int Meta_Sub(lua_State * L)LNOEXCEPT
		{
			fcyColor* pA = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			fcyColor* pB = static_cast<fcyColor*>(luaL_checkudata(L, 2, LUASTG_LUA_TYPENAME_COLOR));
			fcyColor* pResult = CreateAndPush(L);
			pResult->Set(
				::max((int)pA->a - pB->a, 0),
				::max((int)pA->r - pB->r, 0),
				::max((int)pA->g - pB->g, 0),
				::max((int)pA->b - pB->b, 0)
			);
			return 1;
		}
		static int Meta_Mul(lua_State * L)LNOEXCEPT
		{
			lua_Number tFactor;
			fcyColor* p = nullptr, * pResult = nullptr;
			if (lua_isnumber(L, 1))  // arg1为数字，则arg2必为lstgColor
			{
				tFactor = luaL_checknumber(L, 1);
				p = static_cast<fcyColor*>(luaL_checkudata(L, 2, LUASTG_LUA_TYPENAME_COLOR));
			}
			else if (lua_isnumber(L, 2))  // arg2为数字，则arg1必为lstgColor
			{
				tFactor = luaL_checknumber(L, 2);
				p = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			}
			else  // arg1和arg2都必为lstgColor
			{
				fcyColor* pA = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
				fcyColor* pB = static_cast<fcyColor*>(luaL_checkudata(L, 2, LUASTG_LUA_TYPENAME_COLOR));
				pResult = CreateAndPush(L);
				pResult->Set(
					::min((int)pA->a * pB->a, 255),
					::min((int)pA->r * pB->r, 255),
					::min((int)pA->g * pB->g, 255),
					::min((int)pA->b * pB->b, 255)
				);
				return 1;
			}
			pResult = CreateAndPush(L);
			pResult->Set(
				(int)::min((double)p->a * tFactor, 255.),
				(int)::min((double)p->r * tFactor, 255.),
				(int)::min((double)p->g * tFactor, 255.),
				(int)::min((double)p->b * tFactor, 255.)
			);
			return 1;
		}
		static int Meta_ToString(lua_State * L)LNOEXCEPT
		{
			fcyColor* p = static_cast<fcyColor*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLOR));
			lua_pushfstring(L, "lstg.Color(%d,%d,%d,%d)", p->a, p->r, p->g, p->b);
			return 1;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ "ARGB", &WrapperImplement::ARGB },
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__eq", &WrapperImplement::Meta_Eq },
		{ "__add", &WrapperImplement::Meta_Add },
		{ "__sub", &WrapperImplement::Meta_Sub },
		{ "__mul", &WrapperImplement::Meta_Mul },
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ NULL, NULL }
	};

	luaL_openlib(L, LUASTG_LUA_TYPENAME_COLOR, tMethods, 0);  // t //注册到全局表
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_COLOR);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt //注册到metetable
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

fcyColor* ColorWrapper::CreateAndPush(lua_State* L)
{
	fcyColor* p = static_cast<fcyColor*>(lua_newuserdata(L, sizeof(fcyColor)));
	new(p) fcyColor();  // 构造
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_COLOR);
	lua_setmetatable(L, -2);
	return p;
}
