#include "LuaWrapper.h"
#include "AppFrame.h"

using namespace std;
using namespace LuaSTGPlus;

void BentLaserDataWrapper::Register(lua_State* L)LNOEXCEPT
{
	struct WrapperImplement
	{
		static int Update(lua_State* L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			if (!lua_istable(L, 2))
				return luaL_error(L, "invalid lstg object for 'Update'.");
			lua_rawgeti(L, 2, 2);  // self t(object) ??? id
			size_t id = (size_t)luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			if (!p->handle->Update(id, luaL_checkinteger(L, 3), (float)luaL_checknumber(L, 4), luaL_optnumber(L, 5, 0) == 0))
				return luaL_error(L, "invalid lstg object for 'Update'.");
			return 0;
		}
		static int UpdateNode(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			if (!lua_istable(L, 2))
				return luaL_error(L, "invalid lstg object for 'UpdateNode'.");
			lua_rawgeti(L, 2, 2);  // self t(object) ??? id
			size_t id = (size_t)luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			if (!p->handle->UpdateByNode(id, luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), (float)luaL_checknumber(L, 5), luaL_optnumber(L, 6, 0) == 0))
				return luaL_error(L, "invalid lstg object for 'UpdateNode'.");
			return 0;
		}
		static int UpdatePositionByList(lua_State * L)LNOEXCEPT // u(laser) t(list) length width index revert 
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			if (!lua_istable(L, 2))
				return luaL_error(L, "invalid lstg object for 'Update'.");
			//lua_rawgeti(L, 2, 2);  // self t(object) ??? id
			//size_t id = (size_t)luaL_checkinteger(L, -1);
			//lua_pop(L, 1);

			int i3 = luaL_checkinteger(L, 3);
			float f4 = luaL_checknumber(L, 4);
			int i5 = luaL_optinteger(L, 5, 1);
			bool i6 = luaL_optinteger(L, 6, 0) != 0;
			// ... t(list)
			lua_settop(L, 2);

			if (!p->handle->UpdatePositionByList(L,
				i3,
				f4,
				i5,
				i6
			))
				return luaL_error(L, "Update laser data failed.");
			return 0;
		}
		static int SampleByLength(lua_State * L)LNOEXCEPT // t(self) <length>
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			float length = (float)luaL_checknumber(L, 2);
			lua_pop(L, 2); // 
			p->handle->SampleL(L, length); // t(list)
			return 1;
		}
		static int SampleByTime(lua_State * L)LNOEXCEPT // t(self) <length>
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			float time = (float)luaL_checknumber(L, 2);
			lua_pop(L, 2); // 
			p->handle->SampleT(L, time / 60.0f); // t(list)
			return 1;
		}

		static int Release(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (p->handle)
			{
				GameObjectBentLaser::FreeInstance(p->handle);
				p->handle = nullptr;
			}
			return 0;
		}
		static int Render(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			if (!p->handle->Render(
				luaL_checkstring(L, 2),
				TranslateBlendMode(L, 3),
				*static_cast<fcyColor*>(luaL_checkudata(L, 4, LUASTG_LUA_TYPENAME_COLOR)),
				(float)luaL_checknumber(L, 5),
				(float)luaL_checknumber(L, 6),
				(float)luaL_checknumber(L, 7),
				(float)luaL_checknumber(L, 8),
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				(float)luaL_optnumber(L, 9, 1.) * LRES.GetGlobalImageScaleFactor()
#else
				(float)luaL_optnumber(L, 9, 1.)
#endif // GLOBAL_SCALE_COLLI_SHAPE
			))
			{
				return luaL_error(L, "can't render object with texture '%s'.", luaL_checkstring(L, 2));
			}
			return 0;
		}
		static int CollisionCheck(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			bool r = p->handle->CollisionCheck(
				(float)luaL_checknumber(L, 2),
				(float)luaL_checknumber(L, 3),
				(float)luaL_optnumber(L, 4, 0),
				(float)luaL_optnumber(L, 5, 0),
				(float)luaL_optnumber(L, 6, 0),
				lua_toboolean(L, 7) == 0 ? false : true
			);
			lua_pushboolean(L, r);
			return 1;
		}
		static int CollisionCheckWidth(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			bool r = p->handle->CollisionCheckW(
				(float)luaL_checknumber(L, 2),
				(float)luaL_checknumber(L, 3),
				(float)luaL_optnumber(L, 5, 0),
				(float)luaL_optnumber(L, 6, 0),
				(float)luaL_optnumber(L, 7, 0),
				lua_toboolean(L, 8) == 0 ? false : true,
				(float)luaL_checknumber(L, 4)
			);
			lua_pushboolean(L, r);
			return 1;
		}
		static int BoundCheck(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			bool r = p->handle->BoundCheck();
			lua_pushboolean(L, r);
			return 1;
		}
		static int SetAllWidth(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (!p->handle)
				return luaL_error(L, "lstgBentLaserData was released.");
			p->handle->SetAllWidth((float)luaL_checknumber(L, 2));

			return 0;
		}
		static int Meta_ToString(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			lua_pushfstring(L, "lstg.BentLaserData object");
			return 1;
		}
		static int Meta_GC(lua_State * L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_BENTLASER));
			if (p->handle)
			{
				GameObjectBentLaser::FreeInstance(p->handle);
				p->handle = nullptr;
			}
			return 0;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ "Update", &WrapperImplement::Update },
		{ "UpdateNode", &WrapperImplement::UpdateNode },
		{ "Release", &WrapperImplement::Release },
		{ "Render", &WrapperImplement::Render },
		{ "CollisionCheck", &WrapperImplement::CollisionCheck },
		{ "CollisionCheckWidth", &WrapperImplement::CollisionCheckWidth },
		{ "BoundCheck", &WrapperImplement::BoundCheck },
		{ "SampleByLength", &WrapperImplement::SampleByLength },
		{ "SampleByTime", &WrapperImplement::SampleByTime },
		{ "UpdatePositionByList", &WrapperImplement::UpdatePositionByList },
		{ "SetAllWidth", &WrapperImplement::SetAllWidth },
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ "__gc", &WrapperImplement::Meta_GC },
		{ NULL, NULL }
	};

	luaL_openlib(L, LUASTG_LUA_TYPENAME_BENTLASER, tMethods, 0);  // t
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_BENTLASER);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

GameObjectBentLaser * BentLaserDataWrapper::CreateAndPush(lua_State * L)
{
	Wrapper* p = static_cast<Wrapper*>(lua_newuserdata(L, sizeof(Wrapper)));
	p->handle = GameObjectBentLaser::AllocInstance();
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_BENTLASER);
	lua_setmetatable(L, -2);
	return p->handle;
}
