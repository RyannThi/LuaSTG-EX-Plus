#include "AppFrame.h"
#include "RuntimeCheck.hpp"
#include "Utility.h"

using namespace std;
using namespace LuaSTGPlus;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
#ifdef LDEBUG
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(5351);
#endif
	CoScope co;
	if (co()) {
		// 检查运行库
		if (!CheckRuntime()) {
			MessageBoxW(
				0,
				L"检测到运行库缺失，请前往微软官网下载安装DirectX End-User Runtime。",
				L"引擎初始化失败",
				MB_ICONERROR | MB_OK);
			return -1;
		}

		// 初始化
		if (!LAPP.Init())
		{
			MessageBoxW(
				0,
				StringFormat(L"引擎未能成功初始化，查看日志文件'%s'可以获得更多信息。", LLOGFILE).c_str(),
				L"引擎初始化失败",
				MB_ICONERROR | MB_OK);
			return -1;
		}

		// 游戏循环
		LAPP.Run();

		// 销毁
		LAPP.Shutdown();
		return 0;
	}
	else {
		MessageBoxW(
			0,
			L"未能正常初始化COM组件库，请尝试重新启动此应用程序。",
			L"引擎初始化失败",
			MB_ICONERROR | MB_OK);
		return -1;
	}
}
