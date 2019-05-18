#pragma comment(lib, "dxgi.lib")

#include "E2DDXGIImpl.hpp"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using namespace std;
using namespace Eyes2D;

DXGIImpl::DXGIImpl() {
	m_DXGI = nullptr;

	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&m_DXGI));
	if (hr != S_OK)
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::DXGIImpl", L"Failed to create DXGIFactory.");
}

DXGIImpl::~DXGIImpl() {
	if (m_DXGI) {
		m_DXGI->Release();
	}
	m_DXGI = nullptr;
}

bool DXGIImpl::SimpleGetResolutions(int*& width, int*& height, int& counts) {
	HRESULT hr;
	ComPtr<IDXGIAdapter> adapter;
	hr = m_DXGI->EnumAdapters(0, adapter.GetAddressOf());
	if (hr != S_OK)
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::SimpleEnumResolutions", L"Failed to create DXGIAdapter.");
	ComPtr<IDXGIOutput> output;
	hr = adapter->EnumOutputs(0, output.GetAddressOf());
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::SimpleEnumResolutions", L"Failed to create DXGIOutput.");
	DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;//对应D3DFMT_A8R8G8B8
	UINT flag = DXGI_ENUM_MODES_INTERLACED;
	UINT cnt = 0;
	hr = output->GetDisplayModeList(format, flag, &cnt, NULL);
	if (FAILED(hr))
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::SimpleEnumResolutions", L"Failed to get display mode list count.");
	DXGI_MODE_DESC* modes = new DXGI_MODE_DESC[cnt];
	int* w = new int[cnt];
	int* h = new int[cnt];
	if (modes == nullptr || w == nullptr || h == nullptr)
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::SimpleEnumResolutions", L"Failed to alloc arrays.");
	hr = output->GetDisplayModeList(format, flag, &cnt, modes);
	if (FAILED(hr)) {
		delete[] modes;
		delete[] w;
		delete[] h;
		throw E2DException(0, hr, L"Eyes2D::DXGIImpl::SimpleEnumResolutions", L"Failed to get display mode list.");
	}
	for (UINT index = 0; index < cnt; index++) {
		//这里会触发C6385和C6386警告，但是代码是正确的
		DXGI_MODE_DESC desc = modes[index];
		w[index] = desc.Width;
		h[index] = desc.Height;
	}
	//擦屁股
	delete[] modes;
	//输出
	counts = (int)cnt;
	width = w;
	height = h;
	return true;
}
