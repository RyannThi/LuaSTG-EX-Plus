#include <crtdbg.h>
#include <iostream>
#include "E2DAudioEngine.hpp"

using namespace std;

int main() {
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	Eyes2D::Sound::AudioEngine* engine = nullptr;
	try {
		Eyes2D::Sound::AudioEngineImpl* _engine = new Eyes2D::Sound::AudioEngineImpl();
		engine = _engine;
	}
	catch (const Eyes2D::E2DException& e) {
		return -1;
	}

	unsigned int uid = 0;
	try {
		Eyes2D::Sound::AudioMixer* mixer = nullptr;
		uid = engine->CreateMixer(&mixer);
		if (mixer != nullptr) {
			cout << mixer->GetID() << endl;
			if (!engine->SetVolume(0.5f)) cout << "failed to set engine volume" << endl;
			if (!mixer->SetVolume(0.5f)) cout << "failed to set mixer volume" << endl;
			cout << engine->GetVolume() << endl;
			cout << mixer->GetVolume() << endl;
		}
	}
	catch (const Eyes2D::E2DException& e) {
		delete engine;
		system("pause");
		return -1;
	}
	
	Eyes2D::Sound::AudioMixer* mixer = engine->GetMixer(uid);
	if (mixer != nullptr) {
		cout << mixer->GetID() << endl;
		if (!mixer->SetVolume(0.9961f)) cout << "failed to set mixer volume" << endl;
		cout << mixer->GetVolume() << endl;
	}
	mixer = nullptr;

	if (!engine->RemoveMixer(uid)) cout << "failed to remove mixer" << endl;

	if (mixer == nullptr) {
		cout << "failed to get mixer" << endl;
	}

	system("pause");
	delete engine;
	system("pause");
	return 0;
}
