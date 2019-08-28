#pragma once
#include <objbase.h>

namespace LuaSTGPlus
{
	// com组件库域，自动初始化com组件库和卸载com组件库，以函数调用时获取com组件库初始化成功与否
	class CoScope {
	private:
		bool m_Status = false;
	public:
		bool operator ()()const { return m_Status; }
		CoScope() { m_Status = SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)); }
		~CoScope() { if (m_Status) CoUninitialize(); }
	};
}
