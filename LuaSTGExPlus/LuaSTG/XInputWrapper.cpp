#include "LuaWrapper.h"
#include "E2DXInputImpl.h"
#include "AppFrame.h"

using namespace std;
using namespace LuaSTGPlus;

void XInputManagerWrapper::Register(lua_State* L)LNOEXCEPT
{
	struct WrapperImplement
	{
		static int GetKeyState(lua_State* L)
		{
			int index = luaL_checkinteger(L, 1);
			int vkey = luaL_checkinteger(L, 2);
			bool state = Eyes2D::GetXInput().GetKeyState(index, vkey);
			lua_pushboolean(L,state);
			return 1;
		}
		static int GetTriggerState(lua_State* L) {
			int index = luaL_checkinteger(L, 1);
			lua_Number left = Eyes2D::GetXInput().GetTriggerStateL(index);
			lua_Number right = Eyes2D::GetXInput().GetTriggerStateR(index);
			lua_pushnumber(L,left);
			lua_pushnumber(L, right);
			return 2;
		}
		static int GetThumbState(lua_State* L) {
			int index = luaL_checkinteger(L, 1);
			lua_Number LX = Eyes2D::GetXInput().GetThumbStateLX(index);
			lua_Number LY = Eyes2D::GetXInput().GetThumbStateLY(index);
			lua_Number RX = Eyes2D::GetXInput().GetThumbStateRX(index);
			lua_Number RY = Eyes2D::GetXInput().GetThumbStateRY(index);
			lua_pushnumber(L, LX);
			lua_pushnumber(L, LY);
			lua_pushnumber(L, RX);
			lua_pushnumber(L, RY);
			return 4;
		}
		static int SetMotorSpeed(lua_State* L) {
			int index = luaL_checkinteger(L, 1);
			int _L = luaL_checkinteger(L, 2);
			int _H = luaL_checkinteger(L, 3);
			bool ret = Eyes2D::GetXInput().SetMotorSpeed(index, _L, _H);
			lua_pushboolean(L, ret);
			return 1;
		}
		static int GetMotorSpeed(lua_State* L) {
			int index = luaL_checkinteger(L, 1);
			Eyes2D::XDeviceInfo info = Eyes2D::GetXInput().GetDeviceInfo(index);
			bool _FF = info.ForceFeedback;
			lua_Number _L = info.LMotorSpeed;
			lua_Number _H = info.HMotorSpeed;
			lua_pushnumber(L, _L);
			lua_pushnumber(L, _H);
			lua_pushboolean(L, _FF);
			return 3;
		}
		static int Refresh(lua_State* L) {
			lua_Number count = Eyes2D::GetXInput().Refresh();
			lua_pushnumber(L, count);
			return 1;
		}
		static int Update(lua_State* L) {
			Eyes2D::GetXInput().Update();
			return 0;
		}
		static int GetDeviceCount(lua_State* L) {
			lua_Number n = Eyes2D::GetXInput().GetDeviceCount();
			lua_pushnumber(L, n);
			return 1;
		}
		static int Meta_ToString(lua_State* L)LNOEXCEPT
		{
			lua_pushfstring(L, "lstg.XInputManager object");
			return 1;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ "GetKeyState", &WrapperImplement::GetKeyState },
		{ "GetTriggerState", &WrapperImplement::GetTriggerState },
		{ "GetThumbState", &WrapperImplement::GetThumbState },
		{ "SetMotorSpeed", &WrapperImplement::SetMotorSpeed },
		{ "GetMotorSpeed", &WrapperImplement::GetMotorSpeed },
		{ "Refresh", &WrapperImplement::Refresh },
		{ "Update", &WrapperImplement::Update },
		{ "GetDeviceCount", &WrapperImplement::GetDeviceCount },
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ NULL, NULL }
	};

	luaL_openlib(L, LUASTG_LUA_TYPENAME_XINPUTWRAPPER, tMethods, 0);  // t
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_XINPUTWRAPPER);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

void XInputManagerWrapper::CreateAndPush(lua_State * L)
{
	lua_newtable(L);//t
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_XINPUTWRAPPER);//t mt
	lua_setmetatable(L, -2);//t
}
