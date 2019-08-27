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
		RandomizerWrapper::Register(L);  // 随机数发生器
		BentLaserDataWrapper::Register(L);  // 曲线激光
		Fancy2dStopWatchWrapper::Register(L);  // 高精度停表
		BuiltInFunctionWrapper::Register(L);  // 内建函数库
		FileManagerWrapper::Register(L); //内建函数库，文件资源管理，请确保位于内建函数库后加载
		ArchiveWrapper::Register(L); //压缩包
		GameResourceWrapper::Register(L);  // 游戏资源对象
		XInputManagerWrapper::Register(L);  //内建函数库，XInput，请确保位于内建函数库后加载
#ifdef USING_ADVANCE_COLLIDER
		GameObjectColliderWrapper::Register(L);//Collider
#endif // USING_ADVANCE_COLLIDER
	}
}
