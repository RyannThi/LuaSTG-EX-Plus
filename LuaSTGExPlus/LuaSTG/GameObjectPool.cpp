#include "GameObjectPool.h"
#include "GameObjectPropertyHash.inl"
#include "LuaStringToEnum.hpp"
#include "AppFrame.h"
#include "CollisionDetect.h"
#include "LuaWrapper.h"

#define METATABLE_OBJ "mt"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define GETOBJTABLE \
	do { \
		lua_pushlightuserdata(L, (void*)&LAPP); \
		lua_gettable(L, LUA_REGISTRYINDEX); \
	} while (false)

#define LIST_INSERT_BEFORE(target, p, field) \
	do { \
		p->p##field##Prev = (target)->p##field##Prev; \
		p->p##field##Next = (target); \
		p->p##field##Prev->p##field##Next = p; \
		p->p##field##Next->p##field##Prev = p; \
	} while(false)

#define LIST_INSERT_AFTER(target, p, field) \
	do { \
		p->p##field##Prev = (target); \
		p->p##field##Next = (target)->p##field##Next; \
		p->p##field##Prev->p##field##Next = p; \
		p->p##field##Next->p##field##Prev = p; \
	} while(false)

#define LIST_REMOVE(p, field) \
	do { \
		p->p##field##Prev->p##field##Next = p->p##field##Next; \
		p->p##field##Next->p##field##Prev = p->p##field##Prev; \
	} while(false)

#define LIST_INSERT_SORT(p, field, func) \
	do { \
		if (p->p##field##Next->p##field##Next && func(p->p##field##Next, p)) \
		{ \
			GameObject* pInsertBefore = p->p##field##Next->p##field##Next; \
			while (pInsertBefore->p##field##Next && func(pInsertBefore, p)) \
				pInsertBefore = pInsertBefore->p##field##Next; \
			LIST_REMOVE(p, field); \
			LIST_INSERT_BEFORE(pInsertBefore, p, field); \
		} \
		else if (p->p##field##Prev->p##field##Prev && func(p, p->p##field##Prev)) \
		{ \
			GameObject* pInsertAfter = p->p##field##Prev->p##field##Prev; \
			while (pInsertAfter->p##field##Prev && func(p, pInsertAfter)) \
				pInsertAfter = pInsertAfter->p##field##Prev; \
			LIST_REMOVE(p, field); \
			LIST_INSERT_AFTER(pInsertAfter, p, field); \
		} \
	} while (false)

using namespace std;
using namespace LuaSTGPlus;

static inline bool ObjectListSortFunc(GameObject* p1, GameObject* p2)LNOEXCEPT
{
	// 总是以uid为参照
	return p1->uid < p2->uid;
}

static inline bool RenderListSortFunc(GameObject* p1, GameObject* p2)LNOEXCEPT
{
	// layer小的靠前。若layer相同则参照uid。
	return (p1->layer < p2->layer) || ((p1->layer == p2->layer) && (p1->uid < p2->uid));
}

////////////////////////////////////////////////////////////////////////////////
/// GameObject
////////////////////////////////////////////////////////////////////////////////
#pragma region GameObject
// TODO:collider //ok
bool GameObject::ChangeResource(const char* res_name)
{
	//LASSERT(!res);

	fcyRefPointer<ResSprite> tSprite = LRES.FindSprite(res_name);
	if (tSprite)
	{
		res = tSprite;
		res->AddRef();
#ifdef USING_ADVANCE_COLLIDER
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		colliders[0].a = tSprite->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		colliders[0].b = tSprite->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		colliders[0].a = tSprite->GetHalfSizeX();
		colliders[0].b = tSprite->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		if (tSprite->IsRectangle()) {
			colliders[0].type = GameObjectColliderType::OBB;
		}
		else {
			colliders[0].type = GameObjectColliderType::Ellipse;
		}
		colliders[0].calcircum();
#else
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		a = tSprite->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tSprite->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		a = tSprite->GetHalfSizeX();
		b = tSprite->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		rect = tSprite->IsRectangle();
		UpdateCollisionCirclrRadius();
#endif // USING_ADVANCE_COLLIDER
		return true;
	}
	
	fcyRefPointer<ResAnimation> tAnimation = LRES.FindAnimation(res_name);
	if (tAnimation)
	{
		res = tAnimation;
		res->AddRef();
#ifdef USING_ADVANCE_COLLIDER
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		colliders[0].a = tAnimation->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		colliders[0].b = tAnimation->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		colliders[0].a = tAnimation->GetHalfSizeX();
		colliders[0].b = tAnimation->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		if (tAnimation->IsRectangle()) {
			colliders[0].type = GameObjectColliderType::OBB;
		}
		else {
			colliders[0].type = GameObjectColliderType::Ellipse;
		}
		colliders[0].calcircum();
#else
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		a = tAnimation->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tAnimation->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		a = tAnimation->GetHalfSizeX();
		b = tAnimation->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		rect = tAnimation->IsRectangle();
		UpdateCollisionCirclrRadius();
#endif // USING_ADVANCE_COLLIDER
		return true;
	}

	fcyRefPointer<ResParticle> tParticle = LRES.FindParticle(res_name);
	if(tParticle)
	{
		res = tParticle;
		if (!(ps = tParticle->AllocInstance()))
		{
			res = nullptr;
			LERROR("无法构造粒子池，内存不足");
			return false;
		}
		ps->SetInactive();
		ps->SetCenter(fcyVec2((float)x, (float)y));
		ps->SetRotation((float)rot);
		ps->SetActive();

		res->AddRef();
#ifdef USING_ADVANCE_COLLIDER
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		colliders[0].a = tParticle->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		colliders[0].b = tParticle->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		colliders[0].a = tParticle->GetHalfSizeX();
		colliders[0].b = tParticle->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		if (tParticle->IsRectangle()) {
			colliders[0].type = GameObjectColliderType::OBB;
		}
		else {
			colliders[0].type = GameObjectColliderType::Ellipse;
		}
		colliders[0].calcircum();
#else
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		a = tParticle->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tParticle->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
		a = tParticle->GetHalfSizeX();
		b = tParticle->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
		rect = tParticle->IsRectangle();
		UpdateCollisionCirclrRadius();
#endif // USING_ADVANCE_COLLIDER
		return true;
	}

	return false;
}

#pragma endregion

#pragma region GameObjectPool

GameObjectPool::GameObjectPool(lua_State* pL)
{
	// Lua_State
	L = pL;
	
	// 初始化对象链表
	/*
	memset(&m_pObjectListHeader, 0, sizeof(GameObject));
	memset(&m_pRenderListHeader, 0, sizeof(GameObject));
	memset(m_pCollisionListHeader, 0, sizeof(m_pCollisionListHeader));
	memset(&m_pObjectListTail, 0, sizeof(GameObject));
	memset(&m_pRenderListTail, 0, sizeof(GameObject));
	memset(m_pCollisionListTail, 0, sizeof(m_pCollisionListTail));
	m_pObjectListHeader.pObjectNext = &m_pObjectListTail;
	m_pObjectListHeader.uid = numeric_limits<uint64_t>::min();
	m_pObjectListTail.pObjectPrev = &m_pObjectListHeader;
	m_pObjectListTail.uid = numeric_limits<uint64_t>::max();
	m_pRenderListHeader.pRenderNext = &m_pRenderListTail;
	m_pRenderListHeader.uid = numeric_limits<uint64_t>::min();
	m_pRenderListHeader.layer = numeric_limits<lua_Number>::min();
	m_pRenderListTail.pRenderPrev = &m_pRenderListHeader;
	m_pRenderListTail.uid = numeric_limits<uint64_t>::max();
	m_pRenderListTail.layer = numeric_limits<lua_Number>::max();
	for (size_t i = 0; i < LGOBJ_GROUPCNT; ++i)
	{
		m_pCollisionListHeader[i].pCollisionNext = &m_pCollisionListTail[i];
		m_pCollisionListTail[i].pCollisionPrev = &m_pCollisionListHeader[i];
	}
	//*/
	m_UpdateList.clear();
	m_RenderList.clear();
	for (auto& i : m_ColliList) {
		i.clear();
	}

	// 创建一个全局表用于存放所有对象
	lua_pushlightuserdata(L, (void*)&LAPP);  // p(使用APP实例指针作键用以防止用户访问)
	lua_createtable(L, LGOBJ_MAXCNT, 0);  // p t(创建足够大的table用于存放所有的游戏对象在lua中的对应对象)

	// 取出lstg.GetAttr和lstg.SetAttr创建元表
	lua_newtable(L);  // ... t
	lua_getglobal(L, "lstg");  // ... t t
	lua_pushstring(L, "GetAttr");  // ... t t s
	lua_gettable(L, -2);  // ... t t f(GetAttr)
	lua_pushstring(L, "SetAttr");  // ... t t f(GetAttr) s
	lua_gettable(L, -3);  // ... t t f(GetAttr) f(SetAttr)
	LASSERT(lua_iscfunction(L, -1) && lua_iscfunction(L, -2));
	lua_setfield(L, -4, "__newindex");  // ... t t f(GetAttr)
	lua_setfield(L, -3, "__index");  // ... t t
	lua_pop(L, 1);  // ... t(将被用作元表)
	
	// 保存元表到 register[app][mt]
	lua_setfield(L, -2, METATABLE_OBJ);  // p t
	lua_settable(L, LUA_REGISTRYINDEX);

	m_pCurrentObject = nullptr;
	m_superpause = 0;
	m_nextsuperpause = 0;
}

GameObjectPool::~GameObjectPool()
{
	ResetPool();
}

GameObject* GameObjectPool::_AllocObject() {
	size_t id = 0;
	if (!m_ObjectPool.Alloc(id)) {
		return nullptr;
	}
	GameObject* p = m_ObjectPool.Data(id);
	p->Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = m_iUid;
	m_iUid++;
#ifdef USING_MULTI_GAME_WORLD
	if (m_pCurrentObject) {
		p->world = m_pCurrentObject->world;
	}
#endif // USING_MULTI_GAME_WORLD
	m_UpdateList.insert(p);
	m_RenderList.insert(p);
	m_ColliList[p->group].insert(p);
	return p;
}

GameObject* GameObjectPool::_ReleaseObject(GameObject* object) {
	GameObject* ret = nullptr;
	{
		auto it = m_UpdateList.find(object);
		ret = *std::next(it);
	}
	m_UpdateList.erase(object);
	m_RenderList.erase(object);
	m_ColliList[object->group].erase(object);
	if (m_pCurrentObject == object) {
		m_pCurrentObject = nullptr;
	}
	object->status = STATUS_FREE;
	m_ObjectPool.Free(object->id);
	return ret;
}

void GameObjectPool::_SetObjectLayer(GameObject* object, lua_Number layer) {
	if (object->layer != layer) {
		m_RenderList.erase(object);
		object->layer = layer;
		m_RenderList.insert(object);
	}
}

void GameObjectPool::_SetObjectColliGroup(GameObject* object, lua_Integer group) {
	if (object->group != group) {
		m_ColliList[object->group].erase(object);
		object->group = group;
		m_ColliList[group].insert(object);
	}
}

bool GameObjectPool::_ObjectBoundCheck(GameObject* object) noexcept {
	return !(
		object->bound &&
		(	
			object->x < m_BoundLeft ||
			object->x > m_BoundRight ||
			object->y < m_BoundBottom ||
			object->y > m_BoundTop
		)
	);
}

GameObject* GameObjectPool::freeObject(GameObject* p)LNOEXCEPT
{
	/*
	GameObject* pRet = p->pObjectNext;

	if (m_pCurrentObject == p){
		m_pCurrentObject = NULL;
	}

	// 从对象链表移除
	LIST_REMOVE(p, Object);

	// 从渲染链表移除
	LIST_REMOVE(p, Render);

	// 从碰撞链表移除
	LIST_REMOVE(p, Collision);

	// 删除lua对象表中元素
	GETOBJTABLE;  // ot
	lua_pushnil(L);  // ot nil
	lua_rawseti(L, -2, p->id + 1);  // ot
	lua_pop(L, 1);

	// 释放引用的资源
	p->ReleaseResource();

	// 回收到对象池
	m_ObjectPool.Free(p->id);
	//*/

	// 删除lua对象表中元素
	GETOBJTABLE;  // ot
	lua_pushnil(L);  // ot nil
	lua_rawseti(L, -2, p->id + 1);  // ot
	lua_pop(L, 1);

	// 释放引用的资源
	p->ReleaseResource();

	GameObject* pRet = _ReleaseObject(p);

	return pRet;
}

void GameObjectPool::DoFrame()LNOEXCEPT
{
	//处理超级暂停
	GETOBJTABLE;  // ot
	int superpause = UpdateSuperPause();

	/*
	GameObject* p = m_pObjectListHeader.pObjectNext;
	lua_Number cache1, cache2;//速度限制计算时用到的中间变量
	while (p && p != &m_pObjectListTail)
	{
		// 根据id获取对象的lua绑定table、拿到class再拿到framefunc
		if (superpause<=0 || p->ignore_superpause){
			m_pCurrentObject = p;
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_FRAME);  // ot t(object) t(class) f(frame)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(frame) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) 执行帧函数
			lua_pop(L, 2);  // ot
			
			if (p->pause<=0){
				if (p->resolve_move){
					p->vx = p->x - p->lastx;
					p->vy = p->y - p->lasty;
				}
				else{
					// 更新对象状态
					p->vx += p->ax;
					p->vy += p->ay;
#ifdef USER_SYSTEM_OPERATION
					p->vy -= p->ag;//单独的重力更新
					//速度限制，来自lua层
					cache1 = sqrt(p->vx * p->vx + p->vy * p->vy);
					if (p->maxv == 0.) {
						p->vx = p->vy = 0.;
					}
					else if (p->maxv < cache1) { //防止maxv为最大值时相乘出现溢出的情况
						cache2 = p->maxv / cache1;
						p->vx = cache2 * p->vx;
						p->vy = cache2 * p->vy;
					}
					//针对x、y方向单独限制
					if (abs(p->vx) > p->maxvx) {
						p->vx = p->maxvx * ((p->vx > 0) ? 1 : -1);
					}
					if (abs(p->vy) > p->maxvy) {
						p->vy = p->maxvy * ((p->vy > 0) ? 1 : -1);
					}
#endif
					//坐标更新
					p->x += p->vx;
					p->y += p->vy;
				}
				p->rot += p->omiga;
				
#ifdef USING_ADVANCE_COLLIDER
				//碰撞体位置更新
				int cc = 0;
				while (p->colliders[cc].type != GameObjectColliderType::None && cc < MAX_COLLIDERS_COUNT) {
					p->colliders[cc].caloffset((float)p->x, (float)p->y, (float)p->rot);
					cc++;
				}
#endif

				// 更新粒子系统（若有）
				if (p->res && p->res->GetType() == ResourceType::Particle)
				{
					float gscale = LRES.GetGlobalImageScaleFactor();
					p->ps->SetRotation((float)p->rot);
					if (p->ps->IsActived())  // 兼容性处理
					{
						p->ps->SetInactive();
						p->ps->SetCenter(fcyVec2((float)p->x, (float)p->y));
						p->ps->SetActive();
					}
					else
						p->ps->SetCenter(fcyVec2((float)p->x, (float)p->y));
					p->ps->Update(1.0f / 60.f);
				}
			
			}
			else{
				p->pause--;
			}
		}
		p = p->pObjectNext;
	}
	m_pCurrentObject = NULL;
	//*/

	lua_Number cache1, cache2;//速度限制计算时用到的中间变量
	for (auto& p : m_UpdateList) {
		// 根据id获取对象的lua绑定table、拿到class再拿到framefunc
		if (superpause <= 0 || p->ignore_superpause) {
			m_pCurrentObject = p;
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_FRAME);  // ot t(object) t(class) f(frame)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(frame) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) 执行帧函数
			lua_pop(L, 2);  // ot

			if (p->pause <= 0) {
				if (p->resolve_move) {
					p->vx = p->x - p->lastx;
					p->vy = p->y - p->lasty;
				}
				else {
					// 更新对象状态
					p->vx += p->ax;
					p->vy += p->ay;
#ifdef USER_SYSTEM_OPERATION
					p->vy -= p->ag;//单独的重力更新
					//速度限制，来自lua层
					cache1 = sqrt(p->vx * p->vx + p->vy * p->vy);
					if (p->maxv == 0.) {
						p->vx = p->vy = 0.;
					}
					else if (p->maxv < cache1) { //防止maxv为最大值时相乘出现溢出的情况
						cache2 = p->maxv / cache1;
						p->vx = cache2 * p->vx;
						p->vy = cache2 * p->vy;
					}
					//针对x、y方向单独限制
					if (abs(p->vx) > p->maxvx) {
						p->vx = p->maxvx * ((p->vx > 0) ? 1 : -1);
					}
					if (abs(p->vy) > p->maxvy) {
						p->vy = p->maxvy * ((p->vy > 0) ? 1 : -1);
					}
#endif
					//坐标更新
					p->x += p->vx;
					p->y += p->vy;
				}
				p->rot += p->omiga;

#ifdef USING_ADVANCE_COLLIDER
				//碰撞体位置更新
				int cc = 0;
				while (p->colliders[cc].type != GameObjectColliderType::None && cc < MAX_COLLIDERS_COUNT) {
					p->colliders[cc].caloffset((float)p->x, (float)p->y, (float)p->rot);
					cc++;
				}
#endif

				// 更新粒子系统（若有）
				if (p->res && p->res->GetType() == ResourceType::Particle)
				{
					float gscale = LRES.GetGlobalImageScaleFactor();
					p->ps->SetRotation((float)p->rot);
					if (p->ps->IsActived())  // 兼容性处理
					{
						p->ps->SetInactive();
						p->ps->SetCenter(fcyVec2((float)p->x, (float)p->y));
						p->ps->SetActive();
					}
					else
						p->ps->SetCenter(fcyVec2((float)p->x, (float)p->y));
					p->ps->Update(1.0f / 60.f);
				}

			}
			else {
				p->pause--;
			}
		}
	}
	m_pCurrentObject = nullptr;

	lua_pop(L, 1);
}

void GameObjectPool::DoRender()LNOEXCEPT
{
	GETOBJTABLE;  // ot

	/*
	GameObject* p = m_pRenderListHeader.pRenderNext;
	LASSERT(p != nullptr);
	lua_Integer world = GetWorldFlag();
	while (p && p != &m_pRenderListTail)
	{
		if (!p->hide  && CheckWorld(p->world, world))  // 只渲染可见对象
		{
			m_pCurrentObject = p;
			// 根据id获取对象的lua绑定table、拿到class再拿到renderfunc
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_RENDER);  // ot t(object) t(class) f(render)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(render) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) 执行渲染函数
			lua_pop(L, 2);  // ot
		}
		p = p->pRenderNext;
	}
	m_pCurrentObject = NULL;
	//*/

#ifdef USING_MULTI_GAME_WORLD
	lua_Integer world = GetWorldFlag();
#endif // USING_MULTI_GAME_WORLD
	for (auto& p : m_RenderList) {
#ifdef USING_MULTI_GAME_WORLD
		if (!p->hide && CheckWorld(p->world, world))  // 只渲染可见对象
#else // USING_MULTI_GAME_WORLD
		if (!p->hide)  // 只渲染可见对象
#endif // USING_MULTI_GAME_WORLD
		{
			m_pCurrentObject = p;
			// 根据id获取对象的lua绑定table、拿到class再拿到renderfunc
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_RENDER);  // ot t(object) t(class) f(render)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(render) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) 执行渲染函数
			lua_pop(L, 2);  // ot
		}
	}
	m_pCurrentObject = nullptr;

	lua_pop(L, 1);
}

void GameObjectPool::BoundCheck()LNOEXCEPT
{
	GETOBJTABLE;  // ot

	/*
	lua_Integer world = GetWorldFlag();
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (CheckWorld(p->world, world)){
			if ((p->x < m_BoundLeft || p->x > m_BoundRight || p->y < m_BoundBottom || p->y > m_BoundTop) && p->bound)
			{
				m_pCurrentObject = p;
				// 越界设置为DEL状态
				p->status = STATUS_DEL;

				// 根据id获取对象的lua绑定table、拿到class再拿到delfunc
				lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
				lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
				lua_rawgeti(L, -1, LGOBJ_CC_DEL);  // ot t(object) t(class) f(del)
				lua_pushvalue(L, -3);  // ot t(object) t(class) f(del) t(object)
				lua_call(L, 1, 0);  // ot t(object) t(class)
				lua_pop(L, 2);  // ot
			}
		}
		p = p->pObjectNext;
	}
	m_pCurrentObject = NULL;
	//*/

#ifdef USING_MULTI_GAME_WORLD
	lua_Integer world = GetWorldFlag();
#endif // USING_MULTI_GAME_WORLD
	for (auto& p : m_UpdateList) {
#ifdef USING_MULTI_GAME_WORLD
		if (CheckWorld(p->world, world)) {
#endif // USING_MULTI_GAME_WORLD
			if (!_ObjectBoundCheck(p))
			{
				m_pCurrentObject = p;
				// 越界设置为DEL状态
				p->status = STATUS_DEL;

				// 根据id获取对象的lua绑定table、拿到class再拿到delfunc
				lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
				lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
				lua_rawgeti(L, -1, LGOBJ_CC_DEL);  // ot t(object) t(class) f(del)
				lua_pushvalue(L, -3);  // ot t(object) t(class) f(del) t(object)
				lua_call(L, 1, 0);  // ot t(object) t(class)
				lua_pop(L, 2);  // ot
			}
#ifdef USING_MULTI_GAME_WORLD
		}
#endif // USING_MULTI_GAME_WORLD
	}
	m_pCurrentObject = nullptr;

	lua_pop(L, 1);
}

void GameObjectPool::CollisionCheck(size_t groupA, size_t groupB)LNOEXCEPT
{
	if (groupA < 0 || groupA >= LGOBJ_MAXCNT || groupB < 0 || groupB >= LGOBJ_MAXCNT)
		luaL_error(L, "Invalid collision group.");

	GETOBJTABLE;  // ot

	/*
	GameObject* pA = m_pCollisionListHeader[groupA].pCollisionNext;
	GameObject* pATail = &m_pCollisionListTail[groupA];
	GameObject* pBHeader = m_pCollisionListHeader[groupB].pCollisionNext;
	GameObject* pBTail = &m_pCollisionListTail[groupB];
	while (pA && pA != pATail)
	{
		GameObject* pB = pBHeader;
		while (pB && pB != pBTail)
		{
			if (CheckWorlds(pA->world, pB->world)){
				if (LuaSTGPlus::CollisionCheck(pA, pB))
				{
					// 根据id获取对象的lua绑定table、拿到class再拿到collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot t(object)
					lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot t(object) t(class) f(colli)
					lua_pushvalue(L, -3);  // ot t(object) t(class) f(colli) t(object)
					lua_rawgeti(L, -5, pB->id + 1);  // ot t(object) t(class) f(colli) t(object) t(object)
					lua_call(L, 2, 0);  // ot t(object) t(class)
					lua_pop(L, 2);  // ot
				}
			}
			pB = pB->pCollisionNext;
		}
		pA = pA->pCollisionNext;
	}
	//*/

	for (auto& pA : m_ColliList[groupA]) {
		for (auto& pB : m_ColliList[groupB]) {
#ifdef USING_MULTI_GAME_WORLD
			if (CheckWorlds(pA->world, pB->world)) {
#endif // USING_MULTI_GAME_WORLD
				if (LuaSTGPlus::CollisionCheck(pA, pB))
				{
					m_pCurrentObject = pA;
					// 根据id获取对象的lua绑定table、拿到class再拿到collifunc
					lua_rawgeti(L, -1, pA->id + 1);  // ot t(object)
					lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
					lua_rawgeti(L, -1, LGOBJ_CC_COLLI);  // ot t(object) t(class) f(colli)
					lua_pushvalue(L, -3);  // ot t(object) t(class) f(colli) t(object)
					lua_rawgeti(L, -5, pB->id + 1);  // ot t(object) t(class) f(colli) t(object) t(object)
					lua_call(L, 2, 0);  // ot t(object) t(class)
					lua_pop(L, 2);  // ot
				}
#ifdef USING_MULTI_GAME_WORLD
			}
#endif // USING_MULTI_GAME_WORLD
		}
	}
	m_pCurrentObject = nullptr;

	lua_pop(L, 1);
}

void GameObjectPool::UpdateXY()LNOEXCEPT
{
	int superpause = GetSuperPauseTime();

	/*
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (superpause <= 0 || p->ignore_superpause){
			p->dx = p->x - p->lastx;
			p->dy = p->y - p->lasty;
			p->lastx = p->x;
			p->lasty = p->y;
			if (p->navi && (p->dx != 0 || p->dy != 0))
				p->rot = atan2(p->dy, p->dx);

		}
		p = p->pObjectNext;
	}
	//*/

	for (auto& p : m_UpdateList) {
		if (superpause <= 0 || p->ignore_superpause) {
			p->dx = p->x - p->lastx;
			p->dy = p->y - p->lasty;
			p->lastx = p->x;
			p->lasty = p->y;
			if (p->navi && (p->dx != 0 || p->dy != 0))
				p->rot = std::atan2(p->dy, p->dx);

		}
	}
}

void GameObjectPool::AfterFrame()LNOEXCEPT
{
	int superpause = GetSuperPauseTime();

	/*
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (superpause <= 0 || p->ignore_superpause){
			p->timer++;
			p->ani_timer++;
			if (p->status != STATUS_DEFAULT)
				p = freeObject(p);
			else
				p = p->pObjectNext;
		}
		else{
			p = p->pObjectNext;
		}
		
	}
	//*/
	
	GameObject* p = nullptr;
	for (auto it = m_UpdateList.begin(); it != m_UpdateList.end();) {
		p = *it;
		if (superpause <= 0 || p->ignore_superpause) {
			p->timer++;
			p->ani_timer++;
			it++;
			if (p->status != STATUS_DEFAULT) {
				freeObject(p);
			}
		}
	}
}

int GameObjectPool::New(lua_State* L)LNOEXCEPT
{
	// 检查参数
	if (!lua_istable(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
	lua_getfield(L, 1, "is_class");  // t(class) ... b
	if (!lua_toboolean(L, -1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
	lua_pop(L, 1);  // t(class) ...

	/*
	// 分配一个对象
	size_t id = 0;
	if (!m_ObjectPool.Alloc(id))
		return luaL_error(L, "can't alloc object, object pool may be full.");
	
	// 设置对象
	GameObject* p = m_ObjectPool.Data(id);
	LASSERT(p);
	p->Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = m_iUid++;

	if (m_pCurrentObject){
		p->world = m_pCurrentObject->world;
	}

	// 插入链表域
	LIST_INSERT_BEFORE(&m_pObjectListTail, p, Object);  // Object链表只与uid有关，因此总在末尾插入
	LIST_INSERT_BEFORE(&m_pRenderListTail, p, Render);  // Render链表在插入后还需要进行排序
	LIST_INSERT_BEFORE(&m_pCollisionListTail[p->group], p, Collision);  // 为保证兼容性，对Collision也做排序
	LIST_INSERT_SORT(p, Render, RenderListSortFunc);
	LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);
	//*/
	
	// 分配一个对象
	GameObject* p = _AllocObject();
	if (p == nullptr) {
		return luaL_error(L, "can't alloc object, object pool may be full.");
	}

	GETOBJTABLE;  // t(class) ... ot
	lua_createtable(L, 2, 0);  // t(class) ... ot t(object)
	lua_pushvalue(L, 1);  // t(class) ... ot t(object) class
	lua_rawseti(L, -2, 1);  // t(class) ... ot t(object)  设置class
	lua_pushinteger(L, (lua_Integer)(p->id));  // t(class) ... ot t(object) id
	lua_rawseti(L, -2, 2);  // t(class) ... ot t(object)  设置id
	lua_getfield(L, -2, METATABLE_OBJ);  // t(class) ... ot t(object) mt
	lua_setmetatable(L, -2);  // t(class) ... ot t(object)  设置元表
	lua_pushvalue(L, -1);  // t(class) ... ot t(object) t(object)
	lua_rawseti(L, -3, p->id + 1);  // t(class) ... ot t(object)  设置到全局表
	lua_insert(L, 1);  // t(object) t(class) ... ot
	lua_pop(L, 1);  // t(object) t(class) ...
	lua_rawgeti(L, 2, LGOBJ_CC_INIT);  // t(object) t(class) ... f(init)
	lua_insert(L, 3);  // t(object) t(class) f(init) ...
	lua_pushvalue(L, 1);  // t(object) t(class) f(init) ... t(object)
	lua_insert(L, 4);  // t(object) t(class) f(init) t(object) ...
	lua_call(L, lua_gettop(L) - 3, 0);  // t(object) t(class)  执行构造函数
	lua_pop(L, 1);  // t(object)

	p->lastx = p->x;
	p->lasty = p->y;
	return 1;
}

int GameObjectPool::Add(lua_State* L)LNOEXCEPT
{
	// 检查参数
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid argument #1, luastg object required for 'Add'.");
//	lua_getfield(L, 1, "is_class");  // t(class) ... b
//	if (!lua_toboolean(L, -1))
//		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
//	lua_pop(L, 1);  // t(class) ...
	/*
	// 分配一个对象
	size_t id = 0;
	if (!m_ObjectPool.Alloc(id))
		return luaL_error(L, "can't alloc object, object pool may be full.");

	// 设置对象
	GameObject* p = m_ObjectPool.Data(id);
	LASSERT(p);
	p->Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = m_iUid++;

	// 插入链表域
	LIST_INSERT_BEFORE(&m_pObjectListTail, p, Object);  // Object链表只与uid有关，因此总在末尾插入
	LIST_INSERT_BEFORE(&m_pRenderListTail, p, Render);  // Render链表在插入后还需要进行排序
	LIST_INSERT_BEFORE(&m_pCollisionListTail[p->group], p, Collision);  // 为保证兼容性，对Collision也做排序
	LIST_INSERT_SORT(p, Render, RenderListSortFunc);
	LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);
	//*/

	GameObject* p = _AllocObject();
	if (p == nullptr) {
		return luaL_error(L, "can't alloc object, object pool may be full.");
	}

	GETOBJTABLE;  // t(object) ... ot
	lua_pushvalue(L, 1);  // t(object) ... ot t(object)
	lua_pushinteger(L, (lua_Integer)(p->id));  // t(object) ... ot t(object) id
	lua_rawseti(L, -2, 2);  // t(object) ... ot t(object)  设置id
	lua_getfield(L, -2, METATABLE_OBJ);  // t(object) ... ot t(object) mt
	lua_setmetatable(L, -2);  // t(object) ... ot t(object)  设置元表
	lua_pushvalue(L, -1);  // t(object) ... ot t(object) t(object)
	lua_rawseti(L, -3, p->id + 1);  // t(object) ... ot t(object)  设置到全局表

	InitAttr(L); //将初始属性传递至C++对象

	lua_insert(L, 1);  // t(object) t(object) ... ot
	lua_pop(L, 1);  // t(object) t(object) ...

	lua_getfield(L, 1, "init"); // t(object) t(object) ... f(init)
	if (lua_isnil(L, -1)){//没有构造函数
		lua_pop(L, 3);// t(object)
	}
	else{
		lua_insert(L, 3);  // t(object) t(object) f(init) ...
		lua_pushvalue(L, 1);  // t(object) t(object) f(init) ... t(object)
		lua_insert(L, 4);  // t(object) t(object) f(init) t(object) ...
		lua_call(L, lua_gettop(L) - 3, 0);  // t(object) t(object)  执行构造函数
		lua_pop(L, 1);  // t(object)
	}
	p->lastx = p->x;
	p->lasty = p->y;
	return 1;
}

int GameObjectPool::Del(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object required for 'Del'.");
	lua_rawgeti(L, 1, 2);  // t(object) ... id
	GameObject* p = m_ObjectPool.Data((size_t)luaL_checknumber(L, -1));
	lua_pop(L, 1);  // t(object) ...
	if (!p)
		return luaL_error(L, "invalid argument #1, invalid luastg object.");
	
	if (p->status == STATUS_DEFAULT)
	{
		p->status = STATUS_DEL;

		// 调用类中的回调方法
		lua_rawgeti(L, 1, 1);  // t(object) ... class
		lua_rawgeti(L, -1, LGOBJ_CC_DEL);  // t(object) ... class f(del)
		lua_insert(L, 1);  // f(del) t(object) ... class
		lua_pop(L, 1);  // f(del) t(object) ...
		lua_call(L, lua_gettop(L) - 1, 0);
	}
	return 0;
}

int GameObjectPool::Kill(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object required for 'Kill'.");
	lua_rawgeti(L, 1, 2);  // t(object) ... id
	GameObject* p = m_ObjectPool.Data((size_t)luaL_checknumber(L, -1));
	lua_pop(L, 1);  // t(object) ...
	if (!p)
		return luaL_error(L, "invalid argument #1, invalid luastg object.");

	if (p->status == STATUS_DEFAULT)
	{
		p->status = STATUS_KILL;

		// 调用类中的回调方法
		lua_rawgeti(L, 1, 1);  // t(object) ... class
		lua_rawgeti(L, -1, LGOBJ_CC_KILL);  // t(object) ... class f(kill)
		lua_insert(L, 1);  // f(kill) t(object) ... class
		lua_pop(L, 1);  // f(kill) t(object) ...
		lua_call(L, lua_gettop(L) - 1, 0);
	}
	return 0;
}

int GameObjectPool::IsValid(lua_State* L)LNOEXCEPT
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "invalid argument count, 1 argument required for 'IsValid'.");
	if (!lua_istable(L, -1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_rawgeti(L, -1, 2);  // t(object) id
	if (!lua_isnumber(L, -1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	// 在对象池中检查
	size_t id = (size_t)lua_tonumber(L, -1);
	lua_pop(L, 1);  // t(object)
	if (!m_ObjectPool.Data(id))
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	GETOBJTABLE;  // t(object) ot
	lua_rawgeti(L, -1, (lua_Integer)(id + 1));  // t(object) ot t(object)
	if (lua_rawequal(L, -1, -3))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

bool GameObjectPool::Angle(size_t idA, size_t idB, double& out)LNOEXCEPT
{
	GameObject* pA = m_ObjectPool.Data(idA);
	GameObject* pB = m_ObjectPool.Data(idB);
	if (!pA || !pB)
		return false;
	out = LRAD2DEGREE * atan2(pB->y - pA->y, pB->x - pA->x);
	return true;
}

bool GameObjectPool::Dist(size_t idA, size_t idB, double& out)LNOEXCEPT
{
	GameObject* pA = m_ObjectPool.Data(idA);
	GameObject* pB = m_ObjectPool.Data(idB);
	if (!pA || !pB)
		return false;
	lua_Number dx = pB->x - pA->x;
	lua_Number dy = pB->y - pA->y;
	out = sqrt(dx*dx + dy*dy);
	return true;
}

bool GameObjectPool::ColliCheck(size_t idA, size_t idB, bool ignoreWorldMask, bool& out)LNOEXCEPT {
	GameObject* pA = m_ObjectPool.Data(idA);
	GameObject* pB = m_ObjectPool.Data(idB);
	if (!pA || !pB) {
		return false;//找不到对象，GG
	}
#ifdef USING_MULTI_GAME_WORLD
	if (ignoreWorldMask) {
#endif // USING_MULTI_GAME_WORLD
		out = LuaSTGPlus::CollisionCheck(pA, pB);
#ifdef USING_MULTI_GAME_WORLD
	}
	else{
		if (CheckWorlds(pA->world, pB->world)) {
			out = LuaSTGPlus::CollisionCheck(pA, pB);
		}
		else {
			out = false;//不在同一个world
		}
	}
#endif // USING_MULTI_GAME_WORLD
	return true;
}

bool GameObjectPool::GetV(size_t id, double& v, double& a)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;
	v = sqrt(p->vx * p->vx + p->vy * p->vy);
	a = atan2(p->vy, p->vx) * LRAD2DEGREE;
	return true;
}

bool GameObjectPool::SetV(size_t id, double v, double a, bool updateRot)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;
	a *= LDEGREE2RAD;
	p->vx = v*cos(a);
	p->vy = v*sin(a);
	if (updateRot)
		p->rot = a;
	return true;
}

bool GameObjectPool::SetImgState(size_t id, BlendMode m, fcyColor c)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;
	if (p->res)
	{
		switch (p->res->GetType())
		{
		case ResourceType::Sprite:
			static_cast<ResSprite*>(p->res)->SetBlendMode(m);
			static_cast<ResSprite*>(p->res)->GetSprite()->SetColor(c);
			break;
		case ResourceType::Animation:
			do {
				ResAnimation* ani = static_cast<ResAnimation*>(p->res);
				ani->SetBlendMode(m);
				for (size_t i = 0; i < ani->GetCount(); ++i)
					ani->GetSprite(i)->SetColor(c);
			} while (false);
			break;
		default:
			break;
		}
	}
	return true;
}

bool GameObjectPool::SetParState(size_t id, BlendMode m, fcyColor c)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;
	if (p->res)
	{
		switch (p->res->GetType())
		{
		case ResourceType::Particle:
			p->ps->SetBlendMode(m);
			p->ps->SetMixColor(c);
			break;
		default:
			break;
		}
	}
	return true;
}

bool GameObjectPool::BoxCheck(size_t id, double left, double right, double top, double bottom, bool& ret)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;
	ret = (p->x > left) && (p->x < right) && (p->y > top) && (p->y < bottom);
	return true;
}

void GameObjectPool::ResetPool()LNOEXCEPT
{
	/*
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p != &m_pObjectListTail)
		p = freeObject(p);
	//*/

	for (auto it = m_UpdateList.begin(); it != m_UpdateList.end();) {
		auto p = *it;
		it++;
		freeObject(p);
	}
	m_UpdateList.clear();
	m_RenderList.clear();
	for (auto& i : m_ColliList) {
		i.clear();
	}
}

bool GameObjectPool::DoDefaultRender(size_t id)LNOEXCEPT
{
	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return false;

	if (p->res)
	{
		switch (p->res->GetType())
		{
		case ResourceType::Sprite:
			LAPP.Render(
				static_cast<ResSprite*>(p->res),
				static_cast<float>(p->x),
				static_cast<float>(p->y),
				static_cast<float>(p->rot),
				static_cast<float>(p->hscale * LRES.GetGlobalImageScaleFactor()),
				static_cast<float>(p->vscale * LRES.GetGlobalImageScaleFactor())
			);
			break;
		case ResourceType::Animation:
			LAPP.Render(
				static_cast<ResAnimation*>(p->res),
				p->ani_timer,
				static_cast<float>(p->x),
				static_cast<float>(p->y),
				static_cast<float>(p->rot),
				static_cast<float>(p->hscale * LRES.GetGlobalImageScaleFactor()),
				static_cast<float>(p->vscale * LRES.GetGlobalImageScaleFactor())
			);
			break;
		case ResourceType::Particle:
			LAPP.Render(
				p->ps,
				static_cast<float>(p->hscale * LRES.GetGlobalImageScaleFactor()),
				static_cast<float>(p->vscale * LRES.GetGlobalImageScaleFactor())
			);
			break;
		default:
			break;
		}
	}
	
	return true;
}
//!!
int GameObjectPool::NextObject(int groupId, int id)LNOEXCEPT
{
	if (id < 0)
		return -1;

	GameObject* p = m_ObjectPool.Data(static_cast<size_t>(id));
	if (!p)
		return -1;

	/*
	// 如果不是一个有效的分组，则在整个对象表中遍历
	if (groupId < 0 || groupId >= LGOBJ_GROUPCNT)
	{
		p = p->pObjectNext;
		if (p == &m_pObjectListTail)
			return -1;
		else
			return static_cast<int>(p->id);
	}
	else
	{
		if (p->group != groupId)
			return -1;
		p = p->pCollisionNext;
		if (p == &m_pCollisionListTail[groupId])
			return -1;
		else
			return static_cast<int>(p->id);
	}
	//*/

	// 如果不是一个有效的分组，则在整个对象表中遍历
	if (groupId < 0 || groupId >= LGOBJ_GROUPCNT)
	{
		auto it = m_UpdateList.find(p);
		it++;
		if (it != m_UpdateList.end()) {
			return static_cast<int>((*it)->id);
		}
		else {
			return -1;
		}
	}
	else
	{
		if (p->group != groupId)
			return -1;

		auto it = m_ColliList[groupId].find(p);
		it++;
		if (it != m_ColliList[groupId].end()) {
			return static_cast<int>((*it)->id);
		}
		else {
			return -1;
		}
	}
}
//!!
int GameObjectPool::NextObject(lua_State* L)LNOEXCEPT
{
	int g = luaL_checkinteger(L, 1);  // i(groupId)
	int id = luaL_checkinteger(L, 2);  // id
	if (id < 0)
		return 0;

	lua_pushinteger(L, NextObject(g, id));  // ??? i(next)
	GETOBJTABLE;  // ??? i(next) ot
	lua_rawgeti(L, -1, id + 1);  // ??? i(next) ot t(object)
	lua_remove(L, -2);  // ??? i(next) t(object)
	return 2;
}
//!!
int GameObjectPool::FirstObject(int groupId)LNOEXCEPT
{
	/*
	GameObject* p;

	// 如果不是一个有效的分组，则在整个对象表中遍历
	if (groupId < 0 || groupId >= LGOBJ_GROUPCNT)
	{
		p = m_pObjectListHeader.pObjectNext;
		if (p == &m_pObjectListTail)
			return -1;
		else
			return static_cast<int>(p->id);
	}
	else
	{
		p = m_pCollisionListHeader[groupId].pCollisionNext;
		if (p == &m_pCollisionListTail[groupId])
			return -1;
		else
			return static_cast<int>(p->id);
	}
	//*/

	// 如果不是一个有效的分组，则在整个对象表中遍历
	if (groupId < 0 || groupId >= LGOBJ_GROUPCNT)
	{
		auto it = m_UpdateList.begin();
		if (it != m_UpdateList.end()) {
			return static_cast<int>((*it)->id);
		}
		else {
			return -1;
		}
	}
	else
	{
		auto it = m_ColliList[groupId].begin();
		if (it != m_ColliList[groupId].end()) {
			return static_cast<int>((*it)->id);
		}
		else {
			return -1;
		}
	}
}

int GameObjectPool::GetAttr(lua_State* L)LNOEXCEPT
{
	using namespace Xrysnow;
	
	lua_rawgeti(L, 1, 2);  // t(object) s(key) ??? i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));
	lua_pop(L, 1);  // t(object) s(key)

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__index' meta operation.");
	
	// 查询属性
	const char* key = luaL_checkstring(L, 2);
	
	// 对x,y作特化处理
	if (key[1] == '\0') {
		switch (key[0])
		{
		case 'x':
			lua_pushnumber(L, p->x);
			return 1;
		case 'y':
			lua_pushnumber(L, p->y);
			return 1;
		}
	}

	// 一般属性
	switch (GameObjectPropertyHash(L, 2))
	//switch (GameObjectPropertyHash(key))
	{
	case GameObjectProperty::DX:
		lua_pushnumber(L, p->dx);
		break;
	case GameObjectProperty::DY:
		lua_pushnumber(L, p->dy);
		break;
	case GameObjectProperty::ROT:
		lua_pushnumber(L, p->rot * LRAD2DEGREE);
		break;
	case GameObjectProperty::OMEGA:
		lua_pushnumber(L, p->omiga * LRAD2DEGREE);
		break;
	case GameObjectProperty::TIMER:
		lua_pushinteger(L, p->timer);
		break;
	case GameObjectProperty::VX:
		lua_pushnumber(L, p->vx);
		break;
	case GameObjectProperty::VY:
		lua_pushnumber(L, p->vy);
		break;
	case GameObjectProperty::AX:
		lua_pushnumber(L, p->ax);
		break;
	case GameObjectProperty::AY:
		lua_pushnumber(L, p->ay);
		break;
#ifdef USER_SYSTEM_OPERATION
	case GameObjectProperty::MAXV:
		lua_pushnumber(L, p->maxv);
		break;
	case GameObjectProperty::MAXVX:
		lua_pushnumber(L, p->maxvx);
		break;
	case GameObjectProperty::MAXVY:
		lua_pushnumber(L, p->maxvy);
		break;
	case GameObjectProperty::AG:
		lua_pushnumber(L, p->ag);
		break;
#endif
	case GameObjectProperty::LAYER:
		lua_pushnumber(L, p->layer);
		break;
	case GameObjectProperty::GROUP:
		lua_pushinteger(L, p->group);
		break;
	case GameObjectProperty::HIDE:
		lua_pushboolean(L, p->hide);
		break;
	case GameObjectProperty::BOUND:
		lua_pushboolean(L, p->bound);
		break;
	case GameObjectProperty::NAVI:
		lua_pushboolean(L, p->navi);
		break;
	case GameObjectProperty::COLLI:
		lua_pushboolean(L, p->colli);
		break;
	case GameObjectProperty::STATUS:
		switch (p->status)
		{
		case STATUS_DEFAULT:
			lua_pushstring(L, "normal");
			break;
		case STATUS_KILL:
			lua_pushstring(L, "kill");
			break;
		case STATUS_DEL:
			lua_pushstring(L, "del");
			break;
		default:
			LASSERT(false);
			break;
		}
		break;
	case GameObjectProperty::HSCALE:
		lua_pushnumber(L, p->hscale);
		break;
	case GameObjectProperty::VSCALE:
		lua_pushnumber(L, p->vscale);
		break;
	case GameObjectProperty::CLASS:
		lua_rawgeti(L, 1, 1);
		break;
#ifdef USING_ADVANCE_COLLIDER
	case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		lua_pushnumber(L, p->colliders[0].a / LRES.GetGlobalImageScaleFactor());
#else
		lua_pushnumber(L, p->colliders[0].a);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		break;
	case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		lua_pushnumber(L, p->colliders[0].b / LRES.GetGlobalImageScaleFactor());
#else
		lua_pushnumber(L, p->colliders[0].b);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		break;
	case GameObjectProperty::RECT:
		if ((p->colliders[0].type == GameObjectColliderType::Circle) || (p->colliders[0].type == GameObjectColliderType::Ellipse)) {
			lua_pushboolean(L, false);
		}
		else if (p->colliders[0].type == GameObjectColliderType::OBB) {
			lua_pushboolean(L, true);
		}
		else {
			lua_pushinteger(L, (int)p->colliders[0].type);
		}
		break;
#else
	case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		lua_pushnumber(L, p->a / LRES.GetGlobalImageScaleFactor());
#else
		lua_pushnumber(L, p->a);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		break;
	case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		lua_pushnumber(L, p->b / LRES.GetGlobalImageScaleFactor());
#else
		lua_pushnumber(L, p->b);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		break;
	case GameObjectProperty::RECT:
		lua_pushboolean(L, p->rect);
		break;
#endif // USING_ADVANCE_COLLIDER
	case GameObjectProperty::IMG:
		if (p->res)
			lua_pushstring(L, p->res->GetResName().c_str());
		else
			lua_pushnil(L);
		break;
	case GameObjectProperty::ANI:
		lua_pushinteger(L, p->ani_timer);
		break;
	case GameObjectProperty::RESOLVEMOVE:
		lua_pushboolean(L, p->resolve_move);
		break;
	case GameObjectProperty::VSPEED:
		lua_pushnumber(L, sqrt(p->vx*p->vx + p->vy*p->vy));
		break;
	case GameObjectProperty::VANGLE:
		if (p->vx || p->vy){
			lua_pushnumber(L, atan2(p->vy, p->vx)*LRAD2DEGREE);
		}
		else{
			lua_pushnumber(L, p->rot*LRAD2DEGREE);
		}
		break;
	case GameObjectProperty::IGNORESUPERPAUSE:
		lua_pushboolean(L, p->ignore_superpause);
		break;
	case GameObjectProperty::PAUSE:
		lua_pushinteger(L, p->pause);
		break;
	case GameObjectProperty::WORLD:
		lua_pushinteger(L, p->world);
		break;
#ifdef USING_ADVANCE_COLLIDER
	case GameObjectProperty::COLLIDER:
		GameObjectColliderWrapper::CreateAndPush(L, p);
		break;
#endif // USING_ADVANCE_COLLIDER
	case GameObjectProperty::X:
	case GameObjectProperty::Y:
		break;
	default:
		lua_pushnil(L);
		break;
	}

	return 1;
}

int GameObjectPool::SetAttr(lua_State* L)LNOEXCEPT
{
	using namespace Xrysnow;
	
	lua_rawgeti(L, 1, 2);  // t(object) s(key) any(v) i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));
	lua_pop(L, 1);  // t(object) s(key) any(v)

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__newindex' meta operation.");

	// 查询属性
	const char* key = luaL_checkstring(L, 2);
	std::string keypp = key;

	// 对x,y作特化处理
	if (key[1] == '\0') {
		switch (key[0])
		{
		case 'x':
			p->x = luaL_checknumber(L, 3);
			return 0;
		case 'y':
			p->y = luaL_checknumber(L, 3);
			return 0;
		}
	}

	// 一般属性
	switch (GameObjectPropertyHash(L, 2))
	{
	case GameObjectProperty::DX:
		return luaL_error(L, "property 'dx' is readonly.");
	case GameObjectProperty::DY:
		return luaL_error(L, "property 'dy' is readonly.");
	case GameObjectProperty::ROT:
		p->rot = luaL_checknumber(L, 3) * LDEGREE2RAD;
		break;
	case GameObjectProperty::OMEGA:
		p->omiga = luaL_checknumber(L, 3) * LDEGREE2RAD;
		break;
	case GameObjectProperty::TIMER:
		p->timer = luaL_checkinteger(L, 3);
		break;
	case GameObjectProperty::VX:
		p->vx = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::VY:
		p->vy = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::AX:
		p->ax = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::AY:
		p->ay = luaL_checknumber(L, 3);
		break;
#ifdef USER_SYSTEM_OPERATION
	case GameObjectProperty::MAXV:
		p->maxv = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::MAXVX:
		p->maxvx = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::MAXVY:
		p->maxvy = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::AG:
		p->ag = luaL_checknumber(L, 3);
		break;
#endif
	case GameObjectProperty::LAYER:
		/*
		p->layer = luaL_checknumber(L, 3);
		LIST_INSERT_SORT(p, Render, RenderListSortFunc); // 刷新p的渲染层级
		LASSERT(m_pRenderListHeader.pRenderNext != nullptr);
		LASSERT(m_pRenderListTail.pRenderPrev != nullptr);
		//*/
		_SetObjectLayer(p, luaL_checknumber(L, 3));
		break;
	case GameObjectProperty::GROUP:
		/*
		do
		{
			int group = luaL_checkinteger(L, 3);
			if (group != p->group)
			{
				if (0 <= p->group && p->group < LGOBJ_GROUPCNT)
					LIST_REMOVE(p, Collision);
				p->group = group;
				if (0 <= group && group < LGOBJ_GROUPCNT)
				{
					LIST_INSERT_BEFORE(&m_pCollisionListTail[group], p, Collision);
					LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);  // 刷新p的碰撞次序
					LASSERT(m_pCollisionListHeader[group].pCollisionNext != nullptr);
					LASSERT(m_pCollisionListTail[group].pCollisionPrev != nullptr);
				}
			}
		} while (false);
		//*/
	{
		int group = luaL_checkinteger(L, 3);
		if (0 <= group && group < LGOBJ_GROUPCNT)
		{
			_SetObjectColliGroup(p, luaL_checkinteger(L, 3));
		}
		break;
	}
	case GameObjectProperty::HIDE:
		p->hide = lua_toboolean(L, 3) == 0 ? false : true;
		break;
	case GameObjectProperty::BOUND:
		p->bound = lua_toboolean(L, 3) == 0 ? false : true;
		break;
	case GameObjectProperty::NAVI:
		p->navi = lua_toboolean(L, 3) == 0 ? false : true;
		break;
	case GameObjectProperty::COLLI:
		p->colli = lua_toboolean(L, 3) == 0 ? false : true;
		break;

	case GameObjectProperty::RESOLVEMOVE:
		p->resolve_move = lua_toboolean(L, 3) == 0 ? false : true;
		break;
	case GameObjectProperty::IGNORESUPERPAUSE:
		p->ignore_superpause = lua_toboolean(L, 3) == 0 ? false : true;
		break;
	case GameObjectProperty::PAUSE:
		p->pause = luaL_checkinteger(L, 3);
		break;
	case GameObjectProperty::STATUS:
		do {
			const char* val = luaL_checkstring(L, 3);
			if (strcmp(val, "normal") == 0)
				p->status = STATUS_DEFAULT;
			else if (strcmp(val, "del") == 0)
				p->status = STATUS_DEL;
			else if (strcmp(val, "kill") == 0)
				p->status = STATUS_KILL;
			else
				return luaL_error(L, "invalid argument for property 'status'.");
		} while (false);
		break;
	case GameObjectProperty::HSCALE:
		p->hscale = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::VSCALE:
		p->vscale = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::CLASS:
		lua_rawseti(L, 1, 1);
		break;
#ifdef USING_ADVANCE_COLLIDER
	case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		p->colliders[0].a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
		p->colliders[0].a = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		p->colliders[0].calcircum();
		break;
	case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		p->colliders[0].b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
		p->colliders[0].b = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		p->colliders[0].calcircum();
		break;
	case GameObjectProperty::RECT:
		if (lua_isboolean(L, 3)) {
			bool ret = lua_toboolean(L, 3) == 0 ? false : true;
			if (ret) {
				p->colliders[0].type = GameObjectColliderType::OBB;
			}
			else {
				p->colliders[0].type = GameObjectColliderType::Ellipse;
			}
		}
		else {
			int iret = luaL_checkinteger(L, 3);
			GameObjectColliderType enumtype = (GameObjectColliderType)iret;
			p->colliders[0].type = enumtype;
		}
		p->colliders[0].calcircum();
		break;
#else
	case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		p->a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
		p->a = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		p->UpdateCollisionCirclrRadius();
		break;
	case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
		p->b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
		p->b = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
		p->UpdateCollisionCirclrRadius();
		break;
	case GameObjectProperty::RECT:
		p->rect = lua_toboolean(L, 3) == 0 ? false : true;
		p->UpdateCollisionCirclrRadius();
		break;
#endif // USING_ADVANCE_COLLIDER
	case GameObjectProperty::IMG:
		do
		{
			const char* name = luaL_checkstring(L, 3);
			if (!p->res || strcmp(name, p->res->GetResName().c_str()) != 0)
			{
				p->ReleaseResource();
				if (!p->ChangeResource(name))
					return luaL_error(L, "can't find resource '%s' in image/animation/particle pool.", luaL_checkstring(L, 3));
			}
		} while (false);
		break;
	case GameObjectProperty::VSPEED:
		{
		float a1 = sqrt(p->vx*p->vx + p->vy*p->vy);
		float a2 = luaL_checknumber(L, 3);
		if (!a1){	
			p->vx = cos(p->rot)*a2;
			p->vy = sin(p->rot)*a2;
			break;
		}
		a2 = a2 / a1;
		p->vx *= a2;
		p->vy *= a2;
		}
		break;
	case GameObjectProperty::VANGLE:
	{
		float a1 = sqrt(p->vx*p->vx + p->vy*p->vy);
		float a2 = luaL_checknumber(L, 3) * LDEGREE2RAD;
		if (!a1){
			p->rot = a2;
			break;
		}
		
		p->vx = a1*cos(a2);
		p->vy = a1*sin(a2);
	}
		break;
	case GameObjectProperty::WORLD:
		p->world = luaL_checknumber(L, 3);
		break;
	case GameObjectProperty::ANI:
		return luaL_error(L, "property 'ani' is readonly.");
#ifdef USING_ADVANCE_COLLIDER
	case GameObjectProperty::COLLIDER:
	{
		// object k t
		int count = lua_objlen(L, 3);
		if (count > MAX_COLLIDERS_COUNT || count < 0) {
			return luaL_error(L, "Out of collider limits.");
		}
		else if (count < (MAX_COLLIDERS_COUNT - 1)) {
			p->colliders[count].type = GameObjectColliderType::None;
		}
		int truei;
		for (int select = 1; select <= count; select++) {
			lua_pushinteger(L, select);// object k t select
			lua_gettable(L, 3);// object k t t
			//{type, a, b, x, y, rot }
			for (int index = 1; index <= 6; index++) {
				lua_pushinteger(L, index);
				lua_gettable(L, 4);
			}
			truei = select - 1;
			// object k t t type a b x y rot
			switch (luaL_checkinteger(L, 5))
			{
			case -1:
				p->colliders[truei].type = GameObjectColliderType::None;
				break;
			case 0:
				p->colliders[truei].type = GameObjectColliderType::Circle;
				break;
			case 1:
				p->colliders[truei].type = GameObjectColliderType::OBB;
				break;
			case 2:
				p->colliders[truei].type = GameObjectColliderType::Ellipse;
				break;
			case 3:
				p->colliders[truei].type = GameObjectColliderType::Diamond;
				break;
			case 4:
				p->colliders[truei].type = GameObjectColliderType::Triangle;
				break;
			case 5:
				p->colliders[truei].type = GameObjectColliderType::Point;
				break;
			default:
				return luaL_error(L, "Invalid collider type.");
			}
			p->colliders[truei].a = luaL_checknumber(L, 6);
			p->colliders[truei].b = luaL_checknumber(L, 7);
			p->colliders[truei].dx = luaL_checknumber(L, 8);
			p->colliders[truei].dy = luaL_checknumber(L, 9);
			p->colliders[truei].rot = luaL_checknumber(L, 10) * LDEGREE2RAD;
			p->colliders[truei].calcircum();//刷新外接圆半径
			lua_pop(L, 7);
			// object k t
		}
		break;
	}
#endif // USING_ADVANCE_COLLIDER
	case GameObjectProperty::X:
	case GameObjectProperty::Y:
		break;
	default:
		lua_rawset(L, 1);
		break;
	}

	return 0;
}

int GameObjectPool::InitAttr(lua_State* L)LNOEXCEPT  // t(object)
{
	using namespace Xrysnow;
	
	lua_rawgeti(L, 1, 2);  // t(object) i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for init function.");

	// 查询属性
	const char* key = NULL;

	lua_getfield(L, 1, "x"); // t(object) i(id) v(x)
	p->x = luaL_optnumber(L, 3, 0);
	lua_pop(L, 1); // t(object) i(id)

	lua_getfield(L, 1, "y"); // t(object) i(id) v(x)
	p->y = luaL_optnumber(L, 3, 0);
	lua_pop(L, 2); // t(object)

	for (int i = 2; i < 26; i++)
	{
		key = LuaSTGPlusESC::s_orgKeyList[i];
		lua_pushstring(L, key); // t(object) s(key)
		lua_pushstring(L, key); // t(object) s(key) s(key)
		lua_rawget(L, 1); // t(object) s(key) v(x)
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 2);// t(object)
		}
		else
		{
			switch ((GameObjectProperty)i)
			{
			case GameObjectProperty::ROT:
				p->rot = luaL_checknumber(L, 3) * LDEGREE2RAD;
				break;
			case GameObjectProperty::OMEGA:
				p->omiga = luaL_checknumber(L, 3) * LDEGREE2RAD;
				break;
			case GameObjectProperty::TIMER:
				p->timer = luaL_checkinteger(L, 3);
				break;
			case GameObjectProperty::VX:
				p->vx = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::VY:
				p->vy = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::AX:
				p->ax = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::AY:
				p->ay = luaL_checknumber(L, 3);
				break;
#ifdef USER_SYSTEM_OPERATION
			case GameObjectProperty::MAXV:
				p->maxv = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::MAXVX:
				p->maxvx = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::MAXVY:
				p->maxvy = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::AG:
				p->ag = luaL_checknumber(L, 3);
				break;
#endif
			case GameObjectProperty::LAYER:
				/*
				p->layer = luaL_checknumber(L, 3);
				LIST_INSERT_SORT(p, Render, RenderListSortFunc); // 刷新p的渲染层级
				LASSERT(m_pRenderListHeader.pRenderNext != nullptr);
				LASSERT(m_pRenderListTail.pRenderPrev != nullptr);
				//*/
				_SetObjectLayer(p, luaL_checknumber(L, 3));
				break;
			case GameObjectProperty::GROUP:
				/*
				do
				{
					int group = luaL_checkinteger(L, 3);
					if (group != p->group)
					{
						if (0 <= p->group && p->group < LGOBJ_GROUPCNT)
							LIST_REMOVE(p, Collision);
						p->group = group;
						if (0 <= group && group < LGOBJ_GROUPCNT)
						{
							LIST_INSERT_BEFORE(&m_pCollisionListTail[group], p, Collision);
							LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);  // 刷新p的碰撞次序
							LASSERT(m_pCollisionListHeader[group].pCollisionNext != nullptr);
							LASSERT(m_pCollisionListTail[group].pCollisionPrev != nullptr);
						}
					}
				} while (false);
				//*/
			{
				int group = luaL_checkinteger(L, 3);
				if (0 <= group && group < LGOBJ_GROUPCNT)
				{
					_SetObjectColliGroup(p, group);
				}
				break;
			}
			case GameObjectProperty::HIDE:
				p->hide = lua_toboolean(L, 3) == 0 ? false : true;
				break;
			case GameObjectProperty::BOUND:
				p->bound = lua_toboolean(L, 3) == 0 ? false : true;
				break;
			case GameObjectProperty::NAVI:
				p->navi = lua_toboolean(L, 3) == 0 ? false : true;
				break;
			case GameObjectProperty::COLLI:
				p->colli = lua_toboolean(L, 3) == 0 ? false : true;
				break;
			case GameObjectProperty::STATUS:
				do {
					const char* val = luaL_checkstring(L, 3);
					if (strcmp(val, "normal") == 0)
						p->status = STATUS_DEFAULT;
					else if (strcmp(val, "del") == 0)
						p->status = STATUS_DEL;
					else if (strcmp(val, "kill") == 0)
						p->status = STATUS_KILL;
					else
						return luaL_error(L, "invalid argument for property 'status'.");
				} while (false);
				break;
			case GameObjectProperty::HSCALE:
				p->hscale = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::VSCALE:
				p->vscale = luaL_checknumber(L, 3);
				break;
			case GameObjectProperty::CLASS:
				lua_rawseti(L, 1, 1);
				lua_pushinteger(L, 0);
				break;
#ifdef USING_ADVANCE_COLLIDER
			case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				p->colliders[0].a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
				p->colliders[0].a = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
				p->colliders[0].calcircum();
				break;
			case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				p->colliders[0].b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
				p->colliders[0].b = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
				p->colliders[0].calcircum();
				break;
			case GameObjectProperty::RECT:
				if (lua_isboolean(L, 3)) {
					bool ret = lua_toboolean(L, 3) == 0 ? false : true;
					if (ret) {
						p->colliders[0].type = GameObjectColliderType::OBB;
					}
					else {
						p->colliders[0].type = GameObjectColliderType::Ellipse;
				}
			}
				else {
					int iret = luaL_checkinteger(L, 3);
					GameObjectColliderType enumtype = (GameObjectColliderType)iret;
					p->colliders[0].type = enumtype;
				}
				p->colliders[0].calcircum();
				break;
#else
			case GameObjectProperty::A:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				p->a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
				p->a = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
				p->UpdateCollisionCirclrRadius();
				break;
			case GameObjectProperty::B:
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				p->b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
#else
				p->b = luaL_checknumber(L, 3);
#endif // GLOBAL_SCALE_COLLI_SHAPE
				p->UpdateCollisionCirclrRadius();
				break;
			case GameObjectProperty::RECT:
				p->rect = lua_toboolean(L, 3) == 0 ? false : true;
				p->UpdateCollisionCirclrRadius();
				break;
#endif // USING_ADVANCE_COLLIDER
			case GameObjectProperty::IMG:
				do
				{
					const char* name = luaL_checkstring(L, 3);
					if (!p->res || strcmp(name, p->res->GetResName().c_str()) != 0)
					{
						p->ReleaseResource();
						if (!p->ChangeResource(name))
							return luaL_error(L, "can't find resource '%s' in image/animation/particle pool.", luaL_checkstring(L, 3));
					}
				} while (false);
				break;
			case GameObjectProperty::ANI:
			case GameObjectProperty::X:
			case GameObjectProperty::Y:
			case GameObjectProperty::DX:
			case GameObjectProperty::DY:
			default:
				break;
			}
			lua_pop(L, 2);// t(object)
		}
	}
	return 0;
}

int GameObjectPool::GetObjectTable(lua_State* L)LNOEXCEPT
{
	GETOBJTABLE;
	return 1;
}

int GameObjectPool::ParticleStop(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
		return luaL_error(L, "invalid lstg object for 'ParticleStop'.");
	lua_rawgeti(L, 1, 2);  // t(object) ??? id
	size_t id = (size_t)luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for 'ParticleStop'.");
	if (!p->res || p->res->GetType() != ResourceType::Particle)
	{
		LWARNING("ParticleStop: 试图停止一个不带有粒子发射器的对象的粒子发射过程(uid=%d)", m_iUid);
		return 0;
	}	
	p->ps->SetInactive();
	return 0;
}

int GameObjectPool::ParticleFire(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid lstg object for 'ParticleFire'.");
	lua_rawgeti(L, 1, 2);  // t(object) ??? id
	size_t id = (size_t)luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for 'ParticleFire'.");
	if (!p->res || p->res->GetType() != ResourceType::Particle)
	{
		LWARNING("ParticleFire: 试图启动一个不带有粒子发射器的对象的粒子发射过程(uid=%d)", m_iUid);
		return 0;
	}	
	p->ps->SetActive();
	return 0;
}

int GameObjectPool::ParticleGetn(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid lstg object for 'ParticleFire'.");
	lua_rawgeti(L, 1, 2);  // t(object) ??? id
	size_t id = (size_t)luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for 'ParticleFire'.");
	if (!p->res || p->res->GetType() != ResourceType::Particle)
	{
		lua_pushinteger(L, 0);
		return 1;
	}
	lua_pushinteger(L, (int)p->ps->GetAliveCount());
	return 1;
}

int GameObjectPool::ParticleGetEmission(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid lstg object for 'ParticleGetEmission'.");
	lua_rawgeti(L, 1, 2);  // t(object) ??? id
	size_t id = (size_t)luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for 'ParticleGetEmission'.");
	if (!p->res || p->res->GetType() != ResourceType::Particle)
	{
		LWARNING("ParticleGetEmission: 试图获取一个不带有粒子发射器的对象的粒子发射密度(uid=%d)", m_iUid);
		lua_pushinteger(L, 0);
		return 1;
	}
	lua_pushnumber(L, p->ps->GetEmission());
	return 1;
}

int GameObjectPool::ParticleSetEmission(lua_State* L)LNOEXCEPT
{
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid lstg object for 'ParticleGetEmission'.");
	lua_rawgeti(L, 1, 2);  // t(object) ??? id
	size_t id = (size_t)luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for 'ParticleGetEmission'.");
	if (!p->res || p->res->GetType() != ResourceType::Particle)
	{
		LWARNING("ParticleSetEmission: 试图设置一个不带有粒子发射器的对象的粒子发射密度(uid=%d)", m_iUid);
		return 0;
	}
	p->ps->SetEmission((float)::max(0., luaL_checknumber(L, 2)));
	return 0;
}

void GameObjectPool::DrawGroupCollider(f2dGraphics2D* graph, f2dGeometryRenderer* grender, int groupId, fcyColor fillColor)
{
	/*
	GameObject* p = m_pCollisionListHeader[groupId].pCollisionNext;
	GameObject* pTail = &m_pCollisionListTail[groupId];
	lua_Integer world = GetWorldFlag();
	while (p && p != pTail)
	{
		if (p->colli && CheckWorld(p->world, world))
		{
#ifdef USING_ADVANCE_COLLIDER
			for (int select = 0; select < MAX_COLLIDERS_COUNT; select++) {
				GameObjectCollider cc = p->colliders[select];
				if (cc.type == GameObjectColliderType::None) { break; }

				cc.caloffset(p->x, p->y, p->rot);
				switch (cc.type)
				{
				case GameObjectColliderType::Circle:
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), cc.a, fillColor, fillColor,
						cc.a < 10.0 ? 6 : (cc.a < 20.0 ? 16 : 32));
					break;
				case GameObjectColliderType::OBB:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出矩形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Ellipse:
				{
					const int vertcount = 37;//分割36份，还要中心一个点
					const int indexcount = 111;//37*3加一个组成封闭图形
					//准备顶点索引
					fuShort index[indexcount];
					{
						for (int i = 0; i < (vertcount - 1); i++) {
							index[i * 3] = 0;//中心点
							index[i * 3 + 1] = i;//1
							index[i * 3 + 2] = i + 1;//2
							//index[i * 3 + 3] = i + 1;//2 //fancy2d貌似不是以三角形为单位……
						}
						index[108] = 0;//中心点
						index[109] = 36;//1
						index[110] = 1;//2
					}
					//准备顶点
					f2dGraphics2DVertex vert[vertcount];
					{
						vert[0].x = 0.0f;
						vert[0].y = 0.0f;
						vert[0].z = 0.5f;//2D下固定z0.5
						vert[0].color = fillColor.argb;
						vert[0].u = 0.0f; vert[0].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						float angle;
						for (int i = 1; i < vertcount; i++) {
							//椭圆参方
							angle = 10.0f * (i - 1) * LDEGREE2RAD;
							vert[i].x = cc.a * std::cosf(angle);
							vert[i].y = cc.b * std::sinf(angle);
							vert[i].z = 0.5f;//2D下固定z0.5
							vert[i].color = fillColor.argb;
							vert[i].u = 0.0f; vert[i].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						}
					}
					// 变换
					{
						float tSin, tCos;
						SinCos(cc.absrot, tSin, tCos);
						for (int i = 0; i < vertcount; i++)
						{
							fFloat tx = vert[i].x * tCos - vert[i].y * tSin,
								ty = vert[i].x * tSin + vert[i].y * tCos;
							vert[i].x = tx + cc.absx;
							vert[i].y = ty + cc.absy;
						}
					}
					//绘制
					graph->DrawRaw(nullptr, vertcount, indexcount, vert, index, false);
					break;
				}
				case GameObjectColliderType::Diamond:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{         0.0f, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{         0.0f,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Triangle:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },//和第三个点相同
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Point:
					//点使用直径1的圆来替代
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), 0.5f, fillColor, fillColor, 3);
					break;
				}
			}
#else
			if (p->rect || p->a != p->b)
			{
				fcyVec2 tHalfSize((float)p->a, (float)p->b);

				// 计算出矩形的4个顶点
				f2dGraphics2DVertex tFinalPos[4] =
				{
					{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
					{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
					{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
					{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
				};

				float tSin, tCos;
				SinCos((float)p->rot, tSin, tCos);

				// 变换
				for (int i = 0; i < 4; i++)
				{
					fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
						ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
					tFinalPos[i].x = tx + (float)p->x; tFinalPos[i].y = ty + (float)p->y;
				}

				graph->DrawQuad(nullptr, tFinalPos);
			}
			else
			{
				grender->FillCircle(graph, fcyVec2((float)p->x, (float)p->y), (float)p->a, fillColor, fillColor,
					p->a < 10 ? 3 : (p->a < 20 ? 6 : 8));
			}
#endif // USING_ADVANCE_COLLIDER
		}

		p = p->pCollisionNext;
	}
	//*/
	
#ifdef USING_MULTI_GAME_WORLD
	lua_Integer world = GetWorldFlag();
#endif // USING_MULTI_GAME_WORLD
	for (auto& p : m_ColliList[groupId]) {
#ifdef USING_MULTI_GAME_WORLD
		if (p->colli && CheckWorld(p->world, world))
#else // USING_MULTI_GAME_WORLD
		if (p->colli)
#endif // USING_MULTI_GAME_WORLD
		{
#ifdef USING_ADVANCE_COLLIDER
			for (int select = 0; select < MAX_COLLIDERS_COUNT; select++) {
				GameObjectCollider cc = p->colliders[select];
				if (cc.type == GameObjectColliderType::None) { break; }

				cc.caloffset(p->x, p->y, p->rot);
				switch (cc.type)
				{
				case GameObjectColliderType::Circle:
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), cc.a, fillColor, fillColor,
						cc.a < 10.0 ? 6 : (cc.a < 20.0 ? 16 : 32));
					break;
				case GameObjectColliderType::OBB:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出矩形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Ellipse:
				{
					const int vertcount = 37;//分割36份，还要中心一个点
					const int indexcount = 111;//37*3加一个组成封闭图形
					//准备顶点索引
					fuShort index[indexcount];
					{
						for (int i = 0; i < (vertcount - 1); i++) {
							index[i * 3] = 0;//中心点
							index[i * 3 + 1] = i;//1
							index[i * 3 + 2] = i + 1;//2
							//index[i * 3 + 3] = i + 1;//2 //fancy2d貌似不是以三角形为单位……
						}
						index[108] = 0;//中心点
						index[109] = 36;//1
						index[110] = 1;//2
					}
					//准备顶点
					f2dGraphics2DVertex vert[vertcount];
					{
						vert[0].x = 0.0f;
						vert[0].y = 0.0f;
						vert[0].z = 0.5f;//2D下固定z0.5
						vert[0].color = fillColor.argb;
						vert[0].u = 0.0f; vert[0].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						float angle;
						for (int i = 1; i < vertcount; i++) {
							//椭圆参方
							angle = 10.0f * (i - 1) * LDEGREE2RAD;
							vert[i].x = cc.a * std::cosf(angle);
							vert[i].y = cc.b * std::sinf(angle);
							vert[i].z = 0.5f;//2D下固定z0.5
							vert[i].color = fillColor.argb;
							vert[i].u = 0.0f; vert[i].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						}
					}
					// 变换
					{
						float tSin, tCos;
						SinCos(cc.absrot, tSin, tCos);
						for (int i = 0; i < vertcount; i++)
						{
							fFloat tx = vert[i].x * tCos - vert[i].y * tSin,
								ty = vert[i].x * tSin + vert[i].y * tCos;
							vert[i].x = tx + cc.absx;
							vert[i].y = ty + cc.absy;
						}
					}
					//绘制
					graph->DrawRaw(nullptr, vertcount, indexcount, vert, index, false);
					break;
				}
				case GameObjectColliderType::Diamond:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{         0.0f, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{         0.0f,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Triangle:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },//和第三个点相同
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Point:
					//点使用直径1的圆来替代
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), 0.5f, fillColor, fillColor, 3);
					break;
				}
			}
#else
			if (p->rect || p->a != p->b)
			{
				fcyVec2 tHalfSize((float)p->a, (float)p->b);

				// 计算出矩形的4个顶点
				f2dGraphics2DVertex tFinalPos[4] =
				{
					{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
					{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
					{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
					{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
				};

				float tSin, tCos;
				SinCos((float)p->rot, tSin, tCos);

				// 变换
				for (int i = 0; i < 4; i++)
				{
					fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
						ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
					tFinalPos[i].x = tx + (float)p->x; tFinalPos[i].y = ty + (float)p->y;
				}

				graph->DrawQuad(nullptr, tFinalPos);
			}
			else
			{
				grender->FillCircle(graph, fcyVec2((float)p->x, (float)p->y), (float)p->a, fillColor, fillColor,
					p->a < 10 ? 3 : (p->a < 20 ? 6 : 8));
			}
#endif // USING_ADVANCE_COLLIDER
		}
	}
}

void GameObjectPool::DrawGroupCollider2(int groupId, fcyColor fillColor)
{
	LAPP.GetRenderDev()->ClearZBuffer();

	fcyRefPointer<f2dGeometryRenderer> grender = LAPP.GetGeometryRenderer();
	fcyRefPointer<f2dGraphics2D> graph = LAPP.GetGraphics2D();
	
	f2dBlendState stState = graph->GetBlendState();
	f2dBlendState stStateClone = stState;
	stStateClone.SrcBlend = F2DBLENDFACTOR_SRCALPHA;
	stStateClone.DestBlend = F2DBLENDFACTOR_INVSRCALPHA;
	stStateClone.BlendOp = F2DBLENDOPERATOR_ADD;
	graph->SetBlendState(stStateClone);
	F2DGRAPH2DBLENDTYPE txState = graph->GetColorBlendType();
	graph->SetColorBlendType(F2DGRAPH2DBLENDTYPE_ADD);//修复反色混合模式的时候会出现颜色异常的问题

	/*
	GameObject* p = m_pCollisionListHeader[groupId].pCollisionNext;
	GameObject* pTail = &m_pCollisionListTail[groupId];
	lua_Integer world = GetWorldFlag();
	while (p && p != pTail)
	{
		if (p->colli && CheckWorld(p->world, world))
		{
#ifdef USING_ADVANCE_COLLIDER
			for (int select = 0; select < MAX_COLLIDERS_COUNT; select++) {
				GameObjectCollider cc = p->colliders[select];
				if (cc.type == GameObjectColliderType::None) { break; }

				cc.caloffset(p->x, p->y, p->rot);
				switch (cc.type)
				{
				case GameObjectColliderType::Circle:
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), cc.a, fillColor, fillColor,
						cc.a < 10.0 ? 6 : (cc.a < 20.0 ? 16 : 32));
					break;
				case GameObjectColliderType::OBB:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出矩形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Ellipse:
				{
					const int vertcount = 37;//分割36份，还要中心一个点
					const int indexcount = 111;//37*3加一个组成封闭图形
					//准备顶点索引
					fuShort index[indexcount];
					{
						for (int i = 0; i < (vertcount - 1); i++) {
							index[i * 3] = 0;//中心点
							index[i * 3 + 1] = i;//1
							index[i * 3 + 2] = i + 1;//2
							//index[i * 3 + 3] = i + 1;//2 //fancy2d貌似不是以三角形为单位……
						}
						index[108] = 0;//中心点
						index[109] = 36;//1
						index[110] = 1;//2
					}
					//准备顶点
					f2dGraphics2DVertex vert[vertcount];
					{
						vert[0].x = 0.0f;
						vert[0].y = 0.0f;
						vert[0].z = 0.5f;//2D下固定z0.5
						vert[0].color = fillColor.argb;
						vert[0].u = 0.0f; vert[0].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						float angle;
						for (int i = 1; i < vertcount; i++) {
							//椭圆参方
							angle = 10.0f * (i - 1) * LDEGREE2RAD;
							vert[i].x = cc.a * std::cosf(angle);
							vert[i].y = cc.b * std::sinf(angle);
							vert[i].z = 0.5f;//2D下固定z0.5
							vert[i].color = fillColor.argb;
							vert[i].u = 0.0f; vert[i].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						}
					}
					// 变换
					{
						float tSin, tCos;
						SinCos(cc.absrot, tSin, tCos);
						for (int i = 0; i < vertcount; i++)
						{
							fFloat tx = vert[i].x * tCos - vert[i].y * tSin,
								ty = vert[i].x * tSin + vert[i].y * tCos;
							vert[i].x = tx + cc.absx;
							vert[i].y = ty + cc.absy;
						}
					}
					//绘制
					graph->DrawRaw(nullptr, vertcount, indexcount, vert, index, false);
					break;
				}
				case GameObjectColliderType::Diamond:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{         0.0f, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{         0.0f,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Triangle:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },//和第三个点相同
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Point:
					//点使用直径1的圆来替代
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), 0.5f, fillColor, fillColor, 3);
					break;
				}
			}
#endif // USING_ADVANCE_COLLIDER
		}

		p = p->pCollisionNext;
	}
	//*/
	
#ifdef USING_MULTI_GAME_WORLD
	lua_Integer world = GetWorldFlag();
#endif // USING_MULTI_GAME_WORLD
	for (auto& p : m_ColliList[groupId]) {
#ifdef USING_MULTI_GAME_WORLD
		if (p->colli && CheckWorld(p->world, world))
#else // USING_MULTI_GAME_WORLD
		if (p->colli)
#endif // USING_MULTI_GAME_WORLD
		{
#ifdef USING_ADVANCE_COLLIDER
			for (int select = 0; select < MAX_COLLIDERS_COUNT; select++) {
				GameObjectCollider cc = p->colliders[select];
				if (cc.type == GameObjectColliderType::None) { break; }

				cc.caloffset(p->x, p->y, p->rot);
				switch (cc.type)
				{
				case GameObjectColliderType::Circle:
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), cc.a, fillColor, fillColor,
						cc.a < 10.0 ? 6 : (cc.a < 20.0 ? 16 : 32));
					break;
				case GameObjectColliderType::OBB:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出矩形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Ellipse:
				{
					const int vertcount = 37;//分割36份，还要中心一个点
					const int indexcount = 111;//37*3加一个组成封闭图形
					//准备顶点索引
					fuShort index[indexcount];
					{
						for (int i = 0; i < (vertcount - 1); i++) {
							index[i * 3] = 0;//中心点
							index[i * 3 + 1] = i;//1
							index[i * 3 + 2] = i + 1;//2
							//index[i * 3 + 3] = i + 1;//2 //fancy2d貌似不是以三角形为单位……
						}
						index[108] = 0;//中心点
						index[109] = 36;//1
						index[110] = 1;//2
					}
					//准备顶点
					f2dGraphics2DVertex vert[vertcount];
					{
						vert[0].x = 0.0f;
						vert[0].y = 0.0f;
						vert[0].z = 0.5f;//2D下固定z0.5
						vert[0].color = fillColor.argb;
						vert[0].u = 0.0f; vert[0].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						float angle;
						for (int i = 1; i < vertcount; i++) {
							//椭圆参方
							angle = 10.0f * (i - 1) * LDEGREE2RAD;
							vert[i].x = cc.a * std::cosf(angle);
							vert[i].y = cc.b * std::sinf(angle);
							vert[i].z = 0.5f;//2D下固定z0.5
							vert[i].color = fillColor.argb;
							vert[i].u = 0.0f; vert[i].v = 0.0f;//没有使用到贴图，uv是多少无所谓
						}
					}
					// 变换
					{
						float tSin, tCos;
						SinCos(cc.absrot, tSin, tCos);
						for (int i = 0; i < vertcount; i++)
						{
							fFloat tx = vert[i].x * tCos - vert[i].y * tSin,
								ty = vert[i].x * tSin + vert[i].y * tCos;
							vert[i].x = tx + cc.absx;
							vert[i].y = ty + cc.absy;
						}
					}
					//绘制
					graph->DrawRaw(nullptr, vertcount, indexcount, vert, index, false);
					break;
				}
				case GameObjectColliderType::Diamond:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{         0.0f, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{         0.0f,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Triangle:
				{
					fcyVec2 tHalfSize(cc.a, cc.b);
					// 计算出菱形的4个顶点
					f2dGraphics2DVertex tFinalPos[4] =
					{
						{  tHalfSize.x,         0.0f, 0.5f, fillColor.argb, 0.0f, 0.0f },
						{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
						{ -tHalfSize.x,  tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },//和第三个点相同
					};
					float tSin, tCos;
					SinCos(cc.absrot, tSin, tCos);
					// 变换
					for (int i = 0; i < 4; i++)
					{
						fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
							ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
						tFinalPos[i].x = tx + cc.absx;
						tFinalPos[i].y = ty + cc.absy;
					}
					graph->DrawQuad(nullptr, tFinalPos);
					break;
				}
				case GameObjectColliderType::Point:
					//点使用直径1的圆来替代
					grender->FillCircle(graph, fcyVec2(cc.absx, cc.absy), 0.5f, fillColor, fillColor, 3);
					break;
				}
			}
#endif // USING_ADVANCE_COLLIDER
		}
	}
	
	graph->SetBlendState(stState);
	graph->SetColorBlendType(txState);
}

int GameObjectPool::PushCurrentObject(lua_State* L) LNOEXCEPT
{
	if (!m_pCurrentObject)
	{
		lua_pushnil(L);
		return 1;
	}
	GETOBJTABLE;
	lua_rawgeti(L, -1, m_pCurrentObject->id + 1);  // ot t(object)
	return 1;
}

#pragma endregion
