# LuaSTG EX Plus  

## 警告  
**随着一些功能的更新，luastg ex+越来越臃肿、性能低下，而且其只能单核单线程运算，远不及其他游戏引擎，继续维护下去已经没有意义。因此该分支暂停维护，不再更新**。  
下一代luastg版本：LuaSTG X [https://github.com/Xrysnow/LuaSTG-x]  
**plus已死，x当立！！！**  

## 说明  

LuaSTG EX Plus 是 LuaSTG Plus 的衍生版本，其大部分代码基于9chu的 LuaSTG Plus ，并由ExboCooope、Xiliusha、Xrysnow、BAKAOLC进行维护。此衍生版本增加了更多的功能，以便开发者进行游戏开发。  

## 使用的第三方库  

### 此项目采用了以下第三方库：  

1.lua file system(unkown version)(->luastg)  
2.cjson to lua(unkown version)(->luastg)  
3.luajit(2.0.5)(->luastg)  
4.【数据删除】  
5.zlib(1.2.11)(->common)  
6.libogg(1.3.3)(->fancy2d)  
7.libvorbis(1.3.6)(->fancy2d)  
8.freetype(2.9.1)(->fancy2d)  
9.DirectShowClass(unkown version)(->fancy2d)  
10.fancy2d(0.6)(->luastg)  
11.lstgx_Math(unkown version)(->luastg)  
12.LuaSocket(2.0.1)(->luastg)  
13.Eyes2DLib(unkown version)(->luastg)  

### 以上用到的项目的源代码、文档均完整地复制在本项目文件夹下，便于构建可执行文件。  

## 工程构建  

本人已经生成好了所需的lib和dll文件，便于直接编译luastg  
如果需要自己编译，需要注意除了LuaSTG外，全部项目应当在release配置下编译（因为其他配置是缺失的）  
可以按照以下顺序编译：  

1.zlib  
2.libogg  
3.libvorbis  
4.freetype  
5.DirectShowClass  
6.fancy2d(fancylib)  
7.fancy2d(fancy2d)  
8.luastg  
