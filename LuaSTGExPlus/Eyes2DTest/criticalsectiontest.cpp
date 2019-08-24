#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <set>
#include <sstream>

#include "E2DMainFunction.hpp"
#define E2DMAINRETURN std::system("pause"); return 0;
#define SAY(W) std::cout<<(W)<<std::endl;

#include "fcyIO/fcyStream.h"
#include "E2DFileManager.hpp"
#include "E2DCodePage.hpp"
#include "E2DWaveDecoder.hpp"
#include "E2DOggDecoder.hpp"
#include "E2DLogSystem.hpp"

#include <Windows.h>
#include <wrl.h>
#include "XAudio2.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using namespace std;

void test1() {
	string fname = "LuaSTG-x TPv6.0.zip";
	system("pause");

	for (int t = 0; t < 10; t++) {
		{
			fstream f;
			vector<uint8_t> databuffer;

			cout << "fstream" << endl;

			auto start = std::chrono::high_resolution_clock::now();

			f.open(fname, ios::in | ios::binary);
			if (!f.is_open()) {
				cout << "Failed to open file." << endl;
			}
			else {
				f.seekg(0, ios::beg);
				size_t pos1 = f.tellg();
				f.seekg(0, ios::end);
				size_t pos2 = f.tellg();
				size_t buffersize = pos2 - pos1;
				cout << "File " << buffersize << " bytes" << endl;

				databuffer.resize((size_t)buffersize, 0);

				f.seekg(0, ios::beg);
				f.read((char*)databuffer.data(), buffersize);

				pos2 = f.tellg();
				buffersize = pos2 - pos1;
				cout << "Read " << buffersize << " bytes" << endl;

				f.close();
			}

			auto end = std::chrono::high_resolution_clock::now();
			chrono::duration<double, ratio<1, 1>> diff = end - start;//频率为每秒1次
			cout << diff.count() << "s" << endl;

			databuffer.clear();
		}

		{
			HANDLE f;
			vector<uint8_t> databuffer;

			cout << "WIN" << endl;

			auto start = std::chrono::high_resolution_clock::now();

			f = ::CreateFileA(
				fname.data(),
				GENERIC_READ,			//读取权限
				FILE_SHARE_READ,		//共享读取
				NULL,					//安全属性结构
				OPEN_EXISTING,			//打开存在的文件
				FILE_ATTRIBUTE_NORMAL,	//普通属性
				NULL					//不使用模板文件
			);
			if (f == INVALID_HANDLE_VALUE) {
				cout << "Failed to open file." << endl;
			}
			else {
				DWORD pos1 = SetFilePointer(f, 0, NULL, FILE_BEGIN);
				DWORD pos2 = SetFilePointer(f, 0, NULL, FILE_END);
				DWORD buffersize = pos2 - pos1;
				cout << "File " << buffersize << " bytes" << endl;

				databuffer.resize((size_t)buffersize, 0);

				DWORD read;
				SetFilePointer(f, 0, NULL, FILE_BEGIN);
				BOOL ret = ReadFile(f, (LPVOID)databuffer.data(), buffersize, &read, NULL);
				if (ret == TRUE) {
					cout << "Read " << read << " bytes" << endl;
				}

				CloseHandle(f);
			}

			auto end = std::chrono::high_resolution_clock::now();
			chrono::duration<double, ratio<1, 1>> diff = end - start;//频率为每秒1次
			cout << diff.count() << "s" << endl;

			databuffer.clear();
		}

		Sleep(1000);
	}

}

void test2() {
	Eyes2D::IO::Archive zip("LuaSTG-x TPv6.0.zip");
	fcyStream* stream = zip.LoadFile("LuaSTG-x TPv6.0/doc/LuaSTG-x_TP_v6.0_readme.txt");
	if (stream == nullptr) {
		SAY("加载失败");
	}
	else {
		stream->SetPosition(FCYSEEKORIGIN_BEG, 0);
		string txt; txt.resize(stream->GetLength(), 0);

		fcyMemStream* mstream = (fcyMemStream*)stream;
		txt.insert(0, (char*)mstream->GetInternalBuffer(), mstream->GetLength());
		SAY(Eyes2D::String::UTF8ToANSI(txt));

		stream->Release();
	}

	stream = zip.LoadEncryptedFile("LuaSTG-x TPv6.0/doc/LuaSTG-x_TP_v6.0_readme2.txt", "0000");
	if (stream == nullptr) {
		SAY("加载失败");
	}
	else {
		stream->SetPosition(FCYSEEKORIGIN_BEG, 0);
		string txt; txt.resize(stream->GetLength(), 0);

		fcyMemStream* mstream = (fcyMemStream*)stream;
		txt.insert(0, (char*)mstream->GetInternalBuffer(), mstream->GetLength());
		SAY(Eyes2D::String::UTF8ToANSI(txt));

		stream->Release();
	}

	//zip.ListFile();
	//Eyes2D::EYESINFO("NMSL");
	Eyes2D::E2DException e(0, 0, L"Source", L"Description");
	Eyes2D::EYESDEBUG(e);
}

void test3() {
	Eyes2D::IO::Archive zip("test.zip");
	zip.ListFile();
}

void test4() {
	struct Archive {
		int priority;
		int uid;
		Archive(int p, int i) {
			priority = p;
			uid = i;
		}
	};
	struct ArchiveSort {
		bool operator()(const Archive& l, const Archive& r) const {
			if (l.priority != r.priority) {
				//优先级大的排在前面
				return l.priority > r.priority;
			}
			else {
				if (l.uid != r.uid) {
					//优先级相同uid大的排在前面
					return l.uid > r.uid;
				}
				else {
					//关我P事
				}
			}
		}
	};
	set< Archive, ArchiveSort> dataset;
	dataset.insert(Archive(0, 0));
	dataset.insert(Archive(0, 1));
	dataset.insert(Archive(1, 0));
	dataset.insert(Archive(2, 1));
	dataset.insert(Archive(-9961, 2));
	dataset.insert(Archive(-3, 3));
	dataset.insert(Archive(0, 2));
	dataset.insert(Archive(0, 3));
	for (auto i : dataset) {
		ostringstream oss;
		oss << i.priority << "    " << i.uid << endl;
		Eyes2D::EYESDEBUG(oss.str().c_str());
	}
}

string ss = "test";

const char* getstring() {
	return ss.c_str();
}

void test5() {
	cout << getstring() << endl;
}

void test6() {
	struct Archive {
		int priority;
		int uid;
		Archive(int p, int i) {
			priority = p;
			uid = i;
		}
	};
	struct ArchiveSort {
		bool operator()(const Archive* lp, const Archive* rp) const {
			const Archive& l = *lp;
			const Archive& r = *rp;
			if (l.priority != r.priority) {
				//优先级大的排在前面
				return l.priority > r.priority;
			}
			else {
				//优先级相同uid大的排在前面
				return l.uid > r.uid;
			}
		}
	};
	set< Archive*, ArchiveSort> dataset;
	Archive* p = new Archive(0, 0);
	dataset.insert(p);
	for (auto it = dataset.begin(); it != dataset.end();) {
		it = dataset.erase(it);
	}
	//SAY(p->priority);
}

int test7a(Eyes2D::Sound::AudioDecoder* decoder) {
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
		waveformat.nChannels = decoder->GetChannels();
		waveformat.nSamplesPerSec = decoder->GetSamplesPerSec();
		waveformat.nAvgBytesPerSec = decoder->GetAvgBytesPerSec();
		waveformat.nBlockAlign = decoder->GetBlockAlign();
		waveformat.wBitsPerSample = decoder->GetBitsPerSample();
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
			return -1;
		}
		else {
			cout << "Submit SourceBuffer" << endl;
		}
	}

	//播放
	{
		hr = source->Start();
		if (FAILED(hr)) {
			cout << "Start SourceVoice failed:" << hr << endl;
			return -1;
		}
		else {
			cout << "Start SourceVoice" << endl;
		}
		
		for (int t = 0; t < 60; t++) {
			Sleep(17);
			
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
					return -1;
				}
				else {
					cout << "Submit SourceBuffer" << endl;
				}
			}

			hr = source->Start();
			if (FAILED(hr)) {
				cout << "Start SourceVoice failed:" << hr << endl;
				return -1;
			}
			else {
				cout << "Start SourceVoice" << endl;
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
		delete decoder;
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

void test7() {
	Eyes2D::IO::FileManager fm;
	fm.LoadArchive("test.zip");
	Eyes2D::IO::Archive* zip = fm.GetArchive("test.zip");
	zip->ListFile();
	if (zip != nullptr) {
		SAY("get");
		//fcyStream* stream = zip->LoadFile("se_pldead00.wav");
		fcyStream* stream = zip->LoadFile("menu.ogg");
		if (stream != nullptr) {
			SAY("load");
			try {
				//Eyes2D::Sound::Decoder* decoder = new Eyes2D::Sound::WaveDecoder(stream);
				Eyes2D::Sound::AudioDecoder* decoder = new Eyes2D::Sound::OggDecoder(stream);
				SAY(decoder->GetDataSize());
				test7a(decoder);
				system("pause");
			}
			catch (Eyes2D::E2DException& e) {
				Eyes2D::EYESERROR(e);
			}
		}
		stream->Release();
	}
}

void test8() {
	string str = "0123456789";
	SAY(str.length());
	SAY(str.size());
	for (int index = 0; index < str.length(); index++) {
		SAY(str.c_str()[index]);
	}
	for (int index = (str.length() - 1); index >= 0; index--) {
		str.data()[index] = '0' + index;
	}
	SAY(str);
	string s = "\\root\\mydir\\test.txt";
	SAY(s);
	for (int index = 0; index < s.length(); index++) {
		if (s.data()[index] == '\\') {
			s.data()[index] = '/';
		}
	}
	SAY(s);
	for (auto& c : s) {
		if (c == '/')
			c = '?';
	}
	SAY(s);
}

E2DMAIN{
	test8();
	E2DMAINRETURN;
}
