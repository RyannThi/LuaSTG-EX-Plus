#include "LuaWrapper.h"
#include "AppFrame.h"

using namespace std;
using namespace LuaSTGPlus;

void GameResourceWrapper::Register(lua_State* L)LNOEXCEPT
{
	struct WrapperImplement
	{
		static int Meta_ToString(lua_State* L)LNOEXCEPT
		{
			ResourceWrapper* p = static_cast<ResourceWrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_RESOURCE));
			string info = "lstg.Resource object,type :";
			switch (p->m_type) {
			case ResourceType::Texture:
				info = info + "texture";
				break;
			case ResourceType::Sprite:
				info = info + "image";
				break;
			case ResourceType::Animation:
				info = info + "animation";
				break;
			case ResourceType::Music:
				info = info + "music";
				break;
			case ResourceType::SoundEffect:
				info = info + "sound";
				break;
			case ResourceType::Particle:
				info = info + "particle";
				break;
			case ResourceType::SpriteFont:
				info = info + "texture font";
				break;
			case ResourceType::TrueTypeFont:
				info = info + "ttf";
				break;
			case ResourceType::FX:
				info = info + "shader";
				break;
			}
			lua_pushfstring(L, info.c_str());
			return 1;
		}
		static int Meta_GC(lua_State* L)LNOEXCEPT
		{
			ResourceWrapper* p = static_cast<ResourceWrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_RESOURCE));
			switch (p->m_type) {
			case ResourceType::Texture:
				p->m_tex->Release();
				break;
			case ResourceType::Sprite:
				p->m_img->Release();
				break;
			case ResourceType::Animation:
				p->m_ani->Release();
				break;
			case ResourceType::Music:
				p->m_bgm->Release();
				break;
			case ResourceType::SoundEffect:
				p->m_se->Release();
				break;
			case ResourceType::Particle:
				p->m_ps->Release();
				break;
			case ResourceType::SpriteFont:
			case ResourceType::TrueTypeFont:
				p->m_fnt->Release();
				break;
			case ResourceType::FX:
				p->m_fx->Release();
				break;
			}
			return 0;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ "__gc", &WrapperImplement::Meta_GC },
		{ NULL, NULL }
	};

	luaL_openlib(L, LUASTG_LUA_TYPENAME_RESOURCE, tMethods, 0);  // t
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_RESOURCE);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

GameResourceWrapper::ResourceWrapper* GameResourceWrapper::CreateAndPush(lua_State* L)
{
	ResourceWrapper* p = static_cast<ResourceWrapper*>(lua_newuserdata(L, sizeof(ResourceWrapper)));
	new(p) ResourceWrapper();  // 构造
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_RESOURCE);
	lua_setmetatable(L, -2);
	return p;
}
