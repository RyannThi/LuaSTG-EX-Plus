#pragma once

#include <string>
#include "E2DGlobal.hpp"
#include "fcyIO/fcyStream.h"
#include "XAudio2.h"

namespace Eyes2D {
	class XAudio2VoicePool;
	
	class EYESDLLAPI XAudio2Impl {
	private:
		IXAudio2* m_XAudio;
		IXAudio2MasteringVoice* m_MasteringVoice;
		XAudio2VoicePool* m_VoicePool;
	public:
		XAudio2Impl();
		~XAudio2Impl();
	public:
		void SetMasterVolume(float v);
		float GetMasterVolume();
		void SetMixerVolume(std::string name, float v);
		float GetMixerVolume(std::string name);
	public:
		long LoadStaticSound(std::string bindmixer, fcyStream* stream); //从流加载静态缓冲区声音
		long LoadDynamicSound(std::string bindmixer, fcyStream* stream); //从流加载静态缓冲区声音
	};

	inline XAudio2Impl& GetXAudio() {
		static XAudio2Impl gs_XAudioInstance;
		return gs_XAudioInstance;
	}
}
