#include "AppFrame.h"
#include "RuntimeCheck.hpp"
#include "Utility.h"

#ifdef USING_STEAM_API
#pragma comment(lib, "steam_api.lib")
#include "steam_api.h"
extern "C" void SteamAPIDebugTextHook(int level, const char* msg) {
	OutputDebugStringA(msg);
	if (level >= 1) { int x = 3; (void)x; }
}
#endif // USING_STEAM_API

using namespace std;
using namespace LuaSTGPlus;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
#ifdef LDEBUG
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(5351);
#endif

#ifdef USING_STEAM_API
	if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
		return 0;
	}
	if (!SteamAPI_Init()) {
		OutputDebugStringA("Failed to initialize steam API.");
		return 1;
	}
	SteamClient()->SetWarningMessageHook(&SteamAPIDebugTextHook);
#endif // USING_STEAM_API
	
	CoScope co;
	if (co()) {
		// 检查运行库
		if (!CheckRuntime()) {
			MessageBoxW(
				0,
				L"检测到运行库缺失，请前往微软官网下载安装 DirectX End-User Runtime。",
				L"引擎初始化失败",
				MB_ICONERROR | MB_OK);
			return -1;
		}

		// 初始化
		if (!LAPP.Init())
		{
			MessageBoxW(
				0,
				L"引擎未能成功初始化，查看日志文件可以获得更多信息。",
				L"引擎初始化失败",
				MB_ICONERROR | MB_OK);
			return -1;
		}

		// 游戏循环
		LAPP.Run();

		// 销毁
		LAPP.Shutdown();
	}
	else {
		MessageBoxW(
			0,
			L"未能正常初始化COM组件库，请尝试重新启动此应用程序。",
			L"引擎初始化失败",
			MB_ICONERROR | MB_OK);
		return -1;
	}

#ifdef USING_STEAM_API
	SteamAPI_Shutdown();
#endif // USING_STEAM_API

	return 0;
}
