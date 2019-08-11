#include <iostream>
#include <sstream>
#include <vector>

#define PAUSE std::system("pause");

static const char* input = "3 3 3 \
1 2 1 \
0 2 0 \
1 3 1 \
";

struct boardop
{
	int i=0;
	int j=0;
	int c=0;
};

int main(int argc, char* argv[]) {
	using namespace std;

	stringstream ss;
	ss.clear();
	ss << input;
	
	int m, n, g;
	ss >> m >> n >> g;

	//0 no use, 1 white, 2 black
	int* mflag = new int[m + 1];
	int* nflag = new int[n + 1];
	memset(mflag, 0, (m + 1) * sizeof(int));
	memset(nflag, 0, (n + 1) * sizeof(int));

	int mbcount = 0;
	int nbcount = 0;
	int mwcount = 0;
	int nwcount = 0;
	int count = 0;
	int i, j, c;
	for (int e = 0; e < g; e++) {
		ss >> i >> j >> c;
		if (c == 1) {
			//black
			if (i == 1) {
				//n
				if (nflag[j]==0) {
					nflag[j] = 2;
					nbcount++;
					count += m;//列的高度是行数
				}
				else if (nflag[j] == 1) {
					nflag[j] = 2;
					nbcount++;
					nwcount--;//减去白列数
					count += m;//列的高度是行数
				}
			}
			else if (i == 0) {
				//m
				if (mflag[j]==0) {
					mflag[j] = 2;
					mbcount++;
					count += n - nbcount;//行的宽度是列数
				}
				else if (mflag[j] == 0) {
					mflag[j] = 2;
					mbcount++;
					mwcount--;//减去白行数
					count += n - nbcount;//行的宽度是列数
				}
			}
		}
		else if (c == 0) {
			//white
			if (i == 1) {
				//n
				if (nflag[j]==2) {
					nflag[j] = 1;
					count -= m;//减去这个列上黑子的数目，为行数
					nwcount++;
					nbcount--;//减去黑列数
				}
			}
			else if (i == 0) {
				//m
				if (mflag[j]==2) {
					mflag[j] = 1;
					count -= n;//减去这个行上黑子的数目，为列数
					mwcount++;
					mbcount--;//减去黑行数
				}
			}
		}
	}

	cout << count << endl;
	
	delete[] nflag;
	delete[] mflag;

	PAUSE;
	return 0;
}
