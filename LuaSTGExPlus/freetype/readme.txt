出于版权和版本库大小的考虑，本项目不再提供freetype的头文件和静态库文件。
需要自行前往freetype官网下载freetype2（注意是freetype2）
链接（freetype官方下载地址）：https://www.freetype.org/download.html
下载好后，将压缩包内的文件解压到本项目的freetype文件夹
进入builds\windows\vc2010（可能是vs2010、vs2015或vs2017，视实际情况）
打开freetype.sln，根据实际情况更改Windows SDK版本和编译工具集
分别在Debug Static配置和Release Static配置下进行编译，得到静态库文件
创建build文件夹，将生成的静态库文件复制到build文件夹内
此时，本项目的freetype文件夹内应该有这样的结构：
freetype----
      |
      |---build----
      |        |
      |        |---Debug（本文件夹内应该有freetype.lib文件）
      |        |
      |        |---Release（本文件夹内应该有freetype.lib文件）
      |        |
到这里就算配置完成了，以后基本上不会再动这个文件夹内的内容，除了日后升级freetype版本外
