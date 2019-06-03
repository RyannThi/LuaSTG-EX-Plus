#include <iostream>
#include <string>
#include "E2DMainFunction.hpp"
#define E2DMAINRETURN std::system("pause"); return 0;
#define SAY(W) std::cout<<(W)<<std::endl;

#include "E2DFilePath.hpp"
#include "E2DLogSystem.hpp"

using namespace std;
using namespace Eyes2D;

E2DMAIN{
	char s1[] = "/root/LuaSTG.txt";
	string s2 = "/root/9961L.txt";

	PathFormatWin32LowCase(s1, sizeof(s1));
	PathFormatWin32LowCase(s2);

	EYESINFO(s1);
	EYESINFO(s2.c_str());

	E2DMAINRETURN;
}
