出于版权和版本库大小的考虑，本项目不再提供zlib的头文件和静态库文件。
需要自行前往zlib官网下载zlib，zlib是libzip的前置库
链接（zlib官方下载地址）：https://www.zlib.net/
为了方便项目的配置，我建议使用zlib-win-build
链接（zlib-win-build的GitHub项目地址）：https://github.com/kiyolee/zlib-win-build
下载好后，将压缩包内的文件解压到本项目的libz文件夹
进入build-VS2019-MT文件夹，打开zlib.sln，第一次打开时可能要按照提示更改Windows SDK版本和编译工具集
分别在Debug配置和Release配置下编译libz-static，得到静态库文件
创建build文件夹，将build-VS2019-MT文件夹内的Debug、Release文件夹复制到build文件夹内
此时，本项目的libz文件夹内应该有这样的结构：
libz----
    |
    |---build----
    |        |
    |        |---Debug（本文件夹内应该有libz-static.lib文件）
    |        |
    |        |---Release（本文件夹内应该有libz-static.lib文件）
    |        |
到这里就算配置完成了，以后基本上不会再动这个文件夹内的内容，除了日后升级zlib版本外
