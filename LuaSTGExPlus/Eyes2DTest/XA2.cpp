#include <iostream>

#include <Windows.h>
#include <wrl.h>
#include "XAudio2.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using namespace std;

int main() {
	HRESULT hr;
	
	//XAudio2引擎
	ComPtr<IXAudio2> XAudio;
	{
		//准备COM
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr)) {
			cout << "Install COM failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Install COM" << endl;
		}
		//创建
		hr = XAudio2Create(XAudio.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
		if (FAILED(hr)) {
			cout << "Install XAudio failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Install XAudio" << endl;
		}
	}

	//创建master音轨
	IXAudio2MasteringVoice* master;
	{
		hr = XAudio->CreateMasteringVoice(&master);
		if (FAILED(hr)) {
			cout << "Create MasteringVoice failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Create MasteringVoice" << endl;
		}
	}

	master->SetVolume(0.5);

	//创建SE混音音轨
	IXAudio2SubmixVoice* mixer;
	{
		//FA送到master音轨
		XAUDIO2_SEND_DESCRIPTOR sendunit;
		sendunit.Flags = 0;
		sendunit.pOutputVoice = master;
		XAUDIO2_VOICE_SENDS sendlist;
		sendlist.SendCount = 1;
		sendlist.pSends = &sendunit;
		hr = XAudio->CreateSubmixVoice(&mixer, 2, 44100, 0, 0, &sendlist, 0);
		if (FAILED(hr)) {
			cout << "Create SubmixVoice failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Create SubmixVoice" << endl;
		}
	}

	mixer->SetVolume(0.5);

	//通用推流目标，推流到混音音轨
	XAUDIO2_VOICE_SENDS sendlist;
	XAUDIO2_SEND_DESCRIPTOR sendunit;
	sendunit.Flags = 0;
	sendunit.pOutputVoice = mixer;
	sendlist.SendCount = 1;
	sendlist.pSends = &sendunit;

	//创建音源
	IXAudio2SourceVoice* source;
	{
		//音源数据格式
		WAVEFORMATEX waveformat;
		waveformat.wFormatTag = WAVE_FORMAT_PCM;
		waveformat.nChannels = 2;
		waveformat.nSamplesPerSec = 44100;
		waveformat.nAvgBytesPerSec = 176400;
		waveformat.nBlockAlign = 4;
		waveformat.wBitsPerSample = 16;
		waveformat.cbSize = 0;
		//创建
		hr = XAudio->CreateSourceVoice(&source, &waveformat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, 0, &sendlist, 0);
		if (FAILED(hr)) {
			cout << "Create SourceVoice failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Create SourceVoice" << endl;
		}
	}

	//读取音频数据
	const long data_bytes = 243168;
	uint8_t* buffer = new uint8_t[data_bytes];
	{
		HANDLE file = CreateFile(
			L"se_pldead00.wav",
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (INVALID_HANDLE_VALUE == file) {
			cout << "Open file failed:" << HRESULT_FROM_WIN32(GetLastError()) << endl;
			delete[] buffer;
			return -1;
		}
		else {
			cout << "Open file" << endl;
		}

		if (INVALID_SET_FILE_POINTER == SetFilePointer(file, 44, NULL, FILE_BEGIN)) {
			cout << "Set file Pointer failed:" << HRESULT_FROM_WIN32(GetLastError()) << endl;
			delete[] buffer;
			return -1;
		}
		else {
			cout << "Set file Pointer" << endl;
		}

		DWORD dwRead;
		if (0 == ReadFile(file, buffer, data_bytes, &dwRead, NULL)) {
			cout << "Read file failed:" << HRESULT_FROM_WIN32(GetLastError()) << endl;
			delete[] buffer;
			return -1;
		}
		else {
			cout << "Read file:" << dwRead << endl;
		}

		if (FALSE == CloseHandle(file)) {
			cout << "Close file failed" << endl;
			delete[] buffer;
			return -1;
		}
		else {
			cout << "Close file" << endl;
		}
	}

	//推送音频数据
	{
		
		XAUDIO2_BUFFER XAbuffer;
		ZeroMemory(&XAbuffer, sizeof(XAUDIO2_BUFFER));
		XAbuffer.Flags = XAUDIO2_END_OF_STREAM;
		XAbuffer.AudioBytes = data_bytes;
		XAbuffer.pAudioData = buffer;
		XAbuffer.PlayBegin = 0;
		XAbuffer.PlayLength = data_bytes / 4;//每个采样有2byte * 2channel = 4byte，所以总的采样数是data chunk size/4byte
		XAbuffer.LoopBegin = 0;
		XAbuffer.LoopLength = data_bytes / 4;//同上
		XAbuffer.LoopCount = XAUDIO2_LOOP_INFINITE;//无限循环

		hr = source->SubmitSourceBuffer(&XAbuffer);
		if (FAILED(hr)) {
			cout << "Submit SourceBuffer failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Submit SourceBuffer" << endl;
		}
	}

	//播放
	{
		for (int t = 0; t < 10; t++) {
			hr = source->Start();
			if (FAILED(hr)) {
				cout << "Start SourceVoice failed:" << hr << endl;
				return -1;
			}
			else {
				cout << "Start SourceVoice" << endl;
			}

			Sleep(100);
			hr = source->Stop();
			if (FAILED(hr)) {
				cout << "Stop SourceVoice failed:" << hr << endl;
				return -1;
			}
			else {
				cout << "Stop SourceVoice" << endl;
			}

			hr = source->FlushSourceBuffers();
			if (FAILED(hr)) {
				cout << "Flush source buffers failed:" << hr << endl;
				return -1;
			}
			else {
				cout << "Flush source buffers" << endl;
			}

			//推送音频数据
			{

				XAUDIO2_BUFFER XAbuffer;
				ZeroMemory(&XAbuffer, sizeof(XAUDIO2_BUFFER));
				XAbuffer.Flags = XAUDIO2_END_OF_STREAM;
				XAbuffer.AudioBytes = data_bytes;
				XAbuffer.pAudioData = buffer;
				XAbuffer.PlayBegin = 0;
				XAbuffer.PlayLength = data_bytes / 4;//每个采样有2byte * 2channel = 4byte，所以总的采样数是data chunk size/4byte
				XAbuffer.LoopBegin = 0;
				XAbuffer.LoopLength = data_bytes / 4;//同上
				XAbuffer.LoopCount = XAUDIO2_LOOP_INFINITE;//无限循环

				hr = source->SubmitSourceBuffer(&XAbuffer);
				if (FAILED(hr)) {
					cout << "Submit SourceBuffer failed:" << hr << endl;
					return -1;
				}
				else {
					cout << "Submit SourceBuffer" << endl;
				}
			}
		}

		while (!GetAsyncKeyState(VK_ESCAPE)) {
			Sleep(1);
		}
	}

	//停止
	{
		hr = source->Stop();
		if (FAILED(hr)) {
			cout << "Stop SourceVoice failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Stop SourceVoice" << endl;
		}
	}

	//清理垃圾
	{
		source->SetOutputVoices(NULL);
		source->DestroyVoice();
		delete[] buffer;
		cout << "Destroy SourceVoice and clear SoundData" << endl;

		mixer->SetOutputVoices(NULL);
		mixer->DestroyVoice();
		cout << "Destroy SubmixVoice" << endl;

		master->DestroyVoice();
		cout << "Destroy MaterVoice" << endl;
	}
	
	system("pause");
	return 0;
}
