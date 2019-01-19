/// @file LuaWrapper.h
/// @brief lua包装层 用于导出C++函数和类
#pragma once
#include "Global.h"

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

	class GameObjectBentLaser;

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

	/// @brief 加载内建类
	static inline void RegistBuiltInClassWrapper(lua_State* L)LNOEXCEPT {
		ColorWrapper::Register(L);  // 颜色对象
		RandomizerWrapper::Register(L);  // 随机数发生器
		BentLaserDataWrapper::Register(L);  // 曲线激光
		Fancy2dStopWatchWrapper::Register(L);  // 高精度停表
		BuiltInFunctionWrapper::Register(L);  // 内建函数库
	}
}
