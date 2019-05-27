#include <iostream>
#include <fstream>
#include <string>

#include "E2DCodePage.hpp"

using namespace std;
using namespace Eyes2D::String;

int main() {
	string fname = "log.txt";
	fstream f;
	f.open(fname, ios::out | ios::ate);
	if (!f.is_open())
	{
		f.clear();//clear the fstream error state
		f.open(fname, ios::out);//confirm that file is exist
		f.close();
		f.open(fname, ios::out | ios::ate);
	}

	f << "First line." << endl;
	f << "Second line." << endl;

	string str = "�����С�";
	string u8str = u8"�����С�";
	wstring wstr = L"�����С�";
	f << ANSIToUTF8(str) << endl;

	f.flush();

	f.close();
	system("pause");
	return 0;
}
