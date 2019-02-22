/// @file LuaWrapper.h
/// @brief lua包装层 用于导出C++函数和类
#pragma once
#include "Global.h"
#include "ResourceMgr.h"

namespace LuaSTGPlus
{
	/// @brief 颜色包装
	class ColorWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个颜色类并推入堆栈
		static fcyColor* CreateAndPush(lua_State* L);
	};

	/// @brief 随机数发生器包装
	class RandomizerWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个颜色类并推入堆栈
		static fcyRandomWELL512* CreateAndPush(lua_State* L);
	};

	//这里预定义了这个玩意是为了方便BentLaserDataWrapper和GameObjectBentLaser对接
	//然而如果和另一个正式定义了GameObjectBentLaser的头文件放一起会出事
#ifndef DEFINE_GAME_OBJECT_BENTLAZER_CLASS
	class GameObjectBentLaser;
#endif

	/// @brief 曲线激光包装
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
	
	/// @brief f2d高精度纳秒级停表
	class Fancy2dStopWatchWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
		/// @brief 创建一个高精度纳秒级停表类并推入堆栈
		static fcyStopWatch* CreateAndPush(lua_State* L);
	};

	/// @brief 内建函数包装
	class BuiltInFunctionWrapper
	{
	public:
		/// @brief 向lua注册包装类
		static void Register(lua_State* L)LNOEXCEPT;
	};

	//游戏资源包装对象，通过一个结构来保存多种资源（其实我应该用union的23333）
	//ResourceWrapper结构内有多个指针，以及一个枚举量，用于判断当前保存的资源对象指针
	//该lua类会对保存的资源进行引用次数+1，被GC回收后解除引用
	/// @brief 游戏资源
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
		/// @brief 推入一个游戏资源类到堆栈中
		template <typename T>
		static T CreateAndPush(lua_State* L, T res);
	};

	/// @brief 加载内建类
	static inline void RegistBuiltInClassWrapper(lua_State* L)LNOEXCEPT {
		ColorWrapper::Register(L);  // 颜色对象
		RandomizerWrapper::Register(L);  // 随机数发生器
		BentLaserDataWrapper::Register(L);  // 曲线激光
		Fancy2dStopWatchWrapper::Register(L);  // 高精度停表
		BuiltInFunctionWrapper::Register(L);  // 内建函数库
		GameResourceWrapper::Register(L);  // 游戏资源对象
	}
}
