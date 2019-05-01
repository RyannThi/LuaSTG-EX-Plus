#include "LuaWrapper.h"
#include "GameObjectPool.h"

using namespace LuaSTGPlus;

#ifdef USING_ADVANCE_COLLIDER

void GameObjectColliderWrapper::Register(lua_State* L)LNOEXCEPT {
	struct WrapperImplement
	{
		static int AddCollider(lua_State* L)LNOEXCEPT
		{
			// self, type, a, b // id
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			GameObjectCollider* collider = p->handle->collider;
			int itype = luaL_optinteger(L, 2, 2);
			float fa = (float)luaL_optnumber(L, 3, 0.0);
			float fb = (float)luaL_optnumber(L, 4, 0.0);

			//查询得到最大id，获得链表尾
			GameObjectCollider* plast = collider;
			int id = 0;
			{
				GameObjectCollider* pret = collider->next;
				while (pret != nullptr) {
					id = (pret->id > id) ? pret->id : id;
					plast = pret;
					pret = pret->next;
				}
				id = id + 1;
			}

			//转换类型
			GameObjectColliderType enumtype;
			switch (itype)
			{
			case 0:
				enumtype = GameObjectColliderType::Circle;
				break;
			case 1:
				enumtype = GameObjectColliderType::OBB;
				break;
			case 2:
				enumtype = GameObjectColliderType::Ellipse;
				break;
			case 3:
				enumtype = GameObjectColliderType::Diamond;
				break;
			case 4:
				enumtype = GameObjectColliderType::Triangle;
				break;
			case 5:
				enumtype = GameObjectColliderType::Point;
				break;
			default:
				return luaL_error(L, "Invalid collider type.");
				break;
			}

			//插入
			GameObjectCollider* newcollider = new GameObjectCollider();
			newcollider->reset();
			newcollider->type = enumtype;
			newcollider->a = fa;
			newcollider->b = fb;
			plast->next = newcollider;
			newcollider->last = plast;

			lua_pushinteger(L, id);
			return 1;
		}
		static int DelCollider(lua_State* L) {
			// self, id // success
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			GameObjectCollider* collider = p->handle->collider;
			int id = luaL_checkinteger(L, 2);
			if (id == 0) {
				return luaL_error(L, "The default collider (id = 0) is not allowed to be removed.");
			}

			//找到需要的
			GameObjectCollider* pfind = collider->next;
			while (pfind != nullptr) {
				if (pfind->id == id) {
					break;
				}
				else {
					pfind = pfind->next;
				}
			}
			
			//处理
			if (pfind == nullptr) {
				lua_pushboolean(L, false);
			}
			else {
				pfind->last->next = pfind->next;
				if (pfind->next != nullptr) {
					pfind->next->last = pfind->last;
				}
				if (p->cur != nullptr) {//检查cur是否需要清理
					if (p->cur->id == id) {
						p->cur = nullptr;
					}
				}
				delete pfind;
				lua_pushboolean(L, true);
			}

			return 1;
		}
		static int SetCurCollider(lua_State* L) {
			// self, id|nil // success
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			GameObjectCollider* collider = p->handle->collider;
			int id = luaL_optinteger(L, 2, 0);
			
			//id为0直接处理
			if (id == 0) {
				p->cur = collider;
				lua_pushboolean(L, true);
				return 1;
			}

			//找到需要的
			GameObjectCollider* pfind = collider->next;
			while (pfind != nullptr) {
				if (pfind->id == id) {
					break;
				}
				else {
					pfind = pfind->next;
				}
			}

			//设置
			if (pfind == nullptr) {
				lua_pushboolean(L, false);
			}
			else {
				p->cur = pfind;
				lua_pushboolean(L, true);
			}
			
			return 1;
		}
		static int EnumColliders(lua_State* L) {
			// self
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			GameObjectCollider* collider = p->handle->collider;
			
			lua_newtable(L);// self, t
			int pos = 1;
			while (collider != nullptr) {
				lua_pushinteger(L, collider->id);// self, t, id
				lua_rawseti(L, -2, pos);// self, t
				pos = pos + 1;
				collider = collider->next;
			}
			// self, t

			return 1;
		}

		static int Meta_NewIndex(lua_State* L)LNOEXCEPT
		{
			// t, k, v
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			if (p->cur == nullptr) {
				return luaL_error(L, "The current collider is not specified.");
			}
			else {
				std::string key = luaL_checkstring(L, 2);
				if (key == "a") {
					p->cur->a = (float)luaL_checknumber(L, 3);
					p->cur->calcircum();
				}
				else if (key == "b") {
					p->cur->b = (float)luaL_checknumber(L, 3);
					p->cur->calcircum();
				}
				else if (key == "type") {
					int itype = luaL_checkinteger(L, 3);
					GameObjectColliderType enumtype;
					switch (itype)
					{
					case 0:
						enumtype = GameObjectColliderType::Circle;
						break;
					case 1:
						enumtype = GameObjectColliderType::OBB;
						break;
					case 2:
						enumtype = GameObjectColliderType::Ellipse;
						break;
					case 3:
						enumtype = GameObjectColliderType::Diamond;
						break;
					case 4:
						enumtype = GameObjectColliderType::Triangle;
						break;
					case 5:
						enumtype = GameObjectColliderType::Point;
						break;
					default:
						return luaL_error(L, "Invalid collider type.");
						break;
					}
					p->cur->type = enumtype;
					p->cur->calcircum();
				}
				else if (key == "x") {
					p->cur->dx = (float)luaL_checknumber(L, 3);
				}
				else if (key == "y") {
					p->cur->dy = (float)luaL_checknumber(L, 3);
				}
				else if (key == "rot") {
					p->cur->rot = (float)(luaL_checknumber(L, 3) * LDEGREE2RAD);
				}
				else if (key == "id") {
					return luaL_error(L, "The attribute 'id' is read-only.");
				}
				else {
					return luaL_error(L, "New members are not allowed to be added.");
				}
			}
			return 0;
		}
		static int Meta_Index(lua_State* L)LNOEXCEPT
		{
			// t, k //v
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			if (p->cur == nullptr) {
				return luaL_error(L, "The current collider is not specified.");
			}
			else {
				std::string key = luaL_checkstring(L, 2);
				if (key == "a") {
					lua_pushnumber(L, p->cur->a);
				}
				else if (key == "b") {
					lua_pushnumber(L, p->cur->b);
				}
				else if (key == "type") {
					int _type = (int)p->cur->type;
					lua_pushinteger(L, _type);
				}
				else if (key == "x") {
					lua_pushnumber(L, p->cur->dx);
				}
				else if (key == "y") {
					lua_pushnumber(L, p->cur->dy);
				}
				else if (key == "rot") {
					double _rot = (double)p->cur->rot;
					lua_pushnumber(L, _rot * LRAD2DEGREE);
				}
				else if (key == "id") {
					lua_pushinteger(L, p->cur->id);
				}
				else {
					return luaL_error(L, "Attempts to access nonexistent members.");
				}
			}
			return 1;
		}
		static int Meta_ToString(lua_State* L)LNOEXCEPT
		{
			Wrapper* p = static_cast<Wrapper*>(luaL_checkudata(L, 1, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER));
			lua_pushfstring(L, "lstg.GameObjectColliderManager object");
			return 1;
		}
	};
	
	luaL_Reg tMethods[] =
	{
		{ "AddCollider", &WrapperImplement::AddCollider },
		{ "DelCollider", &WrapperImplement::DelCollider },
		{ "SetCurCollider", &WrapperImplement::SetCurCollider },
		{ "EnumColliders", &WrapperImplement::EnumColliders },
		{ NULL, NULL }
	};
	luaL_Reg tMetaTable[] =
	{
		{ "__tostring", &WrapperImplement::Meta_ToString },
		{ "__newindex", &WrapperImplement::Meta_NewIndex },
		{ "__index", &WrapperImplement::Meta_Index },
		{ NULL, NULL }
	};
	
	luaL_openlib(L, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER, tMethods, 0);  // t
	luaL_newmetatable(L, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER);  // t mt
	luaL_openlib(L, 0, tMetaTable, 0);  // t mt
	lua_pushliteral(L, "__index");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__index"] = t)
	lua_pushliteral(L, "__metatable");  // t mt s
	lua_pushvalue(L, -3);  // t mt s t
	lua_rawset(L, -3);  // t mt (mt["__metatable"] = t)  保护metatable不被修改
	lua_pop(L, 2);
}

void GameObjectColliderWrapper::CreateAndPush(lua_State* L, GameObject* obj)LNOEXCEPT {
	Wrapper* p = static_cast<Wrapper*>(lua_newuserdata(L, sizeof(Wrapper)));// udata
	p->handle = obj;
	p->cur = obj->collider;
	luaL_getmetatable(L, LUASTG_LUA_TYPENAME_COLLIDERWRAPPER);//udata, mt
	lua_setmetatable(L, -2);//udata
}

#endif // USING_ADVANCE_COLLIDER
