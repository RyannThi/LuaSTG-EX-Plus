#pragma once
#include <Windows.h>

namespace LuaSTGPlus {
	inline bool CheckRuntime() {
		if (LoadLibraryW(L"D3D9.dll") == nullptr) {
			return false;
		}
		if (LoadLibraryW(L"D3DX9_43.DLL") == nullptr) {
			return false;
		}
		if (LoadLibraryW(L"XINPUT1_3.DLL") == nullptr) {
			return false;
		}
		if (LoadLibraryW(L"DInput8.dll") == nullptr) {
			return false;
		}
		if (LoadLibraryW(L"DSound.dll") == nullptr) {
			return false;
		}
		return true;
	}
}
