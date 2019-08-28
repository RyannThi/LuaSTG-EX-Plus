#pragma once
#include <objbase.h>

namespace LuaSTGPlus
{
	// com��������Զ���ʼ��com������ж��com����⣬�Ժ�������ʱ��ȡcom������ʼ���ɹ����
	class CoScope {
	private:
		bool m_Status = false;
	public:
		bool operator ()()const { return m_Status; }
		CoScope() { m_Status = SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)); }
		~CoScope() { if (m_Status) CoUninitialize(); }
	};
}
