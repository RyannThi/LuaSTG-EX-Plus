#include "LuaWrapper.hpp"

namespace LuaSTGPlus
{
	namespace LuaWrapper
	{
		void Register(lua_State* L)LNOEXCEPT {
			struct Function
			{
				static int Color(lua_State* L)LNOEXCEPT
				{
					fcyColor c;
					if (lua_gettop(L) == 1)
						c.argb = luaL_checknumber(L, 1);
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
