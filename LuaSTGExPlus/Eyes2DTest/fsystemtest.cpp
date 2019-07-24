#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <limits>

#include "E2DMainFunction.hpp"

#define E2DMAINRETURN std::system("pause"); return 0;
#define SAY(W) std::cout<<(W)<<std::endl;

using namespace std;
namespace fs = filesystem;

E2DMAIN{
for (auto& it : fs::directory_iterator("./")) {
	//SAY(it.path().string().c_str());
}
fs::path p("nmsl.jpg");
fs::directory_entry en(p);
if (en.is_regular_file() && fs::exists(p)) {
	SAY("YES");
}
fstream f("nmsl.jpg", ios::binary | ios::in);
if (f.is_open()) {
	SAY("YES");
}
//SAY(std::numeric_limits<float>::max());
E2DMAINRETURN }
