#pragma once

#pragma comment(lib, "dinput8.lib")

#include <queue>
#include <vector>

#include <Windows.h>
#include <wrl.h>
#include <dinput.h>

namespace Eyes2D {
	enum class InputDeviceType {
		Mouse,
		Keyboard,
		Joystick,
	};
	
	class DeviceInput {
	public:
		virtual bool Update() = 0;
		virtual bool IsValid() = 0;
	};

	class InputSystem;

	class KeyboardInput : public DeviceInput {
	public:
		bool IsKeyDown(int keycode);
		bool Update();
		bool IsValid();
	};

	class MouseInput :public DeviceInput {
	public:
		bool IsKeyDown(int keycode);
		bool Update();
		bool IsValid();
	};

	enum class InputEventType {
		MouseMoveX,
		MouseMoveY,
		MouseMoveZ,
		MouseButtonUp,
		MouseButtonDown,

		KeyboardButtonUp,
		KeyboardButtonDown,
	};

	struct InputEvent
	{
		InputEventType EventType;
		int iCode;
		long lNumber;
		double dNumber;
		bool bResult;
	};
	
	class InputSystem {
	private:
		Microsoft::WRL::ComPtr<IDirectInput8W> m_DInput8;
		std::queue<InputEvent> m_MsgQueue;
		std::vector<DeviceInput*> m_Inputs;
	public:
		InputSystem();
		~InputSystem();
	public:
		void AddInput(DeviceInput* pInput)
		{
			m_Inputs.push_back(pInput);
		}
		void RemoveInput(DeviceInput* pInput)
		{
			std::vector<DeviceInput*>::iterator i = m_Inputs.begin();
			while (i != m_Inputs.end())
			{
				if (*i == pInput) {
					m_Inputs.erase(i);
					break;
				}
				else {
					++i;
				}
			}
		}
		void Update()
		{
			std::vector<DeviceInput*>::iterator i = m_Inputs.begin();
			while (i != m_Inputs.end())
			{
				(*i)->Update();
				++i;
			}
		}
	private:
		static BOOL CALLBACK enumMouse(LPCDIDEVICEINSTANCE pInfo, void* pThis);
		static BOOL CALLBACK enumKeyboard(LPCDIDEVICEINSTANCE pInfo, void* pThis);
		static BOOL CALLBACK enumJoystick(LPCDIDEVICEINSTANCE pInfo, void* pThis);
	public:
		void EnumDevices();
		void CreateKeyboardInput(KeyboardInput** out);
	};
}
