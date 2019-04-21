#define INITGUID
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>

#include "E2DException.h"
#include "E2DInputSystem.h"

#include <wbemidl.h>
#include <oleauto.h>
//#include <wmsstd.h>
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#endif
BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput)
{
	IWbemLocator* pIWbemLocator = NULL;
	IEnumWbemClassObject* pEnumDevices = NULL;
	IWbemClassObject* pDevices[20] = { 0 };
	IWbemServices* pIWbemServices = NULL;
	BSTR                  bstrNamespace = NULL;
	BSTR                  bstrDeviceID = NULL;
	BSTR                  bstrClassName = NULL;
	DWORD                 uReturned = 0;
	bool                  bIsXinputDevice = false;
	UINT                  iDevice = 0;
	VARIANT               var;
	HRESULT               hr;

	// CoInit if needed
	hr = CoInitialize(NULL);
	bool bCleanupCOM = SUCCEEDED(hr);

	// Create WMI
	hr = CoCreateInstance(__uuidof(WbemLocator),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IWbemLocator),
		(LPVOID*)& pIWbemLocator);
	if (FAILED(hr) || pIWbemLocator == NULL)
		goto LCleanup;

	bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2"); if (bstrNamespace == NULL) goto LCleanup;
	bstrClassName = SysAllocString(L"Win32_PNPEntity");   if (bstrClassName == NULL) goto LCleanup;
	bstrDeviceID = SysAllocString(L"DeviceID");          if (bstrDeviceID == NULL)  goto LCleanup;

	// Connect to WMI 
	hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0L,
		0L, NULL, NULL, &pIWbemServices);
	if (FAILED(hr) || pIWbemServices == NULL)
		goto LCleanup;

	// Switch security level to IMPERSONATE. 
	CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
		RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, NULL, &pEnumDevices);
	if (FAILED(hr) || pEnumDevices == NULL)
		goto LCleanup;

	// Loop over all devices
	for (;; )
	{
		// Get 20 at a time
		hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned);
		if (FAILED(hr))
			goto LCleanup;
		if (uReturned == 0)
			break;

		for (iDevice = 0; iDevice < uReturned; iDevice++)
		{
			// For each device, get its device ID
			hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, NULL, NULL);
			if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
			{
				// Check if the device ID contains "IG_".  If it does, then it's an XInput device
					// This information can not be found from DirectInput 
				if (wcsstr(var.bstrVal, L"IG_"))
				{
					// If it does, then get the VID/PID from var.bstrVal
					DWORD dwPid = 0, dwVid = 0;
					WCHAR* strVid = wcsstr(var.bstrVal, L"VID_");
					if (strVid && swscanf(strVid, L"VID_%4X", &dwVid) != 1)
						dwVid = 0;
					WCHAR * strPid = wcsstr(var.bstrVal, L"PID_");
					if (strPid && swscanf(strPid, L"PID_%4X", &dwPid) != 1)
						dwPid = 0;

					// Compare the VID/PID to the DInput device
					DWORD dwVidPid = MAKELONG(dwVid, dwPid);
					if (dwVidPid == pGuidProductFromDirectInput->Data1)
					{
						bIsXinputDevice = true;
						goto LCleanup;
					}
				}
			}
			SAFE_RELEASE(pDevices[iDevice]);
		}
	}

LCleanup:
	if (bstrNamespace)
		SysFreeString(bstrNamespace);
	if (bstrDeviceID)
		SysFreeString(bstrDeviceID);
	if (bstrClassName)
		SysFreeString(bstrClassName);
	for (iDevice = 0; iDevice < 20; iDevice++)
		SAFE_RELEASE(pDevices[iDevice]);
	SAFE_RELEASE(pEnumDevices);
	SAFE_RELEASE(pIWbemLocator);
	SAFE_RELEASE(pIWbemServices);

	if (bCleanupCOM)
		CoUninitialize();

	return bIsXinputDevice;
}

using namespace std;

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

static BOOL g_check;
static HANDLE g_handle;
static COORD g_cursor;
void StartCursor() {
	//隐藏控制台光标
	CONSOLE_CURSOR_INFO  cinfo;
	GetConsoleCursorInfo(g_handle, &cinfo);
	cinfo.bVisible = FALSE;
	SetConsoleCursorInfo(g_handle, &cinfo);
	//先查询控制台光标位置
	CONSOLE_SCREEN_BUFFER_INFO sinfo;
	g_check = GetConsoleScreenBufferInfo(g_handle, &sinfo);
	if (g_check == TRUE) {
		g_cursor = sinfo.dwCursorPosition;
	}
}
void BackCursor() {
	//设置控制台光标位置
	if (g_check == TRUE) {
		SetConsoleCursorPosition(g_handle, g_cursor);
	}
}
void EndCursor() {
	//恢复控制台光标显示
	CONSOLE_CURSOR_INFO  cinfo;
	GetConsoleCursorInfo(g_handle, &cinfo);
	cinfo.bVisible = TRUE;
	SetConsoleCursorInfo(g_handle, &cinfo);
}

static vector<LPCDIDEVICEINSTANCE> g_keyboards;
static int g_count = 0;
static BOOL CALLBACK enumDev(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	g_keyboards.push_back(lpddi);
	g_count = g_count + 1;
	cout << "发现键盘设备" << g_count << endl;
	return DIENUM_CONTINUE;//继续枚举
}

static int g_count2 = 0;
static BOOL CALLBACK enumDev2(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	g_count2 = g_count2 + 1;
	cout << "发现手柄设备" << g_count2;
	if (IsXInputDevice(&lpddi->guidInstance)) {
		cout << "，XInput设备" << endl;
	}
	else {
		cout << "，传统DInput设备" << endl;
	}
	return DIENUM_CONTINUE;//继续枚举
}

static HWND g_hWnd;

typedef struct {
	DWORD vkey;
	string name;
} VKcodeToName;

static const int VKcodeToNameIndexLen = 4;
static VKcodeToName VKcodeToNameIndex[VKcodeToNameIndexLen] = {
	{DIK_LEFT,"LEFT"},
	{DIK_RIGHT,"RIGHT"},
	{DIK_UP,"UP"},
	{DIK_DOWN,"DOWN"},
};

int main() {
	g_hWnd = GetConsoleWindow();//获取当前窗口句柄
	g_handle = GetStdHandle(STD_OUTPUT_HANDLE);//获取当前标准控制台文本输出设备句柄

	Eyes2D::InputSystem *sys;
	try {
		sys = new Eyes2D::InputSystem();
	}
	catch (Eyes2D::E2DException e) {
		wcout << e.errDesc << endl;
	}

	//创建dinput8
	ComPtr<IDirectInput8> DInput8;
	{
		if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)DInput8.GetAddressOf(), NULL) == DI_OK) {
			cout << "DirectInput8创建成功" << endl;
		}
		else {
			cout << "DirectInput8创建失败" << endl;
			system("pause");
			return -1;
		}
	}

	//枚举键盘设备
	{
		g_count = 0;
		if (DInput8->EnumDevices(DI8DEVCLASS_KEYBOARD, enumDev, NULL, DIEDFL_ALLDEVICES) == DI_OK) {
			cout << "枚举键盘设备成功" << endl;
		}
		else {
			cout << "枚举键盘设备失败" << endl;
			system("pause");
			return -1;
		}
	}

	//枚举手柄设备
	{
		g_count2 = 0;
		if (DInput8->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDev2, NULL, DIEDFL_ALLDEVICES) == DI_OK) {
			cout << "枚举手柄设备成功" << endl;
		}
		else {
			cout << "枚举手柄设备失败" << endl;
			system("pause");
			return -1;
		}
	}

	//创建键盘设备
	ComPtr<IDirectInputDevice8> PC_KeyBoard;
	{
		GUID PC_KeyBoard_GUID = GUID_SysKeyboard;
		for (auto item : g_keyboards) {
			GUID tGUID = item->guidInstance;
			break;
		}
		if (DInput8->CreateDevice(PC_KeyBoard_GUID, PC_KeyBoard.GetAddressOf(), NULL) == DI_OK) {
			cout << "创建键盘设备成功" << endl;
		}
		else {
			cout << "创建键盘设备失败" << endl;
			system("pause");
			return -1;
		}
	}

	//设置键盘协作方式
	{
		//前台非独占访问
		if (PC_KeyBoard->SetCooperativeLevel(g_hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) == DI_OK) {
			cout << "设置键盘协作方式成功" << endl;
		}
		else {
			cout << "设置键盘协作方式失败" << endl;
			system("pause");
			return -1;
		}
	}

	//设置键盘数据格式
	{
		if (PC_KeyBoard->SetDataFormat(&c_dfDIKeyboard) == DI_OK) {
			cout << "设置键盘数据格式成功" << endl;
		}
		else {
			cout << "设置键盘数据格式失败" << endl;
			system("pause");
			return -1;
		}
	}

	static const DWORD g_DataCount = 256;
	//设置键盘属性
	{
		DIPROPDWORD property;
		ZeroMemory(&property, sizeof(property));
		property.diph.dwSize = sizeof(DIPROPDWORD);
		property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		property.diph.dwObj = 0;
		property.diph.dwHow = DIPH_DEVICE;
		property.dwData = g_DataCount;

		if (PC_KeyBoard->SetProperty(DIPROP_BUFFERSIZE, &property.diph) == DI_OK) {
			cout << "设置键盘数据缓冲区属性成功" << endl;
		}
		else {
			cout << "设置键盘数据缓冲区属性失败" << endl;
			system("pause");
			return -1;
		}
	}

	//循环检索键盘输入
	{
		bool exit = false;
		while (true) {
			while (PC_KeyBoard->Acquire() != DI_OK) {
				cout << "尝试获取设备失败                " << endl;
				Sleep(1000);
			}
			cout << "获取设备成功                    " << endl;
			cout << "开始检测按键输入，按下ESC退出  " << endl;

			StartCursor();

			while (true) {
				BackCursor();

				bool checkflag[4] = { false,false,false,false };
				DIDEVICEOBJECTDATA keyboardData[g_DataCount];
				DWORD dSize = g_DataCount;
				if (PC_KeyBoard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), keyboardData, &dSize, 0) == DI_OK) {
					cout << "================================" << endl;
					for (int select = 0; select < VKcodeToNameIndexLen; select++) {
						bool find = false;
						for (DWORD i = 0; i < dSize; i++) {
							DWORD vkey = keyboardData[i].dwOfs;
							DWORD trigger = keyboardData[i].dwData;
							if (vkey == DIK_ESCAPE) {
								exit = true;
							}
							if (vkey == VKcodeToNameIndex[select].vkey) {
								if (trigger & 0x80) {
									cout << VKcodeToNameIndex[select].name << ":+ ";
								}
								else {
									cout << VKcodeToNameIndex[select].name << ":  ";
								}
								find = true;
							}
							if (find) {
								break;
							}
							else {
								cout << VKcodeToNameIndex[select].name << ":  ";
								find = true;
								break;
							}
						}
						if (!find) {
							cout << VKcodeToNameIndex[select].name << ":  ";
						}
					}
					cout << endl;
					cout << "================================" << endl;
				}
				else {
					cout << "设备丢失                        " << endl;
					break;
				}

				if (!exit) {
					Sleep(100);
				}
				else {
					break;
				}
			}

			EndCursor();

			if (exit) {
				break;
			}
		}
	}

	//返回设备
	{
		if (PC_KeyBoard->Unacquire() == DI_OK) {
			cout << "已返回设备" << endl;
		}
	}

	system("pause");
	return 0;
}
