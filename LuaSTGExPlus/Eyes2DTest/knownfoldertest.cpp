#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <Windows.h>
#include <wrl.h>
#include <Shobjidl.h>
#include <Knownfolders.h>

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using std::cout;
using std::endl;

class CoScope {
private:
	bool m_Status = false;
public:
	bool operator ()()const { return m_Status; }
	CoScope() { m_Status = SUCCEEDED(CoInitialize(nullptr)); }
	~CoScope() { if (m_Status) CoUninitialize(); }
};

void KnownFolderTest() {
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
}

int main() {
	//*
	// get time
	std::time_t rawtime = std::time(nullptr);
	if (rawtime == (std::time_t)(-1)) {
		cout << "Failed to get time." << endl;
	}
	std::tm tminfo;
	localtime_s(&tminfo, &rawtime);
	// generate time string
	size_t ymdhms_size = 15u; // 14, plus 1 to make c style string
	std::string ymdhms = "";
	ymdhms.resize(ymdhms_size, 0);
	std::strftime(ymdhms.data(), ymdhms_size, "%Y%m%d%H%M%S", &tminfo);
	ymdhms.pop_back(); // remove tail 0
	ymdhms = "log/log" + ymdhms + ".txt";
	// print time
	cout << ymdhms << endl;
	//*/

	std::system("pause");
	return 0;
}
