#include <string>
#include <filesystem>
#include "Global.h"
#include "AppFrame.h"
#include "LuaWrapper.h"
#include "E2DCodePage.hpp"
#include "E2DFilePath.hpp"
#include "E2DFileManager.hpp"

using namespace std;
using namespace LuaSTGPlus;

void FileManagerWrapper::Register(lua_State* L)LNOEXCEPT {
	struct Wrapper {
		static int LoadArchive(lua_State* L) {
			// path ???
			int argn = lua_gettop(L);
			bool ret = false;
			switch (argn) {
			case 1:
				// path
				ret = LFMGR.LoadArchive(luaL_checkstring(L, 1), 0, nullptr);
				break;
			case 2:
				if (lua_isnumber(L, 2)) {
					// path lv
					ret = LFMGR.LoadArchive(luaL_checkstring(L, 1), luaL_checkinteger(L, 2), nullptr);
				}
				else {
					// path pw
					ret = LFMGR.LoadArchive(luaL_checkstring(L, 1), 0, luaL_checkstring(L, 2));
				}
				break;
			case 3:
				// path lv pw
				ret = LFMGR.LoadArchive(luaL_checkstring(L, 1), luaL_checkinteger(L, 2), luaL_checkstring(L, 3));
				break;
			}
			if (ret) {
				Eyes2D::IO::Archive* zip = LFMGR.GetArchive(luaL_checkstring(L, 1));
				if (zip != nullptr) {
					ArchiveWrapper::CreateAndPush(L, zip->GetUID());
				}
				else {
					lua_pushnil(L);
				}
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}
		static int UnloadArchive(lua_State* L) {
			LFMGR.UnloadArchive(luaL_checkstring(L, 1));
			return 0;
		}
		static int UnloadAllArchive(lua_State* L) {
			LFMGR.UnloadAllArchive();
			return 0;
		}
		static int ArchiveExist(lua_State* L) {
			lua_pushboolean(L,
				LFMGR.ArchiveExist(
					luaL_checkstring(L, 1)));
			return 1;
		}
		static int GetArchive(lua_State* L) {
			Eyes2D::IO::Archive* zip = LFMGR.GetArchive(luaL_checkstring(L, 1));
			if (zip != nullptr) {
				ArchiveWrapper::CreateAndPush(L, zip->GetUID());
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}
		static int EnumArchive(lua_State* L) {
			lua_newtable(L); // ??? t 
			for (unsigned int index = 1; index <= LFMGR.GetArchiveCount(); index++) {
				Eyes2D::IO::Archive* ref = LFMGR.GetArchive(index - 1);
				if (ref != nullptr) {
					lua_pushinteger(L, (lua_Integer)index); // ??? t index 
					lua_newtable(L); // ??? t index tt 
					lua_pushinteger(L, 1); // ??? t index tt 1 
					lua_pushstring(L, ref->GetArchivePath()); // ??? t index tt 1 s
					lua_settable(L, -3); // ??? t index tt 
					lua_pushinteger(L, 2); // ??? t index tt 2 
					lua_pushinteger(L, ref->GetPriority()); // ??? t index tt 2 priority 
					lua_settable(L, -3); // ??? t index tt 
					lua_settable(L, -3); // ??? t 
				}
			}
			return 1;
		}
		static int EnumFiles(lua_State* L) {
			string searchpath = luaL_checkstring(L, -1); // ??? path 
			Eyes2D::Platform::PathFormatLinux(searchpath);
			if ((searchpath == ".") || (searchpath == "/") || (searchpath == "./")) {
				searchpath = "";// "/" or "." 不需要
			}
			else if ((searchpath.size() > 1) && (searchpath.back() != '/')) {
				searchpath.push_back('/');//在搜索路径后面手动添加一个分隔符
			}
			lua_newtable(L); // ??? path t 
			filesystem::path path = searchpath;
			unsigned int index = 1;
			for (auto& p : filesystem::directory_iterator(path)) {
				if (filesystem::is_regular_file(p.path()) || filesystem::is_directory(p.path())) {
					lua_pushinteger(L, index); // ??? path t index 
					lua_newtable(L); // ??? path t index tt 
					lua_pushinteger(L, 1); // ??? path t index tt 1 
					string u8path = Eyes2D::String::UTF16ToUTF8(p.path().wstring());
					if (filesystem::is_directory(p.path())) {
						u8path.push_back('/');//在目录路径后面手动添加一个分隔符
					}
					lua_pushstring(L, u8path.c_str()); // ??? path t index tt 1 fpath 
					lua_settable(L, -3); // ??? path t index tt 
					lua_pushinteger(L, 2); // ??? path t index tt 2 
					lua_pushboolean(L, filesystem::is_directory(p.path())); // ??? path t index tt 2 bool //为目录时该项为真
					lua_settable(L, -3); // ??? path t index tt 
					lua_settable(L, -3); // ??? path t 
					index++;
				}
			}
			return 1;
		}
	};

	luaL_Reg tMethods[] =
	{
		{ "LoadArchive", &Wrapper::LoadArchive },
		{ "UnloadArchive", &Wrapper::UnloadArchive },
		{ "UnloadAllArchive", &Wrapper::UnloadAllArchive },
		{ "ArchiveExist", &Wrapper::ArchiveExist },
		{ "GetArchive", &Wrapper::GetArchive },
		{ "EnumArchive", &Wrapper::EnumArchive },
		{ "EnumFiles", &Wrapper::EnumFiles },
		{ NULL, NULL }
	};

	lua_getglobal(L, "lstg"); // ??? t 
	lua_newtable(L); // ??? t t
	::luaL_register(L, NULL, tMethods); // ??? t t 
	lua_setfield(L, -2, "FileManager"); // ??? t 
	lua_pop(L, 1); // ??? 
}
