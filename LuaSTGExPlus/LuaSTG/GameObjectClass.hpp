#pragma once
#include "lua.hpp"
#include "Global.h"

#define LOBJ_CLASS_ISCLASS         "is_class"
#define LOBJ_CLASS_RENDERCLASS     ".render"
#define LOBJ_CLASS_DEFAULTFUNCTION "default_function"

namespace LuaSTGPlus {
	struct GameObjectClass {
		bool IsDefaultUpdate;
		bool IsDefaultRender;
		bool IsRenderClass;

		void Reset() {
			IsDefaultUpdate = false;
			IsDefaultRender = false;
			IsRenderClass = false;
		}

		void CheckClassClass(lua_State* L, int index) {
			// ??? class ???
			lua_pushstring(L, LOBJ_CLASS_DEFAULTFUNCTION);	// ??? class ??? k 
			if (index < 0) {
				lua_rawget(L, index - 1);					// ??? class ??? n 
			}
			else {
				lua_rawget(L, index);						// ??? class ??? n 
			}
			if (lua_isnumber(L, -1)) {
				lua_Integer mask = luaL_checkinteger(L, -1);// ??? class ??? n 
				lua_pop(L, 1);								// ??? class ??? 
				if (mask & (1 << LGOBJ_CC_FRAME)) {
					IsDefaultUpdate = true;
				}
				if (mask & (1 << LGOBJ_CC_RENDER)) {
					IsDefaultRender = true;
				}
			}
			lua_pushstring(L, LOBJ_CLASS_RENDERCLASS);		// ??? class ??? k 
			if (index < 0) {
				lua_rawget(L, index - 1);					// ??? class ??? b 
			}
			else {
				lua_rawget(L, index);						// ??? class ??? b 
			}
			if (lua_isboolean(L, -1)) {
				IsRenderClass = lua_toboolean(L, -1);		// ??? class ??? b 
				lua_pop(L, 1);								// ??? class ??? 
			}
		}

		void CheckGameObjectClass(lua_State* L, int index) {
			// ??? object ???
			lua_rawgeti(L, index, 1);	// ??? object ??? class 
			CheckClassClass(L, -1);
			lua_pop(L, 1);				// ??? object ??? 
		}
	};
}
