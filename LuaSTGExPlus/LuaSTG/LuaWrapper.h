/// @file LuaWrapper.h
/// @brief lua包装层 用于导出C++函数和类
#pragma once
#include "Global.h"
#include "ResourceMgr.h"
#include "GameObjectPool.h"

#define LUASTG_LUA_TYPENAME_COLOR "lstgColor"
#define LUASTG_LUA_TYPENAME_RANDGEN "lstgRand"
#define LUASTG_LUA_TYPENAME_BENTLASER "lstgBentLaserData"
#define LUASTG_LUA_TYPENAME_STOPWATCH "lstgStopWatch"
#define LUASTG_LUA_TYPENAME_XINPUTWRAPPER "lstgXInputWrapper"
#define LUASTG_LUA_TYPENAME_COLLIDERWRAPPER "lstgColliderWrapper"
#define LUASTG_LUA_TYPENAME_RESOURCE "lstgResource"
#define LUASTG_LUA_TYPENAME_ARCHIVE "lstgArchive"

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

	//颜色包装
	class ColorWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个颜色类并推入堆栈
		static fcyColor* CreateAndPush(lua_State* L);
	};

	//随机数发生器包装
	class RandomizerWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个颜色类并推入堆栈
		static fcyRandomWELL512* CreateAndPush(lua_State* L);
	};

	//曲线激光包装
	class BentLaserDataWrapper
	{
	private:
		struct Wrapper
		{
			GameObjectBentLaser* handle;
		};
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个曲线激光类并推入堆栈
		static GameObjectBentLaser* CreateAndPush(lua_State* L);
	};
	
	//高精度纳秒级停表
	class Fancy2dStopWatchWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个高精度纳秒级停表类并推入堆栈
		static fcyStopWatch* CreateAndPush(lua_State* L);
	};

	//内建函数包装
	class BuiltInFunctionWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
	};
	
	//游戏资源包装对象，通过一个结构来保存多种资源（其实我应该用union的23333）
	//ResourceWrapper结构内有多个指针，以及一个枚举量，用于判断当前保存的资源对象指针
	//该lua类会对保存的资源进行引用次数+1，被GC回收后解除引用
	class GameResourceWrapper
	{
	public:
		struct ResourceWrapper
		{
			ResourceType m_type;
			fcyRefPointer<ResTexture> m_tex;
			fcyRefPointer<ResSprite> m_img;
			fcyRefPointer<ResAnimation> m_ani;
			fcyRefPointer<ResMusic> m_bgm;
			fcyRefPointer<ResSound> m_se;
			fcyRefPointer<ResParticle> m_ps;
			fcyRefPointer<ResFont> m_fnt;//TTF和HGEfont
			fcyRefPointer<ResFX> m_fx;
		};
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		//推入一个游戏资源类到堆栈中
		static ResourceWrapper* CreateAndPush(lua_State* L);
	};

	//XInput的lua包装
	class XInputManagerWrapper {
	public:
		//向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
	};

	//游戏碰撞体操作
	class GameObjectColliderWrapper {
	private:
		struct Wrapper
		{
			GameObject* handle;
			int cur;
			size_t id;
			int64_t uid;
			Wrapper() { cur = 0; handle = nullptr; id = 0; uid = 0; }
		};
	public:
		//向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		//创建一个游戏碰撞体包装类并推入堆栈
		static void CreateAndPush(lua_State* L, GameObject* obj)LNOEXCEPT;
	};

	//压缩包
	class ArchiveWrapper {
	private:
		struct Wrapper;
	public:
		static void Register(lua_State* L)LNOEXCEPT;
		static void CreateAndPush(lua_State* L, unsigned int uid)LNOEXCEPT;
	};

	//文件资源管理
	class FileManagerWrapper {
	public:
		static void Register(lua_State* L)LNOEXCEPT;
	};

	//注册内建类
	static inline void RegistBuiltInClassWrapper(lua_State* L)LNOEXCEPT {
		ColorWrapper::Register(L);  // 颜色对象
		RandomizerWrapper::Register(L);  // 随机数发生器
		BentLaserDataWrapper::Register(L);  // 曲线激光
		Fancy2dStopWatchWrapper::Register(L);  // 高精度停表
		BuiltInFunctionWrapper::Register(L);  // 内建函数库
		FileManagerWrapper::Register(L); //内建函数库，文件资源管理，请确保位于内建函数库后加载
		ArchiveWrapper::Register(L); //压缩包
		GameResourceWrapper::Register(L);  // 游戏资源对象
		XInputManagerWrapper::Register(L);  //内建函数库，XInput，请确保位于内建函数库后加载
#ifdef USING_ADVANCE_COLLIDER
		GameObjectColliderWrapper::Register(L);//Collider
#endif // USING_ADVANCE_COLLIDER
	}

	//翻译字符串到混合模式
	static inline BlendMode TranslateBlendMode(lua_State* L, int argnum)
	{
		/*
		const char* s = luaL_checkstring(L, argnum);
		if (strcmp(s, "") == 0 || strcmp(s, "mul+alpha") == 0)
			return BlendMode::MulAlpha;
		else if (strcmp(s, "mul+add") == 0)
			return BlendMode::MulAdd;
		else if (strcmp(s, "add+add") == 0)
			return BlendMode::AddAdd;
		else if (strcmp(s, "add+alpha") == 0)
			return BlendMode::AddAlpha;
		else if (strcmp(s, "add+rev") == 0)
			return BlendMode::AddRev;
		else if (strcmp(s, "mul+rev") == 0)
			return BlendMode::MulRev;
		else if (strcmp(s, "add+sub") == 0)
			return BlendMode::AddSub;
		else if (strcmp(s, "mul+sub") == 0)
			return BlendMode::MulSub;
		else if (strcmp(s, "alpha+bal") == 0)
			return BlendMode::AlphaBal;
		else
			luaL_error(L, "invalid blend mode '%s'.", s);
		return BlendMode::MulAlpha;
		*/

		const char* key = luaL_checkstring(L, argnum);

		if (strcmp(key, "") == 0 || strcmp(key, "mul+alpha") == 0)
			return BlendMode::MulAlpha;

		static const char* s_orgKeyList[] =
		{
			"__reserve__",
			"mul+alpha",
			"mul+add",
			"mul+rev",
			"mul+sub",
			"add+alpha",
			"add+add",
			"add+rev",
			"add+sub",
			"alpha+bal",
			"mul+min",
			"mul+max",
			"mul+mul",
			"mul+screen",
			"add+min",
			"add+max",
			"add+mul",
			"add+screen",
		};

		static const unsigned int s_bestIndices[] =
		{
			1, 6,
		};

		static const unsigned int s_hashTable1[] =
		{
			11, 243,
		};

		static const unsigned int s_hashTable2[] =
		{
			148, 194,
		};

		static const unsigned int s_hashTableG[] =
		{
			0, 0, 0, 0, 0, 7, 0, 0, 3, 2,
			15, 0, 8, 15, 3, 0, 0, 15, 10, 14,
			4, 3, 9, 0, 9, 4, 7, 8, 10,
		};

		unsigned int f1 = 0, f2 = 0, len = strlen(key);
		for (unsigned int i = 0; i < 2; ++i)
		{
			unsigned int idx = s_bestIndices[i];
			if (idx < len)
			{
				f1 = (f1 + s_hashTable1[i] * (unsigned int)key[idx]) % 29;
				f2 = (f2 + s_hashTable2[i] * (unsigned int)key[idx]) % 29;
			}
			else
				break;
		}

		unsigned int hash = (s_hashTableG[f1] + s_hashTableG[f2]) % 18;
		if (strcmp(s_orgKeyList[hash], key) == 0)
			return static_cast<BlendMode>(hash);
		luaL_error(L, "invalid blend mode '%s'.", key);
		return BlendMode::MulAlpha;
	}
}
