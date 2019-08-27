/// @file LuaWrapper.h
/// @brief lua包装层 用于导出C++函数和类
#pragma once
#include "..\Global.h"
#include "LuaWrapperMisc.hpp"
#include "..\ResourceMgr.h"
#include "..\GameObjectPool.h"
#include "..\LuaStringToEnum.hpp"

#define LUASTG_LUA_LIBNAME "lstg"
#define LUASTG_LUA_LIBNAME_IO "IO"

#define LUASTG_LUA_TYPENAME_IO_STREAM "lstg.IO.Stream"
#define LUASTG_LUA_TYPENAME_IO_BINARY_READER "lstg.IO.BinaryReader"
#define LUASTG_LUA_TYPENAME_IO_BINARY_WRITER "lstg.IO.BinaryWriter"

#define LUASTG_LUA_TYPENAME_COLOR "lstg.Color"
#define LUASTG_LUA_TYPENAME_RANDGEN "lstgRand"
#define LUASTG_LUA_TYPENAME_BENTLASER "lstgBentLaserData"
#define LUASTG_LUA_TYPENAME_STOPWATCH "lstgStopWatch"
#define LUASTG_LUA_TYPENAME_XINPUTWRAPPER "lstgXInputWrapper"
#define LUASTG_LUA_TYPENAME_COLLIDERWRAPPER "lstgColliderWrapper"
#define LUASTG_LUA_TYPENAME_RESOURCE "lstgResource"
#define LUASTG_LUA_TYPENAME_ARCHIVE "lstgArchive"

namespace LuaSTGPlus
{
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

	namespace LuaWrapper {
		class ColorWrapper
		{
		public:
			static void Register(lua_State* L)LNOEXCEPT;
			static void CreateAndPush(lua_State* L, const fcyColor& color);
		};

		namespace IO {
			class StreamWrapper
			{
			public:
				struct Wrapper {
					fcyStream* handle;
				};
			public:
				static void Register(lua_State* L)LNOEXCEPT;
				static void CreateAndPush(lua_State* L, fcyStream* handle);
			};

			class BinaryReaderWrapper
			{
			public:
				struct Wrapper {
					fcyBinaryReader* handle;
				};
			public:
				static void Register(lua_State* L)LNOEXCEPT;
				static void CreateAndPush(lua_State* L, fcyStream* handle);
			};

			class BinaryWriterWrapper
			{
			public:
				struct Wrapper {
					fcyBinaryWriter* handle;
				};
			public:
				static void Register(lua_State* L)LNOEXCEPT;
				static void CreateAndPush(lua_State* L, fcyStream* handle);
			};

			void Register(lua_State* L)LNOEXCEPT;
		}

		void Register(lua_State* L)LNOEXCEPT;
	}

	void RegistBuiltInClassWrapper(lua_State* L)LNOEXCEPT;
}
