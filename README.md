# LuaSTG EX Plus  
  
下一代luastg版本：LuaSTG X [https://github.com/Xrysnow/LuaSTG-x]  
  
## 说明  
  
LuaSTG EX Plus 是 LuaSTG Plus 的衍生版本，其大部分代码基于9chu的 LuaSTG Plus ，并由ExboCooope、Xiliusha、Xrysnow、BAKAOLC进行维护。此衍生版本增加了更多的功能，以便开发者进行游戏开发。  
  
## 使用的第三方库  
  
### 此项目采用了以下第三方库：  
  
* lua file system(unkown version)(->luastg)  
* cjson to lua(unkown version)(->luastg)  
* luajit(2.0.5)(->luastg)  
* luasocket(2.0.1)(->luastg)  
* zlib(1.2.11)(->common)  
* libzip(unkown version)(->common)  
* libogg(1.3.3)(->common)  
* libvorbis(1.3.6)(->common)  
* freetype(2.10.0)(->common)  
* DirectShowClass(unkown version)(->fancy2d)  
* fancy2d(0.6)(->luastg)  
* xmath(unkown version)(->common)  
* Eyes2D(unkown version)(->common)  
* DirectX SDK (June 2010)(->common)  
  
### 需要自行下载编译的第三方库  
  
* libogg(1.3.3)(->common)  
* libvorbis(1.3.6)(->common)  
* freetype(2.10.0)(->common)  
* DirectX SDK (June 2010)(->common)  
  
## 工程构建  
  
部分第三方库需要仔细阅读文件夹内的readme.txt下载源代码并进行编译和一些配置。  
C++项目中，除了LuaSTG项目外，其他所有项目应当在Release配置下编译（因为其他所有项目的配置除了Release外，基本上是缺失的）。  
C#项目应该切换到Any CPU配置编译。  
  
