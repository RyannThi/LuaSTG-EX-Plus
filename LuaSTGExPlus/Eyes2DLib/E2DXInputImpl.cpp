#include "E2DXInputImpl.h"

using namespace Eyes2D;

int XInputImpl::Refresh() {
	memset(m_XDevices, 0, (XUSER_MAX_COUNT + 1) * sizeof(XINPUT_STATE));//重置原有的信息
	int devCount = 0;
	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		XINPUT_STATE state;
		memset(&state, 0, sizeof(XINPUT_STATE));
		if (XInputGetState(i, &state) == ERROR_SUCCESS) {
			m_XDevices[i + 1] = state;
			devCount++;
		}
		else {
			break;
		}
	}
	m_XDeviceCount = devCount;
	return devCount;
}

XDeviceInfo XInputImpl::GetDeviceInfo(int index) {
	XDeviceInfo info = XDeviceInfo(false);

	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return info;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	//--其他信息
	XINPUT_CAPABILITIES features;
	memset(&features, 0, sizeof(XINPUT_CAPABILITIES));
	//不应该限制于只查询XINPUT_FLAG_GAMEPAD（Xbox 360 Controller）类型的设备
	if (XInputGetCapabilities(index - 1, 0, &features) == ERROR_SUCCESS) {
		info.Success = true;
		//读取设备类型
		switch (features.SubType)
		{
		case XINPUT_DEVSUBTYPE_GAMEPAD:
			info.DeviceType = XDeviceType::GAMEPAD;
			info.DeviceTypeDesc = XDeviceTypeDesc[0];
			break;
		case XINPUT_DEVSUBTYPE_WHEEL:
			info.DeviceType = XDeviceType::WHEEL;
			info.DeviceTypeDesc = XDeviceTypeDesc[1];
			break;
		case XINPUT_DEVSUBTYPE_ARCADE_STICK:
			info.DeviceType = XDeviceType::ARCADE_STICK;
			info.DeviceTypeDesc = XDeviceTypeDesc[2];
			break;
		case XINPUT_DEVSUBTYPE_FLIGHT_STICK:
			info.DeviceType = XDeviceType::FLIGHT_STICK;
			info.DeviceTypeDesc = XDeviceTypeDesc[3];
			break;
		case XINPUT_DEVSUBTYPE_ARCADE_PAD:
			info.DeviceType = XDeviceType::ARCADE_PAD;
			info.DeviceTypeDesc = XDeviceTypeDesc[9];
			break;
		default:
			break;
		}
		//无线、有线
		info.Wireless = ((features.Flags & XINPUT_CAPS_WIRELESS) != 0) ? true : false;
		//力反馈
		info.ForceFeedback = ((features.Flags & XINPUT_CAPS_FFB_SUPPORTED) != 0) ? true : false;
		//导航按钮
		info.Navigation = ((features.Flags & XINPUT_CAPS_NO_NAVIGATION) != 0) ? false : true;//看清楚是NO_NAVIGATION草
		//马达状态
		if (info.ForceFeedback) {
			info.LMotorSpeed = features.Vibration.wLeftMotorSpeed;
			info.HMotorSpeed = features.Vibration.wRightMotorSpeed;
		}
	}
	
#ifdef EYES2D_USING_XINPUT_1_4
	//--电池
	XINPUT_BATTERY_INFORMATION BatteryInfo;
	memset(&BatteryInfo, 0, sizeof(XINPUT_BATTERY_INFORMATION));
	if (XInputGetBatteryInformation(index - 1, 0, &BatteryInfo) == ERROR_SUCCESS) {
		if (BatteryInfo.BatteryType != BATTERY_TYPE_DISCONNECTED) {
			//电源类型
			switch (BatteryInfo.BatteryType)
			{
			case BATTERY_TYPE_WIRED:
				info.BatteryType = XDeviceBatteryType::WIRED;
				info.BatteryTypeDesc = XDeviceBatteryTypeDesc[0];
				break;
			case BATTERY_TYPE_ALKALINE:
				info.BatteryType = XDeviceBatteryType::ALKALINE;
				info.BatteryTypeDesc = XDeviceBatteryTypeDesc[1];
				break;
			case BATTERY_TYPE_NIMH:
				info.BatteryType = XDeviceBatteryType::NIMH;
				info.BatteryTypeDesc = XDeviceBatteryTypeDesc[2];
				break;
			default:
				break;
			}
			//电量
			if ((BatteryInfo.BatteryType == BATTERY_TYPE_ALKALINE) || (BatteryInfo.BatteryType == BATTERY_TYPE_NIMH)) {
				switch (BatteryInfo.BatteryLevel)
				{
				case BATTERY_LEVEL_EMPTY:
					info.Battery = XDeviceBatteryLV::EMPTY;
					break;
				case BATTERY_LEVEL_LOW:
					info.Battery = XDeviceBatteryLV::LOW;
					break;
				case BATTERY_LEVEL_MEDIUM:
					info.Battery = XDeviceBatteryLV::MEDIUM;
					break;
				case BATTERY_LEVEL_FULL:
					info.Battery = XDeviceBatteryLV::FULL;
					break;
				}
			}
		}
	}
#endif // USING_XINPUT_1_4

	return info;
}

void XInputImpl::Update() {
	DWORD j = m_XDeviceCount;//先转换成DWORD，处理<运算符符号不匹配
	for (DWORD i = 0; i < j; i++)
	{
		XINPUT_STATE state;
		memset(&state, 0, sizeof(XINPUT_STATE));
		if (XInputGetState(i, &state) == ERROR_SUCCESS) {
			m_XDevices[i + 1] = state;
		}
		else {
			Refresh();//失败则刷新状态
			break;
		}
	}
}

bool XInputImpl::GetKeyState(int index, int vkey) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return false;//超出范围
	}
	
	if (m_XDevices[index].Gamepad.wButtons & vkey) {
		return true;
	}
	else {
		return false;
	}
}

int XInputImpl::GetTriggerStateL(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.bLeftTrigger;
}

int XInputImpl::GetTriggerStateR(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.bRightTrigger;
}

int XInputImpl::GetThumbStateLX(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.sThumbLX;
}

int XInputImpl::GetThumbStateLY(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.sThumbLY;
}

int XInputImpl::GetThumbStateRX(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}

	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.sThumbRX;
}

int XInputImpl::GetThumbStateRY(int index) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return 0;//超出范围
	}
	
	//index是以1为索引头，因此需要先减一
	return m_XDevices[index].Gamepad.sThumbRY;
}

bool XInputImpl::SetMotorSpeed(int index, int L, int H) {
	if ((index > XUSER_MAX_COUNT) || (index > m_XDeviceCount) || (index < 1)) {
		return false;//超出范围
	}

	//索引要-1
	if (L < 0 || H < 0) {
		//针对一项进行更改
		XINPUT_CAPABILITIES features;
		memset(&features, 0, sizeof(XINPUT_CAPABILITIES));
		if (XInputGetCapabilities(index-1, 0, &features) != ERROR_SUCCESS) {
			return false;
		}
		XINPUT_VIBRATION var = features.Vibration;
		if (L >= 0) {
			var.wLeftMotorSpeed = L;
		}
		if (H >= 0) {
			var.wRightMotorSpeed = H;
		}
		return (XInputSetState(index-1, &var) == ERROR_SUCCESS) ? true : false;
	}
	else {
		XINPUT_VIBRATION var;
		var.wLeftMotorSpeed = L;
		var.wRightMotorSpeed = H;
		return (XInputSetState(index-1, &var) == ERROR_SUCCESS) ? true : false;
	}
}
