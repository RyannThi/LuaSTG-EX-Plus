#include <string>
#include "E2DOggDecoder.hpp"
#include "vorbis/vorbisfile.h"
#include "fcyIO/fcyBinaryHelper.h"

using namespace std;
using namespace Eyes2D;

struct OggDecoder::OggFileCallbackFunc
{
	static size_t streamReadFunc(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		size_t tLen = (size_t)(((fcyStream*)datasource)->GetLength() - ((fcyStream*)datasource)->GetPosition());

		fLen tCountToRead = (tLen / size);
		if (tCountToRead > nmemb)
			tCountToRead = nmemb;

		((fcyStream*)datasource)->ReadBytes((fData)ptr, tCountToRead * size, NULL);

		return (size_t)tCountToRead;
	}
	static size_t streamSeekFunc(void* datasource, ogg_int64_t offset, int whence)
	{
		FCYSEEKORIGIN tOrigin = FCYSEEKORIGIN_BEG;
		switch (whence)
		{
		case 0:
			tOrigin = FCYSEEKORIGIN_BEG;
			break;
		case 1:
			tOrigin = FCYSEEKORIGIN_CUR;
			break;
		case 2:
			tOrigin = FCYSEEKORIGIN_END;
			break;
		}
		if (FCYFAILED(((fcyStream*)datasource)->SetPosition(tOrigin, offset)))
			return -1;
		else
			return 0;
	}
	static int streamCloseFunc(void* datasource)
	{
		return 0;
	}
	static long streamTellFunc(void* datasource)
	{
		return (long)((fcyStream*)datasource)->GetPosition();
	}
};

OggDecoder::OggDecoder(fcyStream* stream) {
	//m_Type = "???";
	m_FormatTag = 0;
	m_Channels = 0;
	m_SamplesPerSec = 0;
	m_AvgBytesPerSec = 0;
	m_BlockAlign = 0;
	m_BitsPerSample = 0;
	m_ExtendData = 0;
	m_DataBuffer = nullptr;
	m_DataSize = 0;

	if (stream == nullptr)
		throw E2DException(0, 0, L"Eyes2D::OggDecoder::OggDecoder", L"Invalid stream to read.");
	stream->AddRef();
	stream->Lock();
	stream->SetPosition(FCYSEEKORIGIN_BEG, 0);

	OggVorbis_File oggfile;
	ov_callbacks callback =
	{
		(size_t(*)(void*, size_t, size_t, void*)) OggDecoder::OggFileCallbackFunc::streamReadFunc,
		(int (*)(void*, ogg_int64_t, int))        OggDecoder::OggFileCallbackFunc::streamSeekFunc,
		(int (*)(void*))                          OggDecoder::OggFileCallbackFunc::streamCloseFunc,
		(long (*)(void*))                         OggDecoder::OggFileCallbackFunc::streamTellFunc
	};
	int ret = ov_open_callbacks(stream, &oggfile, NULL, 0, callback);

	stream->Unlock();
	if (ret != 0) {
		stream->Release();
		throw E2DException(0, 0, L"Eyes2D::OggDecoder::OggDecoder", L"Failed to open ogg file.");
	}

	//m_Type = "WAVE";//解码成PCM格式

	vorbis_info* ogginfo = ov_info(&oggfile, -1);

	m_FormatTag = 1;//固定不压缩
	m_Channels = ogginfo->channels;
	m_SamplesPerSec = ogginfo->rate;
	m_BlockAlign = m_Channels * 2;//固定16位深
	m_AvgBytesPerSec = m_SamplesPerSec * m_BlockAlign;//固定16位深
	m_BitsPerSample = 16;//固定16位深

	m_DataSize = (uint32_t)ov_pcm_total(&oggfile, -1) * m_BlockAlign;

	m_DataBuffer = new uint8_t[m_DataSize];
	stream->Lock();
	ov_time_seek(&oggfile, 0.0 / (double)m_AvgBytesPerSec);//寻址到开头
	stream->SetPosition(FCYSEEKORIGIN_BEG, stream->GetPosition());//重新定位文件流

	uint32_t SizeRead = 0;
	int sec;
	char* buffer = (char*)m_DataBuffer;
	while (SizeRead < m_DataSize)
	{
		long rlong = ov_read(&oggfile, buffer, m_DataSize - SizeRead, 0, 2, 1, &sec);//固定16位深，有符号整数
		if (rlong < 0)
		{
			// 错误
			ov_clear(&oggfile);
			stream->Unlock();
			stream->Release();
			m_DataSize = SizeRead;
			throw E2DException(0, 0, L"Eyes2D::OggDecoder::OggDecoder", L"Failed to read ogg data.");
		}
		if (rlong == 0) {
			break;// 到尾部
		}
		SizeRead = SizeRead + rlong;
		buffer = buffer + rlong;
	}
	m_DataSize = SizeRead;

	ov_clear(&oggfile);
	stream->Unlock();
	stream->Release();
}

OggDecoder::~OggDecoder() {
	if (m_DataBuffer != nullptr)
		delete[] m_DataBuffer;
}
