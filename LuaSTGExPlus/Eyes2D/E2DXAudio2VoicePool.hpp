#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "E2DXAudio2Impl.hpp"

namespace Eyes2D {
	class XAudio2VoicePool {
	private:
		IXAudio2* m_XAudio;                                              //ֻ�ǳ���
		IXAudio2MasteringVoice* m_MasteringVoice;                        //ֻ�ǳ���
		std::unordered_map<std::string, IXAudio2SubmixVoice*> m_MixVoice;//��������

	public:
		XAudio2VoicePool(IXAudio2* p, IXAudio2MasteringVoice* m);
		~XAudio2VoicePool();
	public:
		IXAudio2SubmixVoice* CreateMixVoice(std::string name);
		IXAudio2SubmixVoice* GetMixVoice(std::string name);
	};
}
