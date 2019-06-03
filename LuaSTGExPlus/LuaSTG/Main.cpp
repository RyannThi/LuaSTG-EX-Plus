#include "AppFrame.h"
#include "Utility.h"
#include "E2DMainFunction.hpp"

using namespace std;
using namespace LuaSTGPlus;

E2DWINMAIN {
#ifdef LDEBUG
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(5351);
#endif
	
	// 检查D3D9是否存在
	if (LoadLibraryW(L"D3DX9_43.DLL") == NULL) {
		MessageBox(
			0,
			L"未能加载D3DX9_43.DLL，这可能是由于没有安装DirectX运行时导致的。\n\n请前往微软官网下载安装DirectX 9 Runtime。",
			L"LuaSTGPlus初始化失败",
			MB_ICONERROR | MB_OK);
		return -1;
	}

	// 初始化
	if (!LAPP.Init())
	{
		MessageBox(
			0,
			StringFormat(L"很抱歉，由于某些原因框架未能成功初始化。\n若该问题多次出现请联系作者。\n\n查看日志文件'%s'可以获得更多信息。", LLOGFILE).c_str(),
			L"LuaSTGPlus初始化失败",
			MB_ICONERROR | MB_OK);
		return -1;
	}

	// 游戏循环
	LAPP.Run();

	// 销毁
	LAPP.Shutdown();
	return 0;
}
