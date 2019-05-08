#include <iostream>
#include"E2DXAudio2.hpp"

using namespace std;
using namespace Eyes2D;

int main() {
	XAudio2Engine* sound = nullptr;
	try {
		sound = new XAudio2Engine();
		sound->SetMasterVolume(1.0f);
		sound->SetMixerVolume("BGM", 0.8f);
		cout << "Master:" << sound->GetMasterVolume() << "  BGM Mixer:" << sound->GetMixerVolume("BGM") << endl;
	}
	catch (E2DException e) {
		cout << e.errResult << endl;
	}

	system("pause");

	delete sound;

	system("pause");
	return 0;
}
