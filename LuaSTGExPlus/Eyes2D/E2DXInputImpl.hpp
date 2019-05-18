#pragma once
/*========================================*\
|*Code by Xiliusha                        *|
|*与微软的XInput对接                      *|
\*========================================*/

#include "E2DGlobal.hpp"
#include <string>

#pragma comment(lib, "Xinput.lib")

#include <Windows.h>
#include "XInput.h"

namespace Eyes2D {
	//设备类型
	enum class XDeviceType
	{
		GAMEPAD,//标准游戏手柄
		WHEEL,//赛车方向盘
		ARCADE_STICK,//街机摇杆
		FLIGHT_STICK,//飞行摇杆
		DANCE_PAD,//跳舞毯
		GUITAR,//吉他
		GUITAR_BASS,//贝斯
		GUITAR_ALTERNATE,//吉他替代物
		DRUM_KIT,//鼓机
		ARCADE_PAD,//街机控制器
		UNKNOWN,//未知设备
	};
	
	//设备类型描述（静态字符串）
	static std::wstring XDeviceTypeDesc[11] = {
		L"手柄",L"赛车方向盘",L"街机摇杆",L"飞行摇杆",
		L"跳舞毯",L"吉他",L"贝斯",L"吉他替代物",L"鼓",
		L"街机控制器",
		L"未知设备"
	};

	//电源类型
	enum class XDeviceBatteryType {
		WIRED,//有线设备
		ALKALINE,//碱性电池
		NIMH,//镍金属氢化物电池
		UNKNOWN,//未知
	};

	//电源类型描述（静态字符串）
	static std::wstring XDeviceBatteryTypeDesc[4] = {
		L"有线设备",
		L"碱性电池",
		L"充电电池",
		L"未知"
	};

	//电量
	enum class XDeviceBatteryLV {EMPTY, LOW, MEDIUM, FULL,};

	//设备信息
	struct XDeviceInfo
	{
		XDeviceType DeviceType;//设备类型
		std::wstring DeviceTypeDesc;//设备类型描述
		bool Wireless;//无线设备？
		bool Navigation;//有无导航按钮（start、back等）
		bool ForceFeedback;//有无力反馈？
		int LMotorSpeed;//低速马达状态
		int HMotorSpeed;//高速马达状态
		bool Success;//是否成功读取
		XDeviceBatteryType BatteryType;//电源类型
		std::wstring BatteryTypeDesc;//电源类型描述
		XDeviceBatteryLV Battery;//电量
		XDeviceInfo(bool success) {
			DeviceType = XDeviceType::UNKNOWN;
			DeviceTypeDesc = XDeviceTypeDesc[10];
			Wireless = false;
			Navigation = false;
			ForceFeedback = false;
			LMotorSpeed = 0;
			HMotorSpeed = 0;
			Success = success;
			BatteryType = XDeviceBatteryType::UNKNOWN;
			BatteryTypeDesc = XDeviceBatteryTypeDesc[3];
			Battery = XDeviceBatteryLV::EMPTY;
		}
	};
	
	//XInput实现
	class EYESDLLAPI XInputImpl {
	private:
		int m_XDeviceCount;                           //检测到的设备数量
		XINPUT_STATE m_XDevices[XUSER_MAX_COUNT + 1]; //调整索引从1开始
	public:
		XInputImpl() {
			m_XDeviceCount = 0;
			memset(m_XDevices, 0, (XUSER_MAX_COUNT + 1) * sizeof(XINPUT_STATE));
			Refresh();
		}
		~XInputImpl() {
			//记得把所有设备的震动关了……不然就悲剧咯
			for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
				SetMotorSpeed(i + 1, 0, 0);
			}
		}
	public:
		int GetDeviceCount() { return m_XDeviceCount; } //返回设备数量
		int Refresh();                                  //刷新设备，并返回设备数量
		XDeviceInfo GetDeviceInfo(int index);           //获取设备信息
		void Update();                                  //获取设备输入，如果设备丢失则刷新设备以及设备数量
	public:
		bool GetKeyState(int index,int vkey);           //获取指定设备的按键状态
		int GetTriggerStateL(int index);                //获取指定设备左扳机状态
		int GetTriggerStateR(int index);                //获取指定设备右扳机状态
		int GetThumbStateLX(int index);                 //获取指定设备左摇杆X轴状态
		int GetThumbStateLY(int index);                 //获取指定设备左摇杆Y轴状态
		int GetThumbStateRX(int index);                 //获取指定设备右摇杆X轴状态
		int GetThumbStateRY(int index);                 //获取指定设备右摇杆Y轴状态
		bool SetMotorSpeed(int index, int L, int H);    //设置马达震动，需要设备支持，如果L或H或LH都填-1，则不进行更改
	};
	
	//获得XInput单例
	//在多线程环境下，该单例的实现是有问题的……
	inline XInputImpl& GetXInput() {
		static XInputImpl gs_XInputInstance;
		return gs_XInputInstance;
	}
}
