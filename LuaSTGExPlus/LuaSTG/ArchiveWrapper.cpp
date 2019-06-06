#include <string>
#include <string_view>
#include "Global.h"
#include "LuaWrapper.h"
#include "E2DFileManager.hpp"

using namespace std;
using namespace LuaSTGPlus;
using namespace Eyes2D::IO;

struct ArchiveWrapper::Wrapper {
	Archive* handle;
};

void ArchiveWrapper::Register(lua_State* L)LNOEXCEPT {
	struct Function {
		static int EnumFiles(lua_State* L) {
			// ??? self searchpath
			ArchiveWrapper::Wrapper* p = static_cast<ArchiveWrapper::Wrapper*>(luaL_checkudata(L, -2, LUASTG_LUA_TYPENAME_ARCHIVE));
			string_view frompath = luaL_checkstring(L, -1);
			lua_newtable(L);// ??? self searchpath t 
			Archive& z = *(p->handle);
			int i = 1;
			for (long long index = 0; index < z.GetFileCount(); index++) {
				string topath = z.GetFileName(index);
				if (frompath.size() > topath.size()) {
					continue;
				}
				else {
					string_view path(&topath[0], frompath.size());
					if (path == frompath) {
						lua_pushinteger(L, i);// ??? self searchpath t i 
						lua_newtable(L);// ??? self searchpath t i tt 
						lua_pushinteger(L, 1);// ??? self searchpath t i tt 1 
						lua_pushstring(L, topath.c_str());// ??? self searchpath t i tt 1 s 
						lua_settable(L, -3);// ??? self searchpath t i tt 
						lua_pushinteger(L, 2);// ??? self searchpath t i tt 2 
						lua_pushboolean(L, *(topath.end()) == '/');// ??? self searchpath t i tt 2 bool 
						lua_settable(L, -3);// ??? self searchpath t i tt 
						lua_settable(L, -3);// ??? self searchpath t 

						i++;
					}
				}
			}
			return 1;
		}
		static int FileExist(lua_State* L) {
			// ??? self path
			ArchiveWrapper::Wrapper* p = static_cast<ArchiveWrapper::Wrapper*>(luaL_checkudata(L, -2, LUASTG_LUA_TYPENAME_ARCHIVE));
			lua_pushboolean(L, p->handle->FileExist(luaL_checkstring(L, -1)));
			return 1;
		}
		static int GetName(lua_State* L) {
			// ??? self
			ArchiveWrapper::Wrapper* p = static_cast<ArchiveWrapper::Wrapper*>(luaL_checkudata(L, -2, LUASTG_LUA_TYPENAME_ARCHIVE));
			lua_pushstring(L, p->handle->GetArchivePath());
			return 1;
		}
	};
	

}

Archive* ArchiveWrapper::CreateAndPush(lua_State* L)LNOEXCEPT {
}

void CreateAndPush(lua_State* L, Archive* archive)LNOEXCEPT {
}
