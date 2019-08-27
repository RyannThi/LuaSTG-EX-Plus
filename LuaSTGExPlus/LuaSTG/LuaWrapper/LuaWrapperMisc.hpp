#pragma once
#include "..\Global.h"
#include "..\ResourceBase.hpp"
#include "..\ResourceFont.hpp"
#include "..\LuaStringToEnum.hpp"

namespace LuaSTGPlus
{
	//注册方法和元方法到名字为name的库和元表中，并保护元表不被修改
	inline void RegisterMethodD(lua_State* L, const char* name, luaL_Reg* method, luaL_Reg* metamethod) {
		luaL_register(L, name, method);     // t        //将方法注册到全局表(library)，作为静态方法
		luaL_newmetatable(L, name);         // t mt     //在注册表中创建元表
		luaL_register(L, NULL, metamethod); // t mt     //将元方法推入元表内
		lua_pushstring(L, "__index");       // t mt s   //__index元方法
		lua_pushvalue(L, -3);               // t mt s t //对应的table
		lua_rawset(L, -3);                  // t mt     //设置__index元方法
		lua_pushstring(L, "__metatable");   // t mt s   //__metatable元方法
		lua_pushvalue(L, -3);               // t mt s t //对应的table
		lua_rawset(L, -3);                  // t mt     //设置__metatable元方法，保护元表不被修改
		lua_pop(L, 2);                      //          //清理
	}

	//注册方法和元方法到名字为name的库和元表中，并保护元表不被修改，不自动注册__index元方法
	inline void RegisterMethodS(lua_State* L, const char* name, luaL_Reg* method, luaL_Reg* metamethod) {
		luaL_register(L, name, method);     // t        //将方法注册到全局表(library)，作为静态方法
		luaL_newmetatable(L, name);         // t mt     //在注册表中创建元表
		luaL_register(L, NULL, metamethod); // t mt     //将元方法推入元表内
		lua_pushstring(L, "__metatable");   // t mt s   //__metatable元方法
		lua_pushvalue(L, -3);               // t mt s t //对应的table
		lua_rawset(L, -3);                  // t mt     //设置__metatable元方法，保护元表不被修改
		lua_pop(L, 2);                      //          //清理
	}
	
	//在栈顶的table创建一个名为name的新表，然后注册静态方法到该表中
	//注册元方法到注册表中名为metaname的元表中，并保护元表不被修改
	inline void RegisterClassIntoTable(lua_State* L, const char* name, luaL_Reg* method, const char* metaname, luaL_Reg* metamethod) {
		// ... t
		lua_pushstring(L, name);			// ... t s			//key
		lua_newtable(L);					// ... t s t		//储存静态方法的table
		luaL_register(L, NULL, method);		// ... t s t		//将方法注册到指定的table内，作为静态方法
		luaL_newmetatable(L, metaname);		// ... t s t mt		//在注册表中创建元表
		luaL_register(L, NULL, metamethod);	// ... t s t mt		//将元方法推入元表内
		lua_pushstring(L, "__index");		// ... t s t mt s	//__index元方法
		lua_pushvalue(L, -3);				// ... t s t mt s t	//静态方法
		lua_rawset(L, -3);					// ... t s t mt		//设置__index元方法
		lua_pushstring(L, "__metatable");	// ... t s t mt s	//__metatable元方法
		lua_pushvalue(L, -3);				// ... t s t mt s t	//静态方法
		lua_rawset(L, -3);					// ... t s t mt		//设置__metatable元方法，保护元表不被修改
		lua_pop(L, 1);						// ... t s t		//清理
		lua_settable(L, -3);				// ... t			//把静态方法塞进栈顶
	}

	//在栈顶的table创建一个名为name的新表，然后注册静态方法到该表中
	//注册元方法到注册表中名为metaname的元表中，并保护元表不被修改
	//不注册index元方法
	inline void RegisterClassIntoTable2(lua_State* L, const char* name, luaL_Reg* method, const char* metaname, luaL_Reg* metamethod) {
		// ... t
		lua_pushstring(L, name);			// ... t s			//key
		lua_newtable(L);					// ... t s t		//储存静态方法的table
		luaL_register(L, NULL, method);		// ... t s t		//将方法注册到指定的table内，作为静态方法
		luaL_newmetatable(L, metaname);		// ... t s t mt		//在注册表中创建元表
		luaL_register(L, NULL, metamethod);	// ... t s t mt		//将元方法推入元表内
		lua_pushstring(L, "__metatable");	// ... t s t mt s	//__metatable元方法
		lua_pushvalue(L, -3);				// ... t s t mt s t	//静态方法
		lua_rawset(L, -3);					// ... t s t mt		//设置__metatable元方法，保护元表不被修改
		lua_pop(L, 1);						// ... t s t		//清理
		lua_settable(L, -3);				// ... t			//把静态方法塞进栈顶
	}

	//翻译字符串到混合模式
	static inline BlendMode TranslateBlendMode(lua_State* L, int argnum)
	{
		const char* key = luaL_checkstring(L, argnum);
		if (strcmp(key, "") == 0 || strcmp(key, "mul+alpha") == 0)
			return BlendMode::MulAlpha;
		BlendMode mode = Xrysnow::BlendModeHash(L, argnum);
		if (mode == BlendMode::_KEY_NOT_FOUND) {
			luaL_error(L, "invalid blend mode '%s'.", key);
			return BlendMode::MulAlpha;
		}
		return mode;
	}

	//翻译混合模式回到lua string
	static inline int TranslateBlendModeToString(lua_State* L, BlendMode blendmode) {
		static const char* sc_sblendmodes[] = {
			"",
			"mul+alpha","mul+add","mul+rev","mul+sub",
			"add+alpha","add+add","add+rev","add+sub",
			"alpha+bal",
			"mul+min","mul+max","mul+mul","mul+screen",
			"add+min","add+max","add+mul","add+screen",
		};
		lua_pushstring(L, sc_sblendmodes[(int)blendmode]);
		return 1;
	}

	static inline void TranslateAlignMode(lua_State* L, int argnum, ResFont::FontAlignHorizontal& halign, ResFont::FontAlignVertical& valign)
	{
		int e = luaL_checkinteger(L, argnum);
		switch (e & 0x03)  // HGETEXT_HORZMASK
		{
		case 0:  // HGETEXT_LEFT
			halign = ResFont::FontAlignHorizontal::Left;
			break;
		case 1:  // HGETEXT_CENTER
			halign = ResFont::FontAlignHorizontal::Center;
			break;
		case 2:  // HGETEXT_RIGHT
			halign = ResFont::FontAlignHorizontal::Right;
			break;
		default:
			luaL_error(L, "invalid align mode.");
			return;
		}
		switch (e & 0x0C)  // HGETEXT_VERTMASK
		{
		case 0:  // HGETEXT_TOP
			valign = ResFont::FontAlignVertical::Top;
			break;
		case 4:  // HGETEXT_MIDDLE
			valign = ResFont::FontAlignVertical::Middle;
			break;
		case 8:  // HGETEXT_BOTTOM
			valign = ResFont::FontAlignVertical::Bottom;
			break;
		default:
			luaL_error(L, "invalid align mode.");
			return;
		}
	}

	static inline F2DTEXTUREADDRESS TranslateTextureSamplerAddress(lua_State* L, int argnum) {
		const char* s = luaL_checkstring(L, argnum);
		if (strcmp(s, "wrap") == 0)
			return F2DTEXTUREADDRESS_WRAP;
		else if (strcmp(s, "mirror") == 0)
			return F2DTEXTUREADDRESS_MIRROR;
		else if (strcmp(s, "clamp") == 0 || strcmp(s, "") == 0)
			return F2DTEXTUREADDRESS_CLAMP;
		else if (strcmp(s, "border") == 0)
			return F2DTEXTUREADDRESS_BORDER;
		else if (strcmp(s, "mirroronce") == 0)
			return F2DTEXTUREADDRESS_MIRRORONCE;
		else
			luaL_error(L, "Invalid texture sampler address mode '%s'.", s);
		return F2DTEXTUREADDRESS_CLAMP;
	}

	static inline F2DTEXFILTERTYPE TranslateTextureSamplerFilter(lua_State* L, int argnum) {
		const char* s = luaL_checkstring(L, argnum);
		if (strcmp(s, "point") == 0)
			return F2DTEXFILTER_POINT;
		else if (strcmp(s, "linear") == 0 || strcmp(s, "") == 0)
			return F2DTEXFILTER_LINEAR;
		else
			luaL_error(L, "Invalid texture sampler filter type '%s'.", s);
		return F2DTEXFILTER_LINEAR;
	}
}
