#include <string>
#include <algorithm>
#include "..\LuaWrapper.hpp"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace LuaSTGPlus
{
	namespace LuaWrapper
	{
		void ColorWrapper::Register(lua_State* L)LNOEXCEPT {
			struct Function {
#define _GETUDATA(i) static_cast<fcyColor*>(luaL_checkudata(L, (i), LUASTG_LUA_TYPENAME_COLOR));
#define GETUDATA(p, i) fcyColor* (p) = _GETUDATA(i);
				static int ARGB(lua_State* L)LNOEXCEPT
				{
					GETUDATA(p, 1);
					lua_pushnumber(L, p->a);
					lua_pushnumber(L, p->r);
					lua_pushnumber(L, p->g);
					lua_pushnumber(L, p->b);
					return 4;
				}

				static int Meta_Index(lua_State* L)LNOEXCEPT
				{
					GETUDATA(p, 1);
					switch (Xrysnow::ColorWrapperPropertyHash(L, 2))
					{
					case Xrysnow::ColorWrapperProperty::m_a:
						lua_pushinteger(L, (lua_Integer)p->a);
						break;
					case Xrysnow::ColorWrapperProperty::m_r:
						lua_pushinteger(L, (lua_Integer)p->r);
						break;
					case Xrysnow::ColorWrapperProperty::m_g:
						lua_pushinteger(L, (lua_Integer)p->g);
						break;
					case Xrysnow::ColorWrapperProperty::m_b:
						lua_pushinteger(L, (lua_Integer)p->b);
						break;
					case Xrysnow::ColorWrapperProperty::f_ARGB:
						lua_pushcfunction(L, ARGB);
					default:
						return luaL_error(L, "Invalid index key.");
					}
					return 1;
				}
				static int Meta_NewIndex(lua_State* L)LNOEXCEPT
				{
					GETUDATA(p, 1);
					lua_Integer set = luaL_checkinteger(L, 3);
					switch (Xrysnow::ColorWrapperPropertyHash(L, 2))
					{
					case Xrysnow::ColorWrapperProperty::m_a:
						p->a = (fByte)std::clamp(set, 0, 255);
						break;
					case Xrysnow::ColorWrapperProperty::m_r:
						p->r = (fByte)std::clamp(set, 0, 255);
						break;
					case Xrysnow::ColorWrapperProperty::m_g:
						p->g = (fByte)std::clamp(set, 0, 255);
						break;
					case Xrysnow::ColorWrapperProperty::m_b:
						p->b = (fByte)std::clamp(set, 0, 255);
						break;
					default:
						return luaL_error(L, "Invalid index key.");
					}
					return 0;
				}
				static int Meta_Eq(lua_State* L)LNOEXCEPT
				{
					GETUDATA(pA, 1);
					GETUDATA(pB, 2);
					lua_pushboolean(L, pA->argb == pB->argb);
					return 1;
				}
				static int Meta_Add(lua_State* L)LNOEXCEPT
				{
					lua_Number tFactor;
					fcyColor* p = nullptr;
					if (lua_isnumber(L, 1))  // arg1为数字，则arg2必为lstgColor
					{
						tFactor = luaL_checknumber(L, 1);
						p = _GETUDATA(2);
					}
					else if (lua_isnumber(L, 2))  // arg2为数字，则arg1必为lstgColor
					{
						tFactor = luaL_checknumber(L, 2);
						p = _GETUDATA(1);
					}
					else {
						GETUDATA(pA, 1);
						GETUDATA(pB, 2);
						ColorWrapper::CreateAndPush(L, fcyColor(
							std::min((int)pA->a + pB->a, 255),
							std::min((int)pA->r + pB->r, 255),
							std::min((int)pA->g + pB->g, 255),
							std::min((int)pA->b + pB->b, 255)
						));
						return 1;
					}
					ColorWrapper::CreateAndPush(L, fcyColor(
						(int)std::min((double)p->a + tFactor, 255.),
						(int)std::min((double)p->r + tFactor, 255.),
						(int)std::min((double)p->g + tFactor, 255.),
						(int)std::min((double)p->b + tFactor, 255.)
					));
					return 1;
				}
				static int Meta_Sub(lua_State* L)LNOEXCEPT
				{
					lua_Number tFactor;
					fcyColor* p = nullptr;
					if (lua_isnumber(L, 1))  // arg1为数字，则arg2必为lstgColor
					{
						tFactor = luaL_checknumber(L, 1);
						p = _GETUDATA(2);
					}
					else if (lua_isnumber(L, 2))  // arg2为数字，则arg1必为lstgColor
					{
						tFactor = luaL_checknumber(L, 2);
						p = _GETUDATA(1);
					}
					else {
						GETUDATA(pA, 1);
						GETUDATA(pB, 2);
						ColorWrapper::CreateAndPush(L, fcyColor(
							std::max((int)pA->a - pB->a, 0),
							std::max((int)pA->r - pB->r, 0),
							std::max((int)pA->g - pB->g, 0),
							std::max((int)pA->b - pB->b, 0)
						));
						return 1;
					}
					ColorWrapper::CreateAndPush(L, fcyColor(
						(int)std::max((double)p->a - tFactor, 0.),
						(int)std::max((double)p->r - tFactor, 0.),
						(int)std::max((double)p->g - tFactor, 0.),
						(int)std::max((double)p->b - tFactor, 0.)
					));
					return 1;
				}
				static int Meta_Mul(lua_State* L)LNOEXCEPT
				{
					lua_Number tFactor;
					fcyColor* p = nullptr;
					if (lua_isnumber(L, 1))  // arg1为数字，则arg2必为lstgColor
					{
						tFactor = luaL_checknumber(L, 1);
						p = _GETUDATA(2);
					}
					else if (lua_isnumber(L, 2))  // arg2为数字，则arg1必为lstgColor
					{
						tFactor = luaL_checknumber(L, 2);
						p = _GETUDATA(1);
					}
					else  // arg1和arg2都必为lstgColor
					{
						GETUDATA(pA, 1);
						GETUDATA(pB, 2);
						ColorWrapper::CreateAndPush(L, fcyColor(
							std::min((int)pA->a * pB->a, 255),
							std::min((int)pA->r * pB->r, 255),
							std::min((int)pA->g * pB->g, 255),
							std::min((int)pA->b * pB->b, 255)
						));
						return 1;
					}
					ColorWrapper::CreateAndPush(L, fcyColor(
						(int)std::min((double)p->a * tFactor, 255.),
						(int)std::min((double)p->r * tFactor, 255.),
						(int)std::min((double)p->g * tFactor, 255.),
						(int)std::min((double)p->b * tFactor, 255.)
					));
					return 1;
				}
				static int Meta_ToString(lua_State* L)LNOEXCEPT
				{
					GETUDATA(p, 1);
					lua_pushfstring(L, "lstg.Color(%d,%d,%d,%d)", p->a, p->r, p->g, p->b);
					return 1;
				}
#undef _GETUDATA
#undef GETUDATA
			};

			luaL_Reg tMethods[] =
			{
				{ "ARGB", &Function::ARGB },
				{ NULL, NULL }
			};

			luaL_Reg tMetaTable[] =
			{
				{ "__index", &Function::Meta_Index },
				{ "__newindex", &Function::Meta_NewIndex },
				{ "__eq", &Function::Meta_Eq },
				{ "__add", &Function::Meta_Add },
				{ "__sub", &Function::Meta_Sub },
				{ "__mul", &Function::Meta_Mul },
				{ "__tostring", &Function::Meta_ToString },
				{ NULL, NULL }
			};

			RegisterClassIntoTable2(L, ".Color", tMethods, LUASTG_LUA_TYPENAME_COLOR, tMetaTable);
		}

		void ColorWrapper::CreateAndPush(lua_State* L, const fcyColor& color) {
			fcyColor* p = static_cast<fcyColor*>(lua_newuserdata(L, sizeof(fcyColor))); // udata
			new(p) fcyColor(color);
			luaL_getmetatable(L, LUASTG_LUA_TYPENAME_COLOR); // udata mt
			lua_setmetatable(L, -2); // udata
		}
	}
}
