#pragma once

#include "E2DGlobal.hpp"
#include "E2DSound.hpp"

#include "fcyIO/fcyStream.h"
#include "XAudio2.h"

namespace Eyes2D {
	namespace Sound {
		class XAudio2SoundPlayerImpl : public SoundPlayer {
		private:
			char* m_FFTWorkset;
			float* m_FFTOutComplex;
		private:
			SoundStatus m_Status;
			IXAudio2SourceVoice* m_Voice;
			Decoder* m_Decoder;
			unsigned int m_PlayPos;   //播放位置
			unsigned int m_LoopStart; //循环节开始
			unsigned int m_LoopEnd;   //循环节结束
			unsigned int m_LoopCount; //循环次数
		public:
			XAudio2SoundPlayerImpl(fcyStream* stream, unsigned int mixerid);
			~XAudio2SoundPlayerImpl();
		private:
			bool pushSoundBuffer();
		public:
			void Play();
			void Pause();
			void Stop();
			SoundStatus GetStatus();

			void SetLoop(unsigned int start, unsigned int end, unsigned int loopcount);
			void GetLoop(unsigned int& outstart, unsigned int& outend, unsigned int& outloopcount);

			bool SetTime(unsigned int sample);
			unsigned int GetTime();
			unsigned long long GetPlayerdTime();
			unsigned int GetTotalTime();

			void SetVolume(float volume);
			float GetVolume();

			void SetBalance(float balance);
			float GetBalance();

			void SetSpeed(float speed);
			float GetSpeed();

			bool GetFFT(float*& outdata, unsigned int& outsize, unsigned int channel = 1);
		};
	}
}
