#include <iostream>
#include <string>
#include <Windows.h>
#include <wrl.h>
#include <Shobjidl.h>
#include <Knownfolders.h>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class CoScope {
private:
	bool m_Status = false;
public:
	bool operator ()()const { return m_Status; }
	CoScope() { m_Status = SUCCEEDED(CoInitialize(nullptr)); }
	~CoScope() { if (m_Status) CoUninitialize(); }
};

int main() {
	CoScope co;
	if (co())
	{
		HRESULT hr;
		ComPtr<IKnownFolderManager> kfm = nullptr;
		hr = CoCreateInstance(CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_IKnownFolderManager, (LPVOID*)kfm.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			ComPtr<IKnownFolder> kf = nullptr;
			std::wstring path = L"";
			//FOLDERID_LocalAppData
			//FOLDERID_RoamingAppData
;			if (SUCCEEDED(kfm->GetFolder(FOLDERID_RoamingAppData, kf.GetAddressOf()))) {
				LPWSTR _path = nullptr;
				if (SUCCEEDED(kf->GetPath(0, &_path))) {
					path = _path;
					path.push_back(L'\\');
					std::wcout << path << std::endl;
				}
			}
			kf = nullptr;
			path = L"";
			if (SUCCEEDED(kfm->GetFolder(FOLDERID_LocalAppData, kf.GetAddressOf()))) {
				LPWSTR _path = nullptr;
				if (SUCCEEDED(kf->GetPath(0, &_path))) {
					path = _path;
					path.push_back(L'\\');
					std::wcout << path << std::endl;
				}
			}
		}
	}
	std::system("pause");
	return 0;
}
