出于版权和版本库大小的考虑，本项目不再提供libvorbis的头文件和静态库文件。
需要自行前往Xiph官网下载libvorbis，另外还要下载libogg
链接（Xiph官方下载地址）：https://xiph.org/downloads/
下载好后，将libvorbis压缩包内的文件解压到本项目的libvorbis文件夹
将libogg压缩包内include文件夹内的ogg文件夹解压到libvorbis文件夹的include文件夹内
此时，本项目的libvorbis文件夹内应该有这样的结构：
libvorbis----
      |
      |---include----
      |        |
      |        |---ogg
      |        |
      |        |---vorbis
      |        |
这样做是因为libvorbis编译时依赖libogg
进入win32\VS2010（可能是VS2015或者VS2017，使用最新版本）
打开vorbis_static.sln，第一次打开时可能要按照提示更改Windows SDK版本和编译工具集
更改运行库，/MD要改成/MT，/MDd要改成/MTd（C/C++ -> 代码生成 -> 运行库）
本项目不使用多线程DLL运行库，使用的是多线程运行库
分别在Debug配置和Release配置下进行编译，得到静态库文件
创建build文件夹，将生成的静态库文件复制到build文件夹内
此时，本项目的libvorbis文件夹内应该有这样的结构：
libvorbis----
      |
      |---build----
      |        |
      |        |---Debug（本文件夹内应该有libvorbis_static.lib和libvorbisfile_static.lib文件）
      |        |
      |        |---Release（本文件夹内应该有libvorbis_static.lib和libvorbisfile_static.lib文件）
      |        |
到这里就算配置完成了，以后基本上不会再动这个文件夹内的内容，除了日后升级libvorbis版本外
