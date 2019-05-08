#include "E2DXAudio2VoicePool.hpp"

using namespace std;
using namespace Eyes2D;

XAudio2VoicePool::XAudio2VoicePool(IXAudio2* p, IXAudio2MasteringVoice* m) {
	m_XAudio = p;
	m_MasteringVoice = m;

	CreateMixVoice("SE");
	CreateMixVoice("BGM");
}

XAudio2VoicePool::~XAudio2VoicePool() {
	for (auto& i : m_MixVoice) {
		i.second->DestroyVoice();
	}
	m_MixVoice.clear();
	m_MasteringVoice = nullptr;
	m_XAudio = nullptr;
}

IXAudio2SubmixVoice* XAudio2VoicePool::CreateMixVoice(string name) {
	XAUDIO2_SEND_DESCRIPTOR sendunit;
	sendunit.Flags = 0;
	sendunit.pOutputVoice = m_MasteringVoice;
	XAUDIO2_VOICE_SENDS sendlist;
	sendlist.SendCount = 1;
	sendlist.pSends = &sendunit;
	
	IXAudio2SubmixVoice* mixer;
	HRESULT hr = m_XAudio->CreateSubmixVoice(&mixer, 2, 44100, 0, 0, &sendlist, 0);
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::XAudio2VoicePool::CreateMixVoice", L"Failed to create SubmixVoice.");

	m_MixVoice.insert({ name, mixer });
	return mixer;
}

IXAudio2SubmixVoice* XAudio2VoicePool::GetMixVoice(string name) {
	auto search = m_MixVoice.find(name);
	if (search != m_MixVoice.end())
		return search->second;
	else
		return nullptr;
}
