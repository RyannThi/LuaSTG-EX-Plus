#include "LogSystem.h"
#include "Utility.h"

using namespace std;
using namespace LuaSTGPlus;

__declspec(noinline) LogSystem& LogSystem::GetInstance()
{
	static LogSystem s_Instance;
	return s_Instance;
}

LogSystem::LogSystem()
{
	m_LogFile.open(LLOGFILE, std::ios::out);
	if (!m_LogFile.is_open())
		Log(LogType::Error, L"无法创建日志文件'%s'", LLOGFILE);
}

LogSystem::~LogSystem()
{
}

__declspec(noinline) void LogSystem::Log(LogType type, const wchar_t* info, ...)noexcept
{
	wstring tRet;

	try
	{
		switch (type)
		{
		case LogType::Error:
			tRet = L"[ERROR] ";
			break;
		case LogType::Warning:
			tRet = L"[WARN]  ";
			break;
		case LogType::Information:
			tRet = L"[INFO]  ";
			break;
		case LogType::Debug:
			tRet = L"[DEBUG] ";
			break;
		case LogType::Fatal:
			tRet = L"[FATAL] ";
			break;
		default:
			tRet = L"[INFO]  ";
			break;
		}

		va_list vargs;
		va_start(vargs, info);
		tRet += std::move(StringFormatV(info, vargs));
		va_end(vargs);
		tRet.push_back(L'\n');
	}
	catch (const bad_alloc&)
	{
		OutputDebugStringW(L"[ERROR] 记录日志时发生内存不足错误");
		return;
	}
	
	OutputDebugStringW(tRet.c_str());
	try
	{
		if (m_LogFile)
		{
			m_LogFile << std::move(fcyStringHelper::WideCharToMultiByte(tRet, CP_UTF8));
			m_LogFile.flush();
		}	
	}
	catch (const bad_alloc&)
	{
		OutputDebugStringW(L"[ERROR] 记录日志时发生内存不足错误");
		return;
	}
}

__declspec(noinline) void LogSystem::LogSnapshoot()noexcept
{
	// create dir
	BOOL ret = CreateDirectoryW(L"log", NULL);
	bool result = (ret != 0) || ((ret == 0) && (ret != ERROR_ALREADY_EXISTS)); // successful or failed (except that the directory already exists)
	if (!result) {
		Log(LogType::Error, L"无法创建日志快照文件夹'log'");
		return;
	}
	try {
		// read buffer
		std::vector<char> buffer;
		{
			// open
			std::fstream infile;
			infile.open(LLOGFILE, std::ios::in | std::ios::binary);
			if (!infile.is_open()) {
				Log(LogType::Error, L"无法读取日志内容");
				return;
			}
			// seek and get size
			infile.seekg(0, std::ios::end);
			auto endpos = m_LogFile.tellg();
			infile.seekg(0, std::ios::beg);
			auto begpos = infile.tellg();
			auto filesize = endpos - begpos;
			// read to buffer
			buffer.resize((size_t)filesize);
			infile.read(buffer.data(), filesize);
			// close
			infile.close();
		}
		// generate file name
		std::wstring filename;
		{
			// get time
			std::time_t rawtime = std::time(nullptr);
			std::tm tminfo;
			localtime_s(&tminfo, &rawtime);
			// format to string
			const size_t ymdhms_size = 15u; // 14, plus 1 to make c style string
			filename.resize(ymdhms_size, 0);
			std::wcsftime(filename.data(), ymdhms_size, L"%Y%m%d%H%M%S", &tminfo);
			filename.pop_back(); // remove tail 0
			filename = L"log/log" + filename + L".txt";
		}
		// open out file
		std::fstream outfile;
		outfile.open(filename, std::ios::out | std::ios::binary);
		if (!outfile.is_open()) {
			Log(LogType::Error, L"无法创建log快照文件'%s'", filename.c_str());
			return;
		}
		// write data
		outfile.write((char*)buffer.data(), buffer.size());
		outfile.flush();
		// close file
		outfile.close();
	}
	catch (...) {
		Log(LogType::Error, L"无法创建log快照");
	}
}
