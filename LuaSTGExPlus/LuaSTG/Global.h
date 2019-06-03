﻿/// @file Global.h
/// @brief 全局定义文件
#pragma once

// C
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cmath>

// STL
#include <functional>
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <regex>
#include <array>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

// CRTDBG
#include <crtdbg.h>

// fancy2d
#include <fcyIO/fcyStream.h>
#include <fcyIO/fcyBinaryHelper.h>
#include <fcyParser/fcyPathParser.h>
#include <fcyParser/fcyIni.h>
#include <fcyMisc/fcyStopWatch.h>
#include <fcyMisc/fcyStringHelper.h>
#include <fcyMisc/fcyRandom.h>
#include <fcyOS/fcyMemPool.h>
#include <f2d.h>

// Zlib
#include <zlib.h>
#include <unzip.h>

// luajit
#include <lua.hpp>

// 日志系统
#include "LogSystem.h"

// 一些全局范围的宏
#define LVERSION L"luaSTGPlus-0.2"
#define LVERSION_LUA LUAJIT_VERSION

// 全局文件
#define LLOGFILE L"log.txt"
#define LLAUNCH_SCRIPT L"launch"
#define LCORE_SCRIPT L"core.lua"

// 全局回调函数名称
#define LFUNC_GAMEINIT "GameInit"
#define LFUNC_FRAME "FrameFunc"
#define LFUNC_RENDER "RenderFunc"
#define LFUNC_LOSEFOCUS "FocusLoseFunc"
#define LFUNC_GAINFOCUS "FocusGainFunc"

// 对象池信息
#define LGOBJ_MAXCNT 32768  // 最大对象数 //32768(full) //16384(half)
#define LGOBJ_MAXLASERNODE 512  // 曲线激光最大节点数
#define LGOBJ_DEFAULTGROUP 0  // 默认组
#define LGOBJ_GROUPCNT 16  // 碰撞组数

// CLASS中存放的回调函数的下标
#define LGOBJ_CC_INIT 1
#define LGOBJ_CC_DEL 2
#define LGOBJ_CC_FRAME 3
#define LGOBJ_CC_RENDER 4
#define LGOBJ_CC_COLLI 5
#define LGOBJ_CC_KILL 6

//功能控制
#define USER_SYSTEM_OPERATION //控制是否启用由lua层转来的额外功能，主要有重力加速度、速度限制
#define GLOBAL_SCALE_COLLI_SHAPE //全局图像缩放是否影响碰撞盒大小（傻吊逻辑谁设计的……mdzz，为什么图像缩放要影响碰撞盒大小）
#define USING_ADVANCE_COLLIDER //使用高级碰撞体方法

// 数学常量
#define DBL_HALF_MAX (DBL_MAX / 2.0) //双精度浮点的半大值
#define LRAD2DEGREE (180.0/3.141592653589793) // 弧度到角度
#define LDEGREE2RAD (1.0/LRAD2DEGREE) // 角度到弧度
#define LPI_HALF (3.141592653589793 / 2)  // PI*0.5

#define LNOEXCEPT noexcept // throw()
#define LNOINLINE __declspec(noinline)
#define LNOUSE(x) static_cast<void>(x)
#ifdef _DEBUG
#define LDEBUG
#endif
// 调试辅助
#if (defined LDEVVERSION) || (defined LDEBUG)
#define LSHOWRESLOADINFO  // 显示加载信息
#endif
// #define LSHOWFONTBASELINE  // 显示文字基线
#define LPERFORMANCEUPDATETIMER 0.25f  // 刷新一次计数器（间隔：秒）

#define LAPP (LuaSTGPlus::AppFrame::GetInstance())
#define LLOGGER (LuaSTGPlus::LogSystem::GetInstance())
#define LPOOL (LAPP.GetGameObjectPool())
#define LRES (LAPP.GetResourceMgr())
#define LFMGR (LAPP.GetFileManager())

#define LWIDE_(x) L ## x
#define LWIDE(x) LWIDE_(x)
#define LERROR(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Error, L##info, __VA_ARGS__)
#define LWARNING(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Warning, L##info, __VA_ARGS__)
#define LINFO(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Information, L##info, __VA_ARGS__)

#ifdef LDEBUG
#define LASSERT(cond) \
	if (!(cond)) \
	{ \
		LERROR("调试断言失败 于文件 '%s' 函数 '%s' 行 %d: %s", LWIDE(__FILE__), LWIDE(__FUNCTION__), __LINE__, L#cond); \
		_wassert(L#cond, LWIDE(__FILE__), __LINE__); \
	}
#else
#define LASSERT(cond)
#endif

#define LPARTICLE_MAXCNT 500  // 单个粒子池最多有500个粒子

#define LJOYSTICK1_MAPPING_START 0x92
#define LJOYSTICK1_MAPPING_END (0x92 + 31)
#define LJOYSTICK2_MAPPING_START 0xDF
#define LJOYSTICK2_MAPPING_END (0xDF + 31)
#define LJOYSTICK_X_MIN -0.65
#define LJOYSTICK_X_MAX 0.65
#define LJOYSTICK_Y_MIN -0.65
#define LJOYSTICK_Y_MAX 0.65

#define LSOUNDGLOBALFOCUS  true

namespace LuaSTGPlus
{
	class AppFrame;
}
