#pragma once

#include <string>

#include "E2DGlobal.hpp"
#include "E2DSound.hpp"

#include "fcyIO/fcyStream.h"
#include "XAudio2.h"

namespace Eyes2D {
	namespace Sound {
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
			IXAudio2* GetXAudio() { return m_XAudio; }
			IXAudio2* GetXAudio2() { return m_XAudio; }
			IXAudio2SubmixVoice* GetMixerVoiceByID(unsigned int id);
			void SetMasterVolume(float v);
			float GetMasterVolume();
			void SetMixerVolume(const std::string& name, float v);
			float GetMixerVolume(const std::string& name);
		public:
			void CreateSoundPlayer(fcyStream* stream, SoundPlayer*& soundplayer);
		};

		inline XAudio2Impl& GetXAudio() {
			static XAudio2Impl gs_XAudioInstance;
			return gs_XAudioInstance;
		}

		inline XAudio2Impl& GetInstance() {
			return GetXAudio();
		}
	}
}
