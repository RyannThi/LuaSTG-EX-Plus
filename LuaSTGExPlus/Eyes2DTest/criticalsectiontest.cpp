#include "E2DMainFunction.hpp"
#define E2DMAINRETURN std::system("pause"); return 0;

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <Windows.h>

using namespace std;

E2DMAIN {
	string fname = "LuaSTG-x TPv6.0.zip";
	system("pause");

	for (int t = 0; t < 10; t++) {
		{
			fstream f;
			vector<uint8_t> databuffer;

			cout << "fstream" << endl;

			auto start = std::chrono::high_resolution_clock::now();

			f.open(fname, ios::in | ios::binary);
			if (!f.is_open()) {
				cout << "Failed to open file." << endl;
			}
			else {
				f.seekg(0, ios::beg);
				size_t pos1 = f.tellg();
				f.seekg(0, ios::end);
				size_t pos2 = f.tellg();
				size_t buffersize = pos2 - pos1;
				cout << "File " << buffersize << " bytes" << endl;

				databuffer.resize((size_t)buffersize, 0);

				f.seekg(0, ios::beg);
				f.read((char*)databuffer.data(), buffersize);

				pos2 = f.tellg();
				buffersize = pos2 - pos1;
				cout << "Read " << buffersize << " bytes" << endl;

				f.close();
			}

			auto end = std::chrono::high_resolution_clock::now();
			chrono::duration<double, ratio<1, 1>> diff = end - start;//频率为每秒1次
			cout << diff.count() << "s" << endl;

			databuffer.clear();
		}

		{
			HANDLE f;
			vector<uint8_t> databuffer;

			cout << "WIN" << endl;

			auto start = std::chrono::high_resolution_clock::now();

			f = ::CreateFileA(
				fname.data(),
				GENERIC_READ,			//读取权限
				FILE_SHARE_READ,		//共享读取
				NULL,					//安全属性结构
				OPEN_EXISTING,			//打开存在的文件
				FILE_ATTRIBUTE_NORMAL,	//普通属性
				NULL					//不使用模板文件
			);
			if (f == INVALID_HANDLE_VALUE) {
				cout << "Failed to open file." << endl;
			}
			else {
				DWORD pos1 = SetFilePointer(f, 0, NULL, FILE_BEGIN);
				DWORD pos2 = SetFilePointer(f, 0, NULL, FILE_END);
				DWORD buffersize = pos2 - pos1;
				cout << "File " << buffersize << " bytes" << endl;

				databuffer.resize((size_t)buffersize, 0);

				DWORD read;
				SetFilePointer(f, 0, NULL, FILE_BEGIN);
				BOOL ret = ReadFile(f, (LPVOID)databuffer.data(), buffersize, &read, NULL);
				if (ret == TRUE) {
					cout << "Read " << read << " bytes" << endl;
				}

				CloseHandle(f);
			}

			auto end = std::chrono::high_resolution_clock::now();
			chrono::duration<double, ratio<1, 1>> diff = end - start;//频率为每秒1次
			cout << diff.count() << "s" << endl;

			databuffer.clear();
		}

		Sleep(1000);
	}

	E2DMAINRETURN;
}
