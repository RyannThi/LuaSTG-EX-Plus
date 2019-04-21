#include"E2DException.h"
#include"E2DInputSystem.h"

using namespace Eyes2D;

//--------------------------------------

BOOL CALLBACK InputSystem::enumKeyboard(LPCDIDEVICEINSTANCE pInfo, void* pThis) {
}

InputSystem::InputSystem() {
	HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)m_DInput8.GetAddressOf(), NULL);
	if (hr != DI_OK) {
		throw E2DException(-1, hr, L"Eyes2D::InputSystemImpl::InputSystemImpl", L"无法创建DirectInput8");
	}
}

InputSystem::~InputSystem() {
	
}

void InputSystem::EnumDevices() {
}
