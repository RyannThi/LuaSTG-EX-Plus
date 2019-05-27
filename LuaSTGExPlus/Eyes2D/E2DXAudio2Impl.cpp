#include "E2DXAudio2Impl.hpp"
#include "E2DXAudio2VoicePool.hpp"

using namespace std;
using namespace Eyes2D;
using namespace Eyes2D::Sound;

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

XAudio2Impl::XAudio2Impl() {
	m_XAudio = nullptr;
	m_MasteringVoice = nullptr;

	HRESULT hr;
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::XAudio2Impl::XAudio2Impl", L"Failed to initialize COM.");
	hr = XAudio2Create(&m_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::XAudio2Impl::XAudio2Impl", L"Failed to create XAudio2.");
	hr = m_XAudio->CreateMasteringVoice(&m_MasteringVoice, 2, 44000);
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::XAudio2Impl::XAudio2Impl", L"Failed to create MasteringVoice.");
	m_VoicePool = new XAudio2VoicePool(m_XAudio, m_MasteringVoice);
}

XAudio2Impl::~XAudio2Impl() {
	delete m_VoicePool;
	m_VoicePool = nullptr;
	if (m_MasteringVoice)
		m_MasteringVoice->DestroyVoice();
	m_MasteringVoice = nullptr;
	if (m_XAudio)
		m_XAudio->Release();
	m_XAudio = nullptr;
}

void XAudio2Impl::SetMasterVolume(float v) {
	HRESULT hr = m_MasteringVoice->SetVolume(v);
	if(FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::XAudio2Impl::SetMasterVolume", L"Failed to set Master volume.");
}

float XAudio2Impl::GetMasterVolume() {
	float v;
	m_MasteringVoice->GetVolume(&v);
	return v;
}

void XAudio2Impl::SetMixerVolume(const string& name, float v) {
	IXAudio2SubmixVoice* mixer = m_VoicePool->GetMixVoice(name);
	if (mixer != nullptr) {
		HRESULT hr = mixer->SetVolume(v);
		if (FAILED(hr))
			throw E2DException(0, hr, L"Eyes2D::XAudio2Impl::SetMixerVolume", L"Failed to set Mixer volume.");
	}
	else {
		throw E2DException(0, 0, L"Eyes2D::XAudio2Impl::SetMixerVolume", L"Failed to find Mixer.");
	}
}

float XAudio2Impl::GetMixerVolume(const string& name) {
	IXAudio2SubmixVoice* mixer = m_VoicePool->GetMixVoice(name);
	if (mixer != nullptr) {
		float v;
		mixer->GetVolume(&v);
		return v;
	}
	else {
		throw E2DException(0, 0, L"Eyes2D::XAudio2Impl::SetMixerVolume", L"Failed to find Mixer.");
	}
}

IXAudio2SubmixVoice* XAudio2Impl::GetMixerVoiceByID(unsigned int id) {
	switch (id)
	{
	case 1:
		return m_VoicePool->GetMixVoice("SE");
		break;
	case 2:
		return m_VoicePool->GetMixVoice("BGM");
		break;
	default:
		return nullptr;
	}
}
