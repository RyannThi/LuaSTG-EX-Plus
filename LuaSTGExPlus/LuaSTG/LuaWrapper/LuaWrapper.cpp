#include "LuaWrapper.hpp"
#include "../Utility.h"

namespace LuaSTGPlus
{
	namespace LuaWrapper
	{
		void Register(lua_State* L)LNOEXCEPT {
			struct Function
			{
				static int GetLocalAppDataPath(lua_State* L)LNOEXCEPT
				{
					try {
						std::string path = fcyStringHelper::WideCharToMultiByte(LuaSTGPlus::GetLocalAppDataPath(), CP_UTF8);
						lua_pushstring(L, path.c_str());
					}
					catch (const std::bad_alloc&) {
						lua_pushstring(L, "");
					}
					return 1;
				}
				static int GetRoamingAppDataPath(lua_State* L)LNOEXCEPT
				{
					try {
						std::string path = fcyStringHelper::WideCharToMultiByte(LuaSTGPlus::GetRoamingAppDataPath(), CP_UTF8);
						lua_pushstring(L, path.c_str());
					}
					catch (const std::bad_alloc&) {
						lua_pushstring(L, "");
					}
					return 1;
				}

				static int ANSIToUTF8(lua_State* L)LNOEXCEPT {
					try {
						std::string fromstring = luaL_checkstring(L, 1);
						std::wstring tempstring = fcyStringHelper::MultiByteToWideChar(fromstring, CP_ACP);
						std::string tostring = fcyStringHelper::WideCharToMultiByte(tempstring, CP_UTF8);
						lua_pushstring(L, tostring.c_str());
					}
					catch (const std::bad_alloc&) {
						lua_pushnil(L);
					}
					return 1;
				}
				static int UTF8ToANSI(lua_State* L)LNOEXCEPT {
					try {
						std::string fromstring = luaL_checkstring(L, 1);
						std::wstring tempstring = fcyStringHelper::MultiByteToWideChar(fromstring, CP_UTF8);
						std::string tostring = fcyStringHelper::WideCharToMultiByte(tempstring, CP_ACP);
						lua_pushstring(L, tostring.c_str());
					}
					catch (const std::bad_alloc&) {
						lua_pushnil(L);
					}
					return 1;
				}
				
				static int Color(lua_State* L)LNOEXCEPT
				{
					fcyColor c;
					if (lua_gettop(L) == 1)
						c.argb = (fuInt)luaL_checknumber(L, 1);
					else
					{
						c = fcyColor(
							luaL_checkinteger(L, 1),
							luaL_checkinteger(L, 2),
							luaL_checkinteger(L, 3),
							luaL_checkinteger(L, 4)
						);
					}
					ColorWrapper::CreateAndPush(L, c);
					return 1;
				}
			};

			luaL_Reg tMethod[] =
			{
				{ "GetLocalAppDataPath", &Function::GetLocalAppDataPath },
				{ "GetRoamingAppDataPath", &Function::GetRoamingAppDataPath },

				{ "ANSIToUTF8", &Function::ANSIToUTF8 },
				{ "UTF8ToANSI", &Function::UTF8ToANSI },
				
				{ "Color", &Function::Color },
				{ NULL, NULL }
			};

			lua_newtable(L);						// ... t
			ColorWrapper::Register(L);
			IO::Register(L);
			::luaL_register(L, NULL, tMethod);
			::lua_setglobal(L, LUASTG_LUA_LIBNAME);	// ...
		}
	}

	void RegistBuiltInClassWrapper(lua_State* L)LNOEXCEPT {
		LuaWrapper::Register(L);
		RandomizerWrapper::Register(L);  // �����������
		BentLaserDataWrapper::Register(L);  // ���߼���
		Fancy2dStopWatchWrapper::Register(L);  // �߾���ͣ��
		BuiltInFunctionWrapper::Register(L);  // �ڽ�������
		FileManagerWrapper::Register(L); //�ڽ������⣬�ļ���Դ������ȷ��λ���ڽ�����������
		ArchiveWrapper::Register(L); //ѹ����
		GameResourceWrapper::Register(L);  // ��Ϸ��Դ����
		XInputManagerWrapper::Register(L);  //�ڽ������⣬XInput����ȷ��λ���ڽ�����������
#ifdef USING_ADVANCE_COLLIDER
		GameObjectColliderWrapper::Register(L);//Collider
#endif // USING_ADVANCE_COLLIDER
	}
}
