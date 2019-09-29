/// @file LogSystem.h
/// @brief 定义日志系统
/// @note 由于日志系统将被Common.h包含，因此独立于Common.h
#pragma once
#include <fstream>

namespace LuaSTGPlus
{
	/// @brief 日志级别
	enum class LogType
	{
		Debug,
		Information,
		Warning,
		Error,
		Fatal,
	};

	/// @brief 日志系统
	class LogSystem
	{
	private:
		std::fstream m_LogFile;
	public:
		//获取日志系统实例
		static __declspec(noinline) LogSystem& GetInstance();

		//记录日志
		//param[in] type 日志类型
		//param[in] info 格式化文本
		__declspec(noinline) void Log(LogType type, const wchar_t* info, ...)noexcept;

		//生成日志快照
		__declspec(noinline) void LogSnapshoot()noexcept;
	public:
		LogSystem();
		~LogSystem();
	};
}
