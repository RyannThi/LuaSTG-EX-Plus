/// @file Global.h
/// @brief ȫ�ֶ����ļ�
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

// ��־ϵͳ
#include "LogSystem.h"

// һЩȫ�ַ�Χ�ĺ�
#define LVERSION L"luaSTGPlus-0.2"
#define LVERSION_LUA LUAJIT_VERSION

// ȫ���ļ�
#define LLOGFILE L"log.txt"
#define LLAUNCH_SCRIPT L"launch"
#define LCORE_SCRIPT L"core.lua"

// ȫ�ֻص���������
#define LFUNC_GAMEINIT "GameInit"
#define LFUNC_FRAME "FrameFunc"
#define LFUNC_RENDER "RenderFunc"
#define LFUNC_LOSEFOCUS "FocusLoseFunc"
#define LFUNC_GAINFOCUS "FocusGainFunc"

// �������Ϣ
#define LGOBJ_MAXCNT 16384  // �������� //32768(old)
#define LGOBJ_MAXLASERNODE 512  // ���߼������ڵ���
#define LGOBJ_DEFAULTGROUP 0  // Ĭ����
#define LGOBJ_GROUPCNT 16  // ��ײ����

// CLASS�д�ŵĻص��������±�
#define LGOBJ_CC_INIT 1
#define LGOBJ_CC_DEL 2
#define LGOBJ_CC_FRAME 3
#define LGOBJ_CC_RENDER 4
#define LGOBJ_CC_COLLI 5
#define LGOBJ_CC_KILL 6

//���ܿ���
#define USER_SYSTEM_OPERATION //�����Ƿ�������lua��ת���Ķ��⹦��

// ��ѧ����
#define DBL_HALF_MAX (DBL_MAX / 2.0) //˫���ȸ���İ��ֵ
#define LRAD2DEGREE (180.0/3.141592653589793) // ���ȵ��Ƕ�
#define LDEGREE2RAD (1.0/LRAD2DEGREE) // �Ƕȵ�����
#define LPI_HALF (3.141592653589793 / 2)  // PI*0.5

#define LNOEXCEPT throw()
#define LNOINLINE __declspec(noinline)
#define LNOUSE(x) static_cast<void>(x)
#ifdef _DEBUG
#define LDEBUG
#endif
// ���Ը���
#if (defined LDEVVERSION) || (defined LDEBUG)
#define LSHOWRESLOADINFO  // ��ʾ������Ϣ
#endif
// #define LSHOWFONTBASELINE  // ��ʾ���ֻ���
#define LPERFORMANCEUPDATETIMER 0.25f  // ˢ��һ�μ�������������룩

#define LAPP (LuaSTGPlus::AppFrame::GetInstance())
#define LLOGGER (LuaSTGPlus::LogSystem::GetInstance())
#define LPOOL (LAPP.GetGameObjectPool())
#define LRES (LAPP.GetResourceMgr())

#define LWIDE_(x) L ## x
#define LWIDE(x) LWIDE_(x)
#define LERROR(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Error, L##info, __VA_ARGS__)
#define LWARNING(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Warning, L##info, __VA_ARGS__)
#define LINFO(info, ...) LLOGGER.Log(LuaSTGPlus::LogType::Information, L##info, __VA_ARGS__)

#ifdef LDEBUG
#define LASSERT(cond) \
	if (!(cond)) \
	{ \
		LERROR("���Զ���ʧ�� ���ļ� '%s' ���� '%s' �� %d: %s", LWIDE(__FILE__), LWIDE(__FUNCTION__), __LINE__, L#cond); \
		_wassert(L#cond, LWIDE(__FILE__), __LINE__); \
	}
#else
#define LASSERT(cond)
#endif

#define LPARTICLE_MAXCNT 500  // �������ӳ������500������

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