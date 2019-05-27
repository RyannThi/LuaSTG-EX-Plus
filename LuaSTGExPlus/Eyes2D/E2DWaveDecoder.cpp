#include <string>
#include "E2DWaveDecoder.hpp"
#include "fcyIO/fcyBinaryHelper.h"

using namespace std;
using namespace Eyes2D;
using namespace Eyes2D::Sound;

WaveDecoder::WaveDecoder(fcyStream* stream) {
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
		throw E2DException(0, 0, L"Eyes2D::WaveDecoder::WaveDecoder", L"Invalid stream to read.");
	stream->AddRef();
	stream->Lock();
	stream->SetPosition(FCYSEEKORIGIN_BEG, 0);

	string TypeTag;
	try
	{
		fcyBinaryReader Reader(stream);
		char ID[5];
		uint32_t Size;
		while (stream->GetPosition() < stream->GetLength())
		{
			memset(ID, 0, 5 * sizeof(char));
			Size = 0;
			Reader.ReadChars(ID, 4);
			Size = Reader.ReadUInt32();

			string sID = ID;
			if (sID == "RIFF") {
				char format[5];
				memset(format, 0, 5 * sizeof(char));
				Reader.ReadChars(format, 4);
				//m_Type = format;
				TypeTag = format;
			}
			else if (sID == "fmt ") {
				m_FormatTag = Reader.ReadUInt16();
				m_Channels = Reader.ReadUInt16();
				m_SamplesPerSec = Reader.ReadUInt32();
				m_AvgBytesPerSec = Reader.ReadUInt32();
				m_BlockAlign = Reader.ReadUInt16();
				m_BitsPerSample = Reader.ReadUInt16();
				if (Size > 18) {
					m_ExtendData = Reader.ReadUInt16();
					stream->SetPosition(FCYSEEKORIGIN_CUR, Size - 18);
				}
				else if (Size > 16 && Size <= 18) {
					m_ExtendData = Reader.ReadUInt16();
				}
				else if (Size <= 16) {
					m_ExtendData = 0;
				}
			}
			else if (sID == "data") {
				if (m_DataBuffer == nullptr) {
					m_DataBuffer = new uint8_t[Size];
					uint64_t Read = 0;
					stream->ReadBytes(m_DataBuffer, Size, &Read);
					m_DataSize = (uint32_t)Read;//潜在的数据溢出
				}
			}
			else {
				stream->SetPosition(FCYSEEKORIGIN_CUR, Size);//跳过不支持的chunk
			}
		}
	}
	catch (const fcyException& e) // 处理读取异常
	{
		//清理
		long t = (long)e.GetTime();
		stream->Unlock();
		stream->Release();
		throw E2DException(t, 0, L"Eyes2D::WaveDecoder::WaveDecoder", L"Failed to read wav file.");
	}

	//清理
	stream->Unlock();
	stream->Release();
	if (TypeTag != "WAVE") {
		throw E2DException(0, 0, L"Eyes2D::WaveDecoder::WaveDecoder", L"Invalid wav file.");
	}
}

WaveDecoder::~WaveDecoder() {
	if (m_DataBuffer != nullptr)
		delete[] m_DataBuffer;
}
