#pragma once

#include <string>
#include "fcyIO/fcyStream.h"
#include "E2DGlobal.hpp"
#include "XAudio2.h"

namespace Eyes2D {
	class XAudio2VoicePool;
	
	class EYESDLLAPI XAudio2Engine {
	private:
		IXAudio2* m_XAudio;
		IXAudio2MasteringVoice* m_MasteringVoice;
		XAudio2VoicePool* m_VoicePool;
	public:
		XAudio2Engine();
		~XAudio2Engine();
	public:
		void SetMasterVolume(float v);
		float GetMasterVolume();
		void SetMixerVolume(std::string name, float v);
		float GetMixerVolume(std::string name);
	public:
		long LoadStaticSound(std::string bindmixer, fcyStream* stream); //从流加载静态缓冲区声音
		long LoadDynamicSound(std::string bindmixer, fcyStream* stream); //从流加载静态缓冲区声音
	};
}
