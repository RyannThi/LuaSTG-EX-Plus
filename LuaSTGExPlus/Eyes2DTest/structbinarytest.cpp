#include <iostream>
#include <fstream>
#include <string>

struct _ReplayFrameRecord {
	bool left;
	bool right;
	bool up;
	bool down;
	bool slow;
	bool shoot;
	bool spell;
	bool special;
};

struct ReplayFrameRecord {
	bool left : 1;
	bool right : 1;
	bool up : 1;
	bool down : 1;
	bool slow : 1;
	bool shoot : 1;
	bool spell : 1;
	bool special : 1;
};

int _s = sizeof(_ReplayFrameRecord);
int s = sizeof(ReplayFrameRecord);
static_assert(std::is_pod<ReplayFrameRecord>::value);

using namespace std;

int main() {
	string fname = "slot.rep";

	{
		fstream f; int mode = ios::out | ios::ate | ios::binary;
		f.open(fname, mode);
		if (!f.is_open())
		{
			f.clear();//clear the fstream error state
			f.open(fname, ios::out);//confirm that file is exist
			f.close();
			f.open(fname, mode);
		}

		ReplayFrameRecord rcs[10];
		memset(rcs, 0, size(rcs));
		rcs[0].left = true;
		rcs[5].up = true;

		for (int e = 0; e < 10; e++) {
			f.write((char*)(rcs + e), 1);
		}

		f.flush();
		f.close();
	}

	{
		fstream f; int mode = ios::in | ios::binary;
		f.open(fname, mode);

		ReplayFrameRecord rcs[10];
		memset(rcs, 0, size(rcs));
		f.read((char*)rcs, 10);



		f.close();
	}

	return 0;
}
