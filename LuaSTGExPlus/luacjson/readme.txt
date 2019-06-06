请到：https://github.com/mpx/lua-cjson 下载lua cjson项目源代码并解压到本文件夹内

如果编译出错，请手动到lua_cjson.c里面添加以下代码：

#ifdef _MSC_VER
#define strcasecmp stricmp
#define strncasecmp  strnicmp
#endif
