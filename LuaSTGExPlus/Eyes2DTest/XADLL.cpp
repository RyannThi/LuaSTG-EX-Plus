#include <iostream>
#include"fcyIO/fcyStream.h"
#include"E2DXAudio2Impl.hpp"
#include"E2DWaveDecoder.hpp"
#include"E2DOggDecoder.hpp"
#include"XFFT.h"

using namespace std;
using namespace Eyes2D;

HRESULT hr;

int main() {
	XAudio2Impl* sound = nullptr;
	try {
		sound = new XAudio2Impl();
		sound->SetMasterVolume(1.0f);
		sound->SetMixerVolume("BGM", 0.8f);
		cout << "Master:" << sound->GetMasterVolume() << "  BGM Mixer:" << sound->GetMixerVolume("BGM") << endl;
	}
	catch (E2DException& e) {
		cout << e.errResult << endl;
	}
	
	//WAV
	{
		const wchar_t path[] = L"se_pldead00.wav";
		fcyStream* stream = new fcyFileStream(path, false);
		WaveDecoder* decoder = new WaveDecoder(stream);

		cout << "channels:" << decoder->GetChannels() << endl;
		cout << "samplerate:" << decoder->GetSamplesPerSec() << endl;
		cout << "bitrate:" << decoder->GetAvgBytesPerSec() << endl;
		cout << "bits:" << decoder->GetBitsPerSample() << endl;
		cout << "blockalign:" << decoder->GetBlockAlign() << endl;
		cout << "datasize:" << decoder->GetDataSize() << endl;

		cout << "totolsamples:" << decoder->GetDataSize() / decoder->GetBlockAlign() << endl;

		{
			const int databuffer_size = 4096;
			const int wavevalue_size = 512;
			//FFT工作内存
			char* fftWorkset = (char*)malloc(xmath::fft::getNeededWorksetSize(wavevalue_size));
			float* fftOutComplex = (float*)malloc(wavevalue_size * sizeof(float) * 2);//512 * 2 = 1024
			//离散窗（函数）数列
			float fftWindow[wavevalue_size];
			xmath::fft::getWindow(wavevalue_size, fftWindow);
			//复制一段要进行FFT的数据
			uint32_t readsize = 0;
			uint8_t databuffer[databuffer_size] = { 0 };
			uint32_t pos = 44100 * decoder->GetBlockAlign();
			for (uint32_t offset = 0; offset < databuffer_size; offset++) {
				if ((pos + offset) < decoder->GetDataSize()) {
					databuffer[offset] = decoder->GetDataBuffer()[pos + offset];
					readsize++;
				}
				else {
					break;
				}
			}
			//解析成浮点数据
			uint32_t getsize = 0;
			const auto nBA = decoder->GetBlockAlign();
			const auto nCh = decoder->GetChannels();
			const auto factor = 1u << (nBA / nCh * 8 - 1);
			float wavevalue[wavevalue_size];
			if (decoder->GetChannels() == 1) {
				int64_t n;//中间变量
				for (uint32_t select = 0; select < wavevalue_size; select++) {
					if (select * decoder->GetBlockAlign() >= readsize) {
						break;
					}

					int64_t val = 0;
					for (size_t j = 0; j < nBA; ++j) {
						n = databuffer[j + select * nBA];
						val += n << (j * 8);
					}
					if (val > factor - 1) {
						n = factor;
						val = val - n * 2;
					}
					wavevalue[select] = double(val) / factor * fftWindow[select];

					getsize++;
				}
			}
			else if (decoder->GetChannels() == 2) {
				int64_t n;//中间变量
				for (uint32_t select = 0; select < wavevalue_size; select++) {
					if (select * decoder->GetBlockAlign() >= readsize) {
						break;
					}

					int64_t val = 0;
					for (size_t j = 0; j < nBA / nCh; ++j) {
						n = databuffer[j + select * nBA];
						val ^= n << (j * 8);
					}
					if (val > factor - 1) {
						n = factor;
						val = val - n * 2;
					}
					wavevalue[select] = double(val) / factor * fftWindow[select];

					getsize++;
				}
			}
			//FFT
			float outdata[256];
			xmath::fft::fft(wavevalue_size, fftWorkset, wavevalue, fftOutComplex, outdata);
			for (int i = 1; i < 256; i++) {
				//cout << outdata[i] << "    ";
			}
			//cout << endl;
			//简易处理
			float printdata[32] = { 0.0f };
			for (int i = 0; i < 32; i++) {
				for (int j = 0; j < 8; j++) {
					printdata[i] = printdata[i] + outdata[i * 8 + j];
				}
				printdata[i] = printdata[i] / 8.0f;
			}
			for (int i = 1; i < 32; i++) {
				cout << printdata[i] << endl;
			}
			//垃圾回收
			free(fftWorkset);
			free(fftOutComplex);
		}

		//创建音源
		IXAudio2SourceVoice* source;
		{
			//音源数据格式
			WAVEFORMATEX waveformat;
			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = decoder->GetChannels();
			waveformat.nSamplesPerSec = decoder->GetSamplesPerSec();
			waveformat.nAvgBytesPerSec = decoder->GetAvgBytesPerSec();
			waveformat.nBlockAlign = decoder->GetBlockAlign();
			waveformat.wBitsPerSample = decoder->GetBitsPerSample();
			waveformat.cbSize = 0;
			//创建
			hr = GetXAudio().GetXAudio()->CreateSourceVoice(&source, &waveformat);
			if (FAILED(hr)) {
				cout << "Create SourceVoice failed:" << hr << endl;
			}
			else {
				cout << "Create SourceVoice" << endl;
			}
		}

		//推送音频数据
		{

			XAUDIO2_BUFFER XAbuffer;
			ZeroMemory(&XAbuffer, sizeof(XAUDIO2_BUFFER));
			XAbuffer.Flags = XAUDIO2_END_OF_STREAM;
			XAbuffer.AudioBytes = decoder->GetDataSize();
			XAbuffer.pAudioData = decoder->GetDataBuffer();
			XAbuffer.PlayBegin = 0;
			XAbuffer.PlayLength = decoder->GetDataSize() / decoder->GetBlockAlign();//每个采样有2byte * 2channel = 4byte，所以总的采样数是data chunk size/4byte
			XAbuffer.LoopBegin = 0;
			XAbuffer.LoopLength = decoder->GetDataSize() / decoder->GetBlockAlign();//同上
			XAbuffer.LoopCount = XAUDIO2_LOOP_INFINITE;//无限循环

			hr = source->SubmitSourceBuffer(&XAbuffer);
			if (FAILED(hr)) {
				cout << "Submit SourceBuffer failed:" << hr << endl;
			}
			else {
				cout << "Submit SourceBuffer" << endl;
			}
		}

		system("pause");

		hr = source->Start();
		if (FAILED(hr)) {
			cout << "Start SourceVoice failed:" << hr << endl;
		}
		else {
			cout << "Start SourceVoice" << endl;
		}

		system("pause");

		source->Stop();
		source->DestroyVoice();
		delete decoder;
		stream->Release();
	}

	//OGG
	{
		const wchar_t path[] = L"luastg.ogg";
		fcyStream* stream = new fcyFileStream(path, false);
		OggDecoder* decoder = new OggDecoder(stream);

		//创建音源
		IXAudio2SourceVoice* source;
		{
			//音源数据格式
			WAVEFORMATEX waveformat;
			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nChannels = decoder->GetChannels();
			waveformat.nSamplesPerSec = decoder->GetSamplesPerSec();
			waveformat.nAvgBytesPerSec = decoder->GetAvgBytesPerSec();
			waveformat.nBlockAlign = decoder->GetBlockAlign();
			waveformat.wBitsPerSample = decoder->GetBitsPerSample();
			waveformat.cbSize = 0;
			//创建
			hr = GetXAudio().GetXAudio()->CreateSourceVoice(&source, &waveformat);
			if (FAILED(hr)) {
				cout << "Create SourceVoice failed:" << hr << endl;
			}
			else {
				cout << "Create SourceVoice" << endl;
			}
		}

		//推送音频数据
		{

			XAUDIO2_BUFFER XAbuffer;
			ZeroMemory(&XAbuffer, sizeof(XAUDIO2_BUFFER));
			XAbuffer.Flags = XAUDIO2_END_OF_STREAM;
			XAbuffer.AudioBytes = decoder->GetDataSize();
			XAbuffer.pAudioData = decoder->GetDataBuffer();
			XAbuffer.PlayBegin = 0;
			XAbuffer.PlayLength = decoder->GetDataSize() / decoder->GetBlockAlign();//每个采样有2byte * 2channel = 4byte，所以总的采样数是data chunk size/4byte
			XAbuffer.LoopBegin = 0;
			XAbuffer.LoopLength = decoder->GetDataSize() / decoder->GetBlockAlign();//同上
			XAbuffer.LoopCount = XAUDIO2_LOOP_INFINITE;//无限循环

			hr = source->SubmitSourceBuffer(&XAbuffer);
			if (FAILED(hr)) {
				cout << "Submit SourceBuffer failed:" << hr << endl;
			}
			else {
				cout << "Submit SourceBuffer" << endl;
			}
		}

		system("pause");

		hr = source->Start();
		if (FAILED(hr)) {
			cout << "Start SourceVoice failed:" << hr << endl;
		}
		else {
			cout << "Start SourceVoice" << endl;
		}

		system("pause");

		source->Stop();
		source->DestroyVoice();
		delete decoder;
		stream->Release();
	}

	system("pause");
	delete sound;
	return 0;
}
