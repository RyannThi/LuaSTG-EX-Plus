#include <iostream>
#include <thread>
#include <chrono>
#include "E2DXInputImpl.h"

using namespace std;
using namespace Eyes2D;
using namespace std::chrono_literals;

//测试单例在多线程下的工作情况

void GetState() {
	while (true) {
		cout << GetXInput().GetThumbStateLX(1) << endl;
		this_thread::sleep_for(1s);
	}
}

void UpdateState() {
	while (true) {
		GetXInput().Update();
		cout << "Update" << endl;
		this_thread::sleep_for(1s);
	}
}

int main() {
	thread t1(GetState);
	thread t2(UpdateState);
	t2.join();
	t1.join();
	system("pause");
	return 0;
}
