# LuaSTG EX Plus  

## 说明  

LuaSTG EX Plus 是 LuaSTG Plus 的衍生版本，其大部分代码基于9chu的 LuaSTG Plus ，并由ExboCooope、Xiliusha进行维护。此衍生版本增加了更多的功能，以便开发者进行游戏开发。  

## 使用的第三方库  

### 此项目采用了以下第三方库：  

1.lua file system(unkown version)(->luastg)  
2.cjson to lua(unkown version)(->luastg)  
3.luajit(2.0.5)(->luastg)  
4.steam API(unkown version)(->luastg)  
5.zlib(1.2.11)(->common)  
6.libogg(1.3.3)(->fancy2d)  
7.libvorbis(1.3.6)(->fancy2d)  
8.freetype(2.9.1)(->fancy2d)  
9.fancy2d(0.6)(->luastg)  

### ！除了fancy2d外，以上用到的项目的源代码、文档均完整地复制在本项目文件夹下，便于构建可执行文件。  

## 工程构建  

除了本项目内的文件，还需前往9chu的主页获取fancy2d，并放置在和项目根目录同级的位置：  

>LuaSTGExPlus  
>fancy2d  

然后需要构建fancy2d，fancy2d的构建方式请咨询9chu（其实我也不会）  
构建好fancy2d后，打开LuaSTGExPlus解决方案，构建zlib库  
luajit的生成文件已经在项目文件夹里生成好了，也可以自行下载其他版本的luajit使用  
最后一步就是生成LuaSTGPlus可执行文件
