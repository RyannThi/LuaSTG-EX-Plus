#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "E2DMainFunction.hpp"

#define E2DMAINRETURN std::system("pause"); return 0;
#define SAY(W) std::cout<<(W)<<std::endl;

using namespace std;
namespace fs = filesystem;

E2DMAIN{
for (auto& it : fs::directory_iterator("./Dir/")) {
	SAY(it.path().string().c_str());
}

E2DMAINRETURN }
