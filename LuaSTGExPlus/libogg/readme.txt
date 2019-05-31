出于版权和版本库大小的考虑，本项目不再提供libogg的头文件和静态库文件。
需要自行前往Xiph官网下载libogg
链接（Xiph官方下载地址）：https://xiph.org/downloads/
下载好后，将压缩包内的文件解压到本项目的libogg文件夹
进入win32\VS2015（可能是VS2010或者VS2017，视实际情况）
打开libogg_static.sln，第一次打开时可能要按照提示更改Windows SDK版本和编译工具集
分别在Debug配置和Release配置下进行编译，得到静态库文件
创建build文件夹，将生成的静态库文件复制到build文件夹内
此时，本项目的libogg文件夹内应该有这样的结构：
libogg----
      |
      |---build----
      |        |
      |        |---Debug（本文件夹内应该有libogg_static.lib文件）
      |        |
      |        |---Release（本文件夹内应该有libogg_static.lib文件）
      |        |
到这里就算配置完成了，以后基本上不会再动这个文件夹内的内容，除了日后升级libogg版本外
