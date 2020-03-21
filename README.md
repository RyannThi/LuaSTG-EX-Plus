# LuaSTG EX Plus  
  
原 LuaSTG Plus 版本：LuaSTG Plus [https://github.com/9chu/LuaSTGPlus]  
下一代 LuaSTG X 版本：LuaSTG-X [https://github.com/Xrysnow/LuaSTG-x]  
  
## 说明  
  
LuaSTG EX Plus 是 LuaSTG Plus 的衍生版本，其大部分代码基于9chu的 LuaSTG Plus ，  
并由ExboCooope、Xiliusha、Xrysnow、BAKAOLC进行维护。此衍生版本增加了更多的功能，  
以便开发者进行游戏开发。  
  
## 使用的第三方库  
  
* lua file system
* cjson to lua
* luajit
* luasocket
* zlib
* libzip
* libogg
* libvorbis
* freetype
* DirectShowClass
* fancy2d
* xmath
* Eyes2D
* DirectX SDK (June 2010)
  
## 工程构建  
  
lua file system、cjson for lua、lua socket这三个第三方库，  
需要前往其项目页面下载源代码回来，复制到本项目对应文件夹内。  
（可参考luacjson、luafilesystem、luasocket文件夹下的readme.txt文件）  
  
剩下的第三方库在Release页面有下载，解压到对应的位置即可。  
（prebuild.zip，解压到LuaSTGExPlus文件夹下）  
  
**不要问我为什么要这样做，因为C/C++的库管理功能太屎了，我为了**  
**偷懒不得不这样做，否则光是库管理都得要了我的命。**  
  
除了LuaSTG项目外，其他所有项目应当在Release、Debug配置下编译。  
（因为其他所有项目的Dev配置是缺失的）  
