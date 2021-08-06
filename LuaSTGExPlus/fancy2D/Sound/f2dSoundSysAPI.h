////////////////////////////////////////////////////////////////////////////////
/// @file  f2dSoundSysAPI.h
/// @brief fancy2Dﾒｵﾏｵﾍｳ API
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "fcyRefObj.h"
#include "fcyIO\fcyStream.h"

#include <dsound.h>

////////////////////////////////////////////////////////////////////////////////
/// @brief ｶｯﾌｬｼﾓﾔﾘdSound
////////////////////////////////////////////////////////////////////////////////
class f2dSoundSysAPI
{
private:
	typedef HRESULT (WINAPI *pDLLEntry_DirectSoundCreate8)(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter);
private:
	HMODULE m_hModule;

	pDLLEntry_DirectSoundCreate8 m_Entry_pDirectSoundCreate8;
public:
	HRESULT DLLEntry_DirectSoundCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter);
public:
	f2dSoundSysAPI();
	~f2dSoundSysAPI();
};
