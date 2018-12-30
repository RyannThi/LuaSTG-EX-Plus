#include "GameObjectPool.h"
#include "GameObjectPropertyHash.inl"
#include "AppFrame.h"
#include "CollisionDetect.h"

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
	// ������uidΪ����
	return p1->uid < p2->uid;
}

static inline bool RenderListSortFunc(GameObject* p1, GameObject* p2)LNOEXCEPT
{
	// layerС�Ŀ�ǰ����layer��ͬ�����uid��
	return (p1->layer < p2->layer) || ((p1->layer == p2->layer) && (p1->uid < p2->uid));
}

static inline bool CollisionCheck(GameObject* p1, GameObject* p2)LNOEXCEPT
{
	if (!p1->colli || !p2->colli)
	return false;

	if ((p1->x - p1->col_r >= p2->x + p2->col_r) ||
		(p1->x + p1->col_r <= p2->x - p2->col_r) ||
		(p1->y - p1->col_r >= p2->y + p2->col_r) ||
		(p1->y + p1->col_r <= p2->y - p2->col_r))
	{
		return false;
	}

	fcyVec2 pos1((float)p1->x, (float)p1->y), pos2((float)p2->x, (float)p2->y);
	fcyVec2 size1((float)p1->a, (float)p1->b), size2((float)p2->a, (float)p2->b); 
	if (!p1->rect && !p2->rect){
		bool rt = ElliTest(pos1, p1->a, p1->b, p1->rot, pos2, p2->a, p2->b, p2->rot);
		return rt;
	}
	float r1((float)p1->col_r), r2((float)p2->col_r);

	if (!CircleHitTest(pos1, r1, pos2, r2))
		return false;


	if (p1->rect)
	{
		if (p2->rect)
			return OBBHitTest(pos1, size1, (float)p1->rot, pos2, size2, (float)p2->rot);
		else
			return OBBCircleHitTest(pos1, size1, (float)p1->rot, pos2, r2);
	}
	else
	{
		if (p2->rect)
			return OBBCircleHitTest(pos2, size2, (float)p2->rot, pos1, r1);
		else
			return true;  
	}

	
}

////////////////////////////////////////////////////////////////////////////////
/// GameObjectBentLaser
////////////////////////////////////////////////////////////////////////////////
#pragma region GameObjectBentLaser
static fcyMemPool<sizeof(GameObjectBentLaser)> s_GameObjectBentLaserPool(1024);

GameObjectBentLaser* GameObjectBentLaser::AllocInstance()
{
	// ! Ǳ��bad_alloc
	GameObjectBentLaser* pRet = new(s_GameObjectBentLaserPool.Alloc()) GameObjectBentLaser();
	return pRet;
}

void GameObjectBentLaser::FreeInstance(GameObjectBentLaser* p)
{
	p->~GameObjectBentLaser();
	s_GameObjectBentLaserPool.Free(p);
}

GameObjectBentLaser::GameObjectBentLaser()
{
}

GameObjectBentLaser::~GameObjectBentLaser()
{
}

bool GameObjectBentLaser::Update(size_t id, int length, float width,bool active)LNOEXCEPT
{
	GameObject* p = LPOOL.GetPooledObject(id);
	if (!p)
		return false;
	if (length <= 1)
	{
		LERROR("lstgBentLaserData: ��Ч�Ĳ���length");
		return false;
	}

	// �Ƴ�����Ľڵ㣬��֤������length��Χ��
	while (m_Queue.IsFull() || m_Queue.Size() >= (size_t)length)
	{
		LaserNode tLastPop;
		m_Queue.Pop(tLastPop);

		// �����ܳ���
		if (!m_Queue.IsEmpty())
		{
			LaserNode &tFront = m_Queue.Front();
			if (tLastPop.active && tFront.active){
				m_fLength -= tFront.dis;
			}
			tFront.dis = 0;			
		}
	}

	// �����½ڵ�
	if (m_Queue.Size() < (size_t)length)
	{
		LaserNode tNode;
		tNode.pos.Set((float)p->x, (float)p->y);
		tNode.half_width = width / 2.f;
		tNode.active = active;
		m_Queue.Push(tNode);

		// �����ܳ���
		if (m_Queue.Size() > 1)
		{
			LaserNode& tNodeLast = m_Queue.Back();
			LaserNode& tNodeBeforeLast = m_Queue[m_Queue.Size() - 2];
			fcyVec2 dpos = tNodeLast.pos - tNodeBeforeLast.pos;
			float l = dpos.Length();
			tNodeLast.dis = l;
			if (l == 0){
				tNodeLast.rot = tNodeBeforeLast.rot;
			}
			else{
				tNodeLast.rot = dpos.CalcuAngle();
				if (tNodeLast.active && active){
					m_fLength += l;
				}
			}
			CompileNode(m_Queue.Size() - 1);
			CompileNode(m_Queue.Size() - 2);
		}
		else{
			LaserNode& tNodeLast = m_Queue.Back();
			tNodeLast.rot = atan2(p->vy, p->vx);
			tNodeLast.dis = 0;
		}
	}

	return true;
}

bool GameObjectBentLaser::UpdateByNode(size_t id, int node ,int length, float width, bool active)LNOEXCEPT
{
	GameObject* p = LPOOL.GetPooledObject(id);
	if (!p)
		return false;
	if (length <= 1)
	{
		LERROR("lstgBentLaserData: ��Ч�Ĳ���length");
		return false;
	}

	if (node < 0){
		node = m_Queue.Size() + node;
	}
	
	// �����½ڵ�
	if (node<m_Queue.Size() && node>=0)
	{
		LaserNode& tNode = m_Queue[node];
		tNode.active = active;

		UpdateLength();
		RecalRot();
	}

	return true;
}


bool GameObjectBentLaser::RecalRot()LNOEXCEPT
{
	size_t sz = m_Queue.Size();
	for (size_t i = 0; i < sz - 1; ++i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i + 1];
		fcyVec2 dpos = next.pos - cur.pos;
		float l = dpos.Length();
		if (l == 0){
			next.rot = cur.rot;
		}
		else{
			next.rot = dpos.CalcuAngle();
		}
		if (i == 0){
			cur.rot = next.rot;
		}
	}
	for (size_t i = 0; i < sz ; ++i){
		CompileNode(i);
	}
	return true;
}

bool GameObjectBentLaser::CompileNode(size_t i)LNOEXCEPT
{
	LaserNode& cur = m_Queue[i];
	int p = m_Queue.Size() - 1;

	float a, b, c, d;
	cur.sharp = false;

	a = cos(cur.rot);
	b = sin(cur.rot);
	
	if (i == 0){
		cur.x_dir = b ;
		cur.y_dir = -a ;
		return true;
	}

	if (i < p){
		LaserNode& next = m_Queue[i+1];
		c = cos(next.rot);
		d = sin(next.rot);
	}
	else{
		cur.x_dir = b;
		cur.y_dir = -a;
		return true;
	}

	float d1 = a*d - c*b;
	if (d1 < 0.3 && d1 > -0.3)
	{
		float d2 = a*c + b*d;
		if (d2 > 0){
			if (d1<0.01 && d1>-0.01)
			{
				cur.x_dir = (b+d)/2;
				cur.y_dir = (-a-c)/2;
				return true;
			}
		}
		else{
			cur.sharp = true;
			c = -c;
			d = -d;
			d1 = -d1;
		}
	}
	cur.x_dir = (a - c) / d1;
	cur.y_dir = (b - d) / d1;

	return true;
}


void GameObjectBentLaser::Release()LNOEXCEPT
{
}

bool GameObjectBentLaser::Render(const char* tex_name, BlendMode blend, fcyColor c, float tex_left, float tex_top, float tex_width, float tex_height, float scale)LNOEXCEPT
{
	// ����ֻ��һ���ڵ�����
	if (m_Queue.Size() <= 1)
		return true;

	fcyRefPointer<ResTexture> pTex = LRES.FindTexture(tex_name);
	if (!pTex)
	{
		LERROR("lstgBentLaserData: �Ҳ���������Դ'%m'", tex_name);
		return false;
	}

	f2dGraphics2DVertex renderVertex[4] = {
		{ 0, 0, 0.5f, c.argb, 0, tex_top },
		{ 0, 0, 0.5f, c.argb, 0, tex_top },
		{ 0, 0, 0.5f, c.argb, 0, tex_top + tex_height },
		{ 0, 0, 0.5f, c.argb, 0, tex_top + tex_height }
	};
	fuInt org_c = c.argb;
	c.a = 0;
	fuInt trans_c = c.argb;

	float tVecLength = 0;
	bool flip = false;
	for (size_t i = 0; i < m_Queue.Size() - 1; ++i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i + 1];

		if (!cur.active || !next.active){
			continue;
		}

		if (cur.sharp){
			flip = !flip;
		}


		float expX1 = cur.x_dir * scale * cur.half_width;
		float expY1 = cur.y_dir * scale * cur.half_width;

		if (flip){
			expX1 = -expX1;
			expY1 = -expY1;
		}
		float u = tex_left + tVecLength / m_fLength * tex_width;

		renderVertex[0].x = cur.pos.x + expX1;
		renderVertex[0].y = cur.pos.y + expY1;
		renderVertex[0].u = u;
		renderVertex[0].color = cur.active ? org_c : trans_c;
		renderVertex[3].x = cur.pos.x - expX1;
		renderVertex[3].y = cur.pos.y - expY1;
		renderVertex[3].u = u;
		renderVertex[3].color = cur.active ? org_c : trans_c;
		float expX2 = next.x_dir * scale * cur.half_width;
		float expY2 = next.y_dir * scale * cur.half_width;

		if (flip){
			expX2 = -expX2;
			expY2 = -expY2;
		}
		float lenOffsetA = next.dis;
		tVecLength += lenOffsetA;
		u = tex_left + tVecLength / m_fLength * tex_width;

		renderVertex[1].x = next.pos.x + expX2;
		renderVertex[1].y = next.pos.y + expY2;
		renderVertex[1].u = u;
		renderVertex[1].color = next.active ? org_c : trans_c;
		renderVertex[2].x = next.pos.x - expX2;
		renderVertex[2].y = next.pos.y - expY2;
		renderVertex[2].u = u;
		renderVertex[2].color = next.active ? org_c : trans_c;



	//	fcyVec2 offsetA = cur.pos - next.pos;
		//float lenOffsetA = next.dis;// offsetA.Length();
	//	fcyVec2 offsetNA;
	//	offsetNA.Set2(-1, next.rot);
		//if (lenOffsetA < 0.0001f && i + 1 != m_Queue.Size() - 1)
		//	continue;

		// ��������ϵ���չ����(��ת270��)

		/*
	//	fcyVec2 expandVec = offsetNA;//offsetA.GetNormalize();
	//	std::swap(expandVec.x, expandVec.y);
	//	expandVec.y = -expandVec.y;

		if (i == 0)  // ����ǵ�һ���ڵ㣬���������չʹ��expandVec����
		{
			float expX = cur.x_dir * scale * cur.half_width;
			float expY = cur.y_dir * scale * cur.half_width;
			renderVertex[0].x = cur.pos.x + expX;
			renderVertex[0].y = cur.pos.y + expY;
			renderVertex[0].u = tex_left;
			renderVertex[3].x = cur.pos.x - expX;
			renderVertex[3].y = cur.pos.y - expY;
			renderVertex[3].u = tex_left;
		}
		else  // ���򣬿���1��2
		{
			renderVertex[0].x = renderVertex[1].x;
			renderVertex[0].y = renderVertex[1].y;
			renderVertex[0].u = renderVertex[1].u;
			renderVertex[3].x = renderVertex[2].x;
			renderVertex[3].y = renderVertex[2].y;
			renderVertex[3].u = renderVertex[2].u;
		}


		
		if (i == m_Queue.Size() - 2)  
		{
			float expX = expandVec.x * scale * next.half_width;
			float expY = expandVec.y * scale * next.half_width;
			renderVertex[1].x = next.pos.x + expX;
			renderVertex[1].y = next.pos.y + expY;
			renderVertex[1].u = tex_left + tex_width;
			renderVertex[2].x = next.pos.x - expX;
			renderVertex[2].y = next.pos.y - expY;
			renderVertex[2].u = tex_left + tex_width;
		}
		else  
		{
			
			float expX, expY;
			LaserNode& afterNext = m_Queue[i + 2];

			fcyVec2 offsetB = afterNext.pos - next.pos;
			fcyVec2 offsetNB;
			offsetNB.Set2(1, afterNext.rot);
			fcyVec2 angleBisect = offsetNA+offsetNB;// offsetA.GetNormalize() + offsetB.GetNormalize();
			float angleBisectLen = angleBisect.Length();

			if (angleBisectLen < 0.00002f || angleBisectLen > 1.99998f) 
			{
				expX = expandVec.x * scale * next.half_width;
				expY = expandVec.y * scale * next.half_width;
			}
			else // �����ƽ���ߵ������߾���Ϊnext.half_width * scale��ƫ����
			{
				angleBisect *= (1 / angleBisectLen);  // angleBisect.Normalize();
				float t = angleBisect * offsetA.GetNormalize();
				float l = scale * next.half_width;
				float expandDelta = sqrt(l * l / (1.f - t * t));
				if (l && expandDelta/l > 2)expandDelta = 2*l;
				expX = angleBisect.x * expandDelta;
				expY = angleBisect.y * expandDelta;
			}
			

			float u = tex_left + tVecLength / m_fLength * tex_width;
			renderVertex[1].x = next.pos.x + expX;
			renderVertex[1].y = next.pos.y + expY;
			renderVertex[1].u = u;
			renderVertex[2].x = next.pos.x - expX;
			renderVertex[2].y = next.pos.y - expY;
			renderVertex[2].u = u;


			float cross1 = fcyVec2(renderVertex[1].x - renderVertex[0].x, renderVertex[1].y - renderVertex[0].y) *
				fcyVec2(renderVertex[2].x - renderVertex[3].x, renderVertex[2].y - renderVertex[3].y);
			float cross2 = fcyVec2(renderVertex[2].x - renderVertex[0].x, renderVertex[2].y - renderVertex[0].y) *
				fcyVec2(renderVertex[1].x - renderVertex[3].x, renderVertex[1].y - renderVertex[3].y);
			if (cross2 > cross1)
			{
				std::swap(renderVertex[1].x, renderVertex[2].x);
				std::swap(renderVertex[1].y, renderVertex[2].y);
			}	
			
		}*/

		if (!LAPP.RenderTexture(pTex, blend, renderVertex))
			return false;
	}
	return true;
}

bool GameObjectBentLaser::CollisionCheck(float x, float y, float rot, float a, float b, bool rect)LNOEXCEPT
{
	// ����ֻ��һ���ڵ�����
	if (m_Queue.Size() <= 1)
		return false;

	GameObject testObjA;
	testObjA.Reset();
	testObjA.rot = 0.;
	testObjA.rect = false;

	GameObject testObjB;
	testObjB.Reset();
	testObjB.x = x;
	testObjB.y = y;
	testObjB.rot = rot;
	testObjB.a = a;
	testObjB.b = b;
	testObjB.rect = rect;
	testObjB.UpdateCollisionCirclrRadius();
	int sn = m_Queue.Size();
	for (size_t i = 0; i < sn; ++i)
	{
		LaserNode& n = m_Queue[i];
		if (!n.active)continue;
		if (i > 0){
			LaserNode& last = m_Queue[i - 1];
			if (!last.active){
				float df = n.dis;
				if (df > n.half_width){
					fcyVec2 c = (last.pos + n.pos)*0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.rect = true;
					testObjA.rot = n.rot;
					testObjA.a = df / 2;
					testObjA.b = n.half_width;
					testObjA.UpdateCollisionCirclrRadius();
					if (::CollisionCheck(&testObjA, &testObjB))
						return true;

				}
			}
		}
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.a = testObjA.b = n.half_width;
		testObjA.rect = false;
		testObjA.UpdateCollisionCirclrRadius();
		if (::CollisionCheck(&testObjA, &testObjB))
			return true;
	}
	return false;
}

bool GameObjectBentLaser::CollisionCheckW(float x, float y, float rot, float a, float b, bool rect,float width)LNOEXCEPT
{
	// ����ֻ��һ���ڵ�����
	if (m_Queue.Size() <= 1)
	return false;
	width = width / 2;
	GameObject testObjA;
	testObjA.Reset();
	testObjA.rot = 0.;
	testObjA.rect = false;

	GameObject testObjB;
	testObjB.Reset();
	testObjB.x = x;
	testObjB.y = y;
	testObjB.rot = rot;
	testObjB.a = a;
	testObjB.b = b;
	testObjB.rect = rect;
	testObjB.UpdateCollisionCirclrRadius();
	int sn = m_Queue.Size();
	for (size_t i = 0; i < sn; ++i)
	{
		LaserNode& n = m_Queue[i];
		if (!n.active)continue;
		if (i > 0){
			LaserNode& last = m_Queue[i - 1];
			if (!last.active){
				float df = n.dis;
				if (df > width){
					fcyVec2 c = (last.pos + n.pos)*0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.rect = true;
					testObjA.rot = n.rot;
					testObjA.a = df / 2;
					testObjA.b = width;
					testObjA.UpdateCollisionCirclrRadius();
					if (::CollisionCheck(&testObjA, &testObjB))
						return true;

				}
			}
		}
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.a = testObjA.b = width;
		testObjA.rect = false;
		testObjA.UpdateCollisionCirclrRadius();
		if (::CollisionCheck(&testObjA, &testObjB))
			return true;
	}
	return false;
}


bool GameObjectBentLaser::BoundCheck()LNOEXCEPT
{
	fcyRect tBound = LPOOL.GetBound();
	for (size_t i = 0; i < m_Queue.Size(); ++i)
	{
		LaserNode& n = m_Queue[i];
		if (n.pos.x >= tBound.a.x && n.pos.x <= tBound.b.x && n.pos.y <= tBound.a.y && n.pos.y >= tBound.b.y)
			return true;
	}
	// Խ��ʱ����false��ֻ�е����еĵ�ĻԽ��ŷ���false
	return false;
}

//��luaһֱ��index��1��ʼ
bool LuaSTGPlus::GameObjectBentLaser::UpdatePositionByList(lua_State *L,int length,float width, int index,bool revert) LNOEXCEPT// ... t(list)
{
	// ... t(list)
	int push_count = 0;//�Բ���ͷ�Ľڵ����� 

	for (int i = 0; i < length; i++)
	{
		//���x,y
		lua_rawgeti(L, -1, i + 1);// ... t(list) t(object)
		lua_pushstring(L, "x");// ... t(list) t(object) 'x'
		lua_gettable(L, -2);// ... t(list) t(object) x
		float x=luaL_optnumber(L, -1, 0.0);
		lua_pop(L, 1);
		lua_pushstring(L, "y");// ... t(list) t(object) 'y'
		lua_gettable(L, -2);// ... t(list) t(object) y
		float y = luaL_optnumber(L, -1, 0.0);// ... t(list) t(object) y
		lua_pop(L, 2);// ... t(list)

		//�õ�index
		//���㴦�ڶ���ǰ��
		int cindex = push_count+ index-1 + (revert?-i:i);
		if (cindex < 0){
			int j = cindex;
			LaserNode np;
			np.active = false;
			while (j > 0){
				m_Queue.Push(np);
				j--;
				push_count++;
			}
		}

		int size = m_Queue.Size();
		//���㴦�ڶ��к��
		if (cindex >= size){
			int j = cindex - size+1;
			LaserNode np;
			np.active = false;
			while (j > 0){
				m_Queue.PushBack(np);
				j--;
			}
		}
		size = m_Queue.Size();
		//���ö���
		LaserNode *tNode = &m_Queue[size-cindex-1];
		tNode->active = true;
		tNode->half_width = width / 2;
		tNode->pos.Set(x, y);
	}
	UpdateLength();
	RecalRot();
	return true;
}

int LuaSTGPlus::GameObjectBentLaser::SampleL(lua_State *L, float length) LNOEXCEPT
{
	//����һ������
	lua_newtable(L); //... t(list)
	// ����û�нڵ�����
	if (m_Queue.Size() <= 1)
	return true;

	float fLeft = 0;// ʣ�೤��
	int count = 0;

	float tVecLength = 0;
	for (size_t i = m_Queue.Size() - 1; i > 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i - 1];

		fcyVec2 vn = cur.pos;
		fcyVec2 offsetA = next.pos - cur.pos;
		float lenOffsetA = offsetA.Length();
		fcyVec2 expandVec = offsetA.GetNormalize();
		float angle = expandVec.CalcuAngle()*LRAD2DEGREE+180;
		while (fLeft - lenOffsetA <= 0){
			vn = expandVec * fLeft + cur.pos;
			lua_newtable(L); //... t(list) t(object)
			lua_pushnumber(L, vn.x); //... t(list) t(object) <x>
			lua_setfield(L, -2, "x");//... t(list) t(object)
			lua_pushnumber(L, vn.y); //... t(list) t(object) <y>
			lua_setfield(L, -2, "y");//... t(list) t(object)
			lua_pushnumber(L, angle); //... t(list) t(object) <angle>
			lua_setfield(L, -2, "rot");//... t(list) t(object)
			count++;
			lua_rawseti(L, -2, count);//... t(list)
			fLeft = fLeft + length;
		}
		fLeft = fLeft - lenOffsetA;
	}
	return true;
}

int LuaSTGPlus::GameObjectBentLaser::UpdateLength() LNOEXCEPT
{
	m_fLength = 0;
	if (m_Queue.Size() > 0){
		LaserNode& cur = m_Queue[0];
		cur.dis = 0;
	}
	for (size_t i = m_Queue.Size() - 1; i > 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i - 1];
		fcyVec2 offsetA = next.pos - cur.pos;
		float lenOffsetA = offsetA.Length();
		cur.dis = lenOffsetA;
		if (cur.active && next.active){
			m_fLength += lenOffsetA;
		}
	}
	return 1;
}

int LuaSTGPlus::GameObjectBentLaser::SampleT(lua_State *L, float delay) LNOEXCEPT
{
	//����һ������
	lua_newtable(L); //... t(list)
	// ����û�нڵ�����
	if (m_Queue.Size() <= 1)
		return true;

	float fLeft = 0;// ʣ�೤��
	int count = 0;

	float tVecLength = 0;
	for (size_t i = m_Queue.Size() - 1; i > 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i - 1];


		fcyVec2 vn = cur.pos;
		fcyVec2 offsetA = next.pos - cur.pos;
		float lenOffsetA = offsetA.Length();
		float angle = offsetA.CalcuAngle()*LRAD2DEGREE+180;
		while (fLeft - 1 <= 0){
			vn = offsetA * fLeft + cur.pos;
			lua_newtable(L); //... t(list) t(object)
			lua_pushnumber(L, vn.x); //... t(list) t(object) <x>
			lua_setfield(L, -2, "x");//... t(list) t(object)
			lua_pushnumber(L, vn.y); //... t(list) t(object) <y>
			lua_setfield(L, -2, "y");//... t(list) t(object)
			lua_pushnumber(L, angle); //... t(list) t(object) <angle>
			lua_setfield(L, -2, "rot");//... t(list) t(object)
			count++;
			lua_rawseti(L, -2, count);//... t(list)
			fLeft = fLeft + delay;
		}
		fLeft = fLeft - 1;
	}
	return true;
}


void LuaSTGPlus::GameObjectBentLaser::SetAllWidth(float width) LNOEXCEPT
{
	//if (m_Queue.Size() == 0)return;
	for (int i = (unsigned int)m_Queue.Size() - 1; i >= 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		cur.half_width = width / 2.f;
	}
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////
/// GameObject
////////////////////////////////////////////////////////////////////////////////
#pragma region GameObject

bool GameObject::ChangeResource(const char* res_name)
{
	//LASSERT(!res);

	fcyRefPointer<ResSprite> tSprite = LRES.FindSprite(res_name);
	if (tSprite)
	{
		res = tSprite;
		res->AddRef();
		a = tSprite->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tSprite->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
		rect = tSprite->IsRectangle();
		UpdateCollisionCirclrRadius();
		return true;
	}

	fcyRefPointer<ResAnimation> tAnimation = LRES.FindAnimation(res_name);
	if (tAnimation)
	{
		res = tAnimation;
		res->AddRef();
		a = tAnimation->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tAnimation->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
		rect = tAnimation->IsRectangle();
		UpdateCollisionCirclrRadius();
		return true;
	}

	fcyRefPointer<ResParticle> tParticle = LRES.FindParticle(res_name);

	if(tParticle)
	{
		res = tParticle;
		if (!(ps = tParticle->AllocInstance()))
		{
			res = nullptr;
			LERROR("�޷��������ӳأ��ڴ治��");
			return false;
		}
		ps->SetInactive();
		ps->SetCenter(fcyVec2((float)x, (float)y));
		ps->SetRotation((float)rot);
		ps->SetActive();

		res->AddRef();
		a = tParticle->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
		b = tParticle->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
		rect = tParticle->IsRectangle();
		UpdateCollisionCirclrRadius();
		return true;
	}

	return false;
}

GameObjectPool::GameObjectPool(lua_State* pL)
	: L(pL)
{
	// ��ʼ��αͷ������
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

	// ����һ��ȫ�ֱ����ڴ�����ж���
	lua_pushlightuserdata(L, (void*)&LAPP);  // p(ʹ��APPʵ��ָ���������Է�ֹ�û�����)
	lua_createtable(L, LGOBJ_MAXCNT, 0);  // p t(�����㹻���table���ڴ�����е���Ϸ������lua�еĶ�Ӧ����)

	// ȡ��lstg.GetAttr��lstg.SetAttr����Ԫ��
	lua_newtable(L);  // ... t
	lua_getglobal(L, "lstg");  // ... t t
	lua_pushstring(L, "GetAttr");  // ... t t s
	lua_gettable(L, -2);  // ... t t f(GetAttr)
	lua_pushstring(L, "SetAttr");  // ... t t f(GetAttr) s
	lua_gettable(L, -3);  // ... t t f(GetAttr) f(SetAttr)
	LASSERT(lua_iscfunction(L, -1) && lua_iscfunction(L, -2));
	lua_setfield(L, -4, "__newindex");  // ... t t f(GetAttr)
	lua_setfield(L, -3, "__index");  // ... t t
	lua_pop(L, 1);  // ... t(��������Ԫ��)
	
	// ����Ԫ���� register[app][mt]
	lua_setfield(L, -2, METATABLE_OBJ);  // p t
	lua_settable(L, LUA_REGISTRYINDEX);

	m_pCurrentObject = NULL;
}

GameObjectPool::~GameObjectPool()
{
	ResetPool();
}

GameObject* GameObjectPool::freeObject(GameObject* p)LNOEXCEPT
{
	GameObject* pRet = p->pObjectNext;

	if (m_pCurrentObject == p){
		m_pCurrentObject = NULL;
	}

	// �Ӷ��������Ƴ�
	LIST_REMOVE(p, Object);

	// ����Ⱦ�����Ƴ�
	LIST_REMOVE(p, Render);

	// ����ײ�����Ƴ�
	LIST_REMOVE(p, Collision);

	// ɾ��lua�������Ԫ��
	GETOBJTABLE;  // ot
	lua_pushnil(L);  // ot nil
	lua_rawseti(L, -2, p->id + 1);  // ot
	lua_pop(L, 1);

	// �ͷ����õ���Դ
	p->ReleaseResource();

	// ���յ������
	m_ObjectPool.Free(p->id);

	return pRet;
}

void GameObjectPool::DoFrame()LNOEXCEPT
{
	//����������ͣ
	GETOBJTABLE;  // ot
	int superpause = LAPP.UpdateSuperPause();


	GameObject* p = m_pObjectListHeader.pObjectNext;
	lua_Number cache1, cache1m, cache2;//�ٶ����Ƽ���ʱ�õ����м����
	while (p && p != &m_pObjectListTail)
	{
		// ����id��ȡ�����lua��table���õ�class���õ�framefunc
		if (superpause<=0 || p->ignore_superpause){
			m_pCurrentObject = p;
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_FRAME);  // ot t(object) t(class) f(frame)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(frame) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) ִ��֡����
			lua_pop(L, 2);  // ot
			
			if (p->pause<=0){
				if (p->resolve_move){
					p->vx = p->x - p->lastx;
					p->vy = p->y - p->lasty;
				}
				else{
					// ���¶���״̬
					p->vx += p->ax;
					p->vy += p->ay;
#ifdef USER_SYSTEM_OPERATION
					p->vy -= p->ag;//��������������
					//�ٶ����ƣ�����lua��
					cache1 = sqrt(p->vx * p->vx + p->vy * p->vy);
					if (p->maxv == 0.) {
						p->vx = p->vy = 0.;
					}
					else if (p->maxv < cache1) { //��ֹmaxvΪ���ֵʱ��˳�����������
						cache2 = p->maxv / cache1;
						p->vx = cache2 * p->vx;
						p->vy = cache2 * p->vy;
					}
					//���x��y���򵥶�����
					if (abs(p->vx) > p->maxvx) {
						p->vx = p->maxvx * ((p->vx > 0) ? 1 : -1);
					}
					if (abs(p->vy) > p->maxvy) {
						p->vy = p->maxvy * ((p->vy > 0) ? 1 : -1);
					}
#endif
					//�������
					p->x += p->vx;
					p->y += p->vy;
				}
				p->rot += p->omiga;
				
				// ��������ϵͳ�����У�
				if (p->res && p->res->GetType() == ResourceType::Particle)
				{
					float gscale = LRES.GetGlobalImageScaleFactor();
					p->ps->SetRotation((float)p->rot);
					if (p->ps->IsActived())  // �����Դ���
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
	lua_pop(L, 1);
}

void GameObjectPool::DoRender()LNOEXCEPT
{
	GETOBJTABLE;  // ot

	GameObject* p = m_pRenderListHeader.pRenderNext;
	LASSERT(p != nullptr);
	lua_Integer world = LAPP.GetWorldFlag();
	while (p && p != &m_pRenderListTail)
	{
		if (!p->hide  && CheckWorld(p->world, world))  // ֻ��Ⱦ�ɼ�����
		{
			m_pCurrentObject = p;
			// ����id��ȡ�����lua��table���õ�class���õ�renderfunc
			lua_rawgeti(L, -1, p->id + 1);  // ot t(object)
			lua_rawgeti(L, -1, 1);  // ot t(object) t(class)
			lua_rawgeti(L, -1, LGOBJ_CC_RENDER);  // ot t(object) t(class) f(render)
			lua_pushvalue(L, -3);  // ot t(object) t(class) f(render) t(object)
			lua_call(L, 1, 0);  // ot t(object) t(class) ִ����Ⱦ����
			lua_pop(L, 2);  // ot
		}
		p = p->pRenderNext;
	}
	m_pCurrentObject = NULL;
	lua_pop(L, 1);
}

void GameObjectPool::BoundCheck()LNOEXCEPT
{
	GETOBJTABLE;  // ot
	lua_Integer world = LAPP.GetWorldFlag();
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (CheckWorld(p->world, world)){
			if ((p->x < m_BoundLeft || p->x > m_BoundRight || p->y < m_BoundBottom || p->y > m_BoundTop) && p->bound)
			{
				m_pCurrentObject = p;
				// Խ������ΪDEL״̬
				p->status = STATUS_DEL;

				// ����id��ȡ�����lua��table���õ�class���õ�delfunc
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
	lua_pop(L, 1);
}
/*
void GameObjectPool::BoundCheckWorld(lua_Number w)LNOEXCEPT
{
	GETOBJTABLE;  // ot

	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p && p != &m_pObjectListTail)
	{
		if (p->world && w){
			if ((p->x < m_BoundLeft || p->x > m_BoundRight || p->y < m_BoundBottom || p->y > m_BoundTop) && p->bound)
			{
				m_pCurrentObject = p;
				// Խ������ΪDEL״̬
				p->status = STATUS_DEL;

				// ����id��ȡ�����lua��table���õ�class���õ�delfunc
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
	lua_pop(L, 1);
}*/

void GameObjectPool::CollisionCheck(size_t groupA, size_t groupB)LNOEXCEPT
{
	if (groupA >= LGOBJ_MAXCNT || groupB >= LGOBJ_MAXCNT)
		luaL_error(L, "Invalid collision group.");

	GETOBJTABLE;  // ot

	GameObject* pA = m_pCollisionListHeader[groupA].pCollisionNext;
	GameObject* pATail = &m_pCollisionListTail[groupA];
	GameObject* pBHeader = m_pCollisionListHeader[groupB].pCollisionNext;
	GameObject* pBTail = &m_pCollisionListTail[groupB];
	while (pA && pA != pATail)
	{
		GameObject* pB = pBHeader;
		while (pB && pB != pBTail)
		{
			if (LAPP.CheckWorlds(pA->world, pB->world)){
				if (::CollisionCheck(pA, pB))
				{
					// ����id��ȡ�����lua��table���õ�class���õ�collifunc
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

	lua_pop(L, 1);
}

void GameObjectPool::UpdateXY()LNOEXCEPT
{
	int superpause = LAPP.GetSuperPauseTime();
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
}

void GameObjectPool::AfterFrame()LNOEXCEPT
{
	int superpause = LAPP.GetSuperPauseTime();
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
}

int GameObjectPool::New(lua_State* L)LNOEXCEPT
{
	// ������
	if (!lua_istable(L, 1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
	lua_getfield(L, 1, "is_class");  // t(class) ... b
	if (!lua_toboolean(L, -1))
		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
	lua_pop(L, 1);  // t(class) ...

	// ����һ������
	size_t id = 0;
	if (!m_ObjectPool.Alloc(id))
		return luaL_error(L, "can't alloc object, object pool may be full.");
	
	// ���ö���
	GameObject* p = m_ObjectPool.Data(id);
	LASSERT(p);
	p->Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = m_iUid++;

	if (m_pCurrentObject){
		p->world = m_pCurrentObject->world;
	}

	// ����������
	LIST_INSERT_BEFORE(&m_pObjectListTail, p, Object);  // Object����ֻ��uid�йأ��������ĩβ����
	LIST_INSERT_BEFORE(&m_pRenderListTail, p, Render);  // Render�����ڲ������Ҫ��������
	LIST_INSERT_BEFORE(&m_pCollisionListTail[p->group], p, Collision);  // Ϊ��֤�����ԣ���CollisionҲ������
	LIST_INSERT_SORT(p, Render, RenderListSortFunc);
	LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);

	GETOBJTABLE;  // t(class) ... ot
	lua_createtable(L, 2, 0);  // t(class) ... ot t(object)
	lua_pushvalue(L, 1);  // t(class) ... ot t(object) class
	lua_rawseti(L, -2, 1);  // t(class) ... ot t(object)  ����class
	lua_pushinteger(L, (lua_Integer)id);  // t(class) ... ot t(object) id
	lua_rawseti(L, -2, 2);  // t(class) ... ot t(object)  ����id
	lua_getfield(L, -2, METATABLE_OBJ);  // t(class) ... ot t(object) mt
	lua_setmetatable(L, -2);  // t(class) ... ot t(object)  ����Ԫ��
	lua_pushvalue(L, -1);  // t(class) ... ot t(object) t(object)
	lua_rawseti(L, -3, id + 1);  // t(class) ... ot t(object)  ���õ�ȫ�ֱ�
	lua_insert(L, 1);  // t(object) t(class) ... ot
	lua_pop(L, 1);  // t(object) t(class) ...
	lua_rawgeti(L, 2, LGOBJ_CC_INIT);  // t(object) t(class) ... f(init)
	lua_insert(L, 3);  // t(object) t(class) f(init) ...
	lua_pushvalue(L, 1);  // t(object) t(class) f(init) ... t(object)
	lua_insert(L, 4);  // t(object) t(class) f(init) t(object) ...
	lua_call(L, lua_gettop(L) - 3, 0);  // t(object) t(class)  ִ�й��캯��
	lua_pop(L, 1);  // t(object)

	p->lastx = p->x;
	p->lasty = p->y;
	return 1;
}

int GameObjectPool::Add(lua_State* L)LNOEXCEPT
{
	// ������
	if (!lua_istable(L, 1))
	return luaL_error(L, "invalid argument #1, luastg object required for 'Add'.");
//	lua_getfield(L, 1, "is_class");  // t(class) ... b
//	if (!lua_toboolean(L, -1))
//		return luaL_error(L, "invalid argument #1, luastg object class required for 'New'.");
//	lua_pop(L, 1);  // t(class) ...

	// ����һ������
	size_t id = 0;
	if (!m_ObjectPool.Alloc(id))
		return luaL_error(L, "can't alloc object, object pool may be full.");

	// ���ö���
	GameObject* p = m_ObjectPool.Data(id);
	LASSERT(p);
	p->Reset();
	p->status = STATUS_DEFAULT;
	p->id = id;
	p->uid = m_iUid++;

	// ����������
	LIST_INSERT_BEFORE(&m_pObjectListTail, p, Object);  // Object����ֻ��uid�йأ��������ĩβ����
	LIST_INSERT_BEFORE(&m_pRenderListTail, p, Render);  // Render�����ڲ������Ҫ��������
	LIST_INSERT_BEFORE(&m_pCollisionListTail[p->group], p, Collision);  // Ϊ��֤�����ԣ���CollisionҲ������
	LIST_INSERT_SORT(p, Render, RenderListSortFunc);
	LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);

	GETOBJTABLE;  // t(object) ... ot
	lua_pushvalue(L, 1);  // t(object) ... ot t(object)
	lua_pushinteger(L, (lua_Integer)id);  // t(object) ... ot t(object) id
	lua_rawseti(L, -2, 2);  // t(object) ... ot t(object)  ����id
	lua_getfield(L, -2, METATABLE_OBJ);  // t(object) ... ot t(object) mt
	lua_setmetatable(L, -2);  // t(object) ... ot t(object)  ����Ԫ��
	lua_pushvalue(L, -1);  // t(object) ... ot t(object) t(object)
	lua_rawseti(L, -3, id + 1);  // t(object) ... ot t(object)  ���õ�ȫ�ֱ�

	InitAttr(L); //����ʼ���Դ�����C++����

	lua_insert(L, 1);  // t(object) t(object) ... ot
	lua_pop(L, 1);  // t(object) t(object) ...

	lua_getfield(L, 1, "init"); // t(object) t(object) ... f(init)
	if (lua_isnil(L, -1)){//û�й��캯��
		lua_pop(L, 3);// t(object)
	}
	else{
		lua_insert(L, 3);  // t(object) t(object) f(init) ...
		lua_pushvalue(L, 1);  // t(object) t(object) f(init) ... t(object)
		lua_insert(L, 4);  // t(object) t(object) f(init) t(object) ...
		lua_call(L, lua_gettop(L) - 3, 0);  // t(object) t(object)  ִ�й��캯��
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

		// �������еĻص�����
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

		// �������еĻص�����
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

	// �ڶ�����м��
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
	GameObject* p = m_pObjectListHeader.pObjectNext;
	while (p != &m_pObjectListTail)
		p = freeObject(p);
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

int GameObjectPool::NextObject(int groupId, int id)LNOEXCEPT
{
	if (id < 0)
		return -1;

	GameObject* p = m_ObjectPool.Data(static_cast<size_t>(id));
	if (!p)
		return -1;

	// �������һ����Ч�ķ��飬��������������б���
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
}

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

int GameObjectPool::FirstObject(int groupId)LNOEXCEPT
{
	GameObject* p;

	// �������һ����Ч�ķ��飬��������������б���
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
}

int GameObjectPool::GetAttr(lua_State* L)LNOEXCEPT
{
	lua_rawgeti(L, 1, 2);  // t(object) s(key) ??? i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));
	lua_pop(L, 1);  // t(object) s(key)

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__index' meta operation.");
	
	// ��ѯ����
	const char* key = luaL_checkstring(L, 2);
	
	// ��x,y���ػ�����
	if (key[0] == 'x' && key[1] == '\0')
	{
		lua_pushnumber(L, p->x);
		return 1;
	}
	else if (key[0] == 'y' && key[1] == '\0')
	{
		lua_pushnumber(L, p->y);
		return 1;
	}
	
	// һ������
	switch (GameObjectPropertyHash(key))
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
	case GameObjectProperty::OMIGA:
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
	case GameObjectProperty::A:
		lua_pushnumber(L, p->a / LRES.GetGlobalImageScaleFactor());
		break;
	case GameObjectProperty::B:
		lua_pushnumber(L, p->b / LRES.GetGlobalImageScaleFactor());
		break;
	case GameObjectProperty::RECT:
		lua_pushboolean(L, p->rect);
		break;
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
	case GameObjectProperty::X:
	case GameObjectProperty::Y:
	default:
		lua_pushnil(L);
		break;
	}

	return 1;
}

int GameObjectPool::SetAttr(lua_State* L)LNOEXCEPT
{
	lua_rawgeti(L, 1, 2);  // t(object) s(key) any(v) i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));
	lua_pop(L, 1);  // t(object) s(key) any(v)

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for '__newindex' meta operation.");

	// ��ѯ����
	const char* key = luaL_checkstring(L, 2);

	// ��x,y���ػ�����
	if (key[0] == 'x' && key[1] == '\0')
	{
		p->x = luaL_checknumber(L, 3);
		return 0;
	}
	else if (key[0] == 'y' && key[1] == '\0')
	{
		p->y = luaL_checknumber(L, 3);
		return 0;
	}	

	// һ������
	switch (GameObjectPropertyHash(key))
	{
	case GameObjectProperty::DX:
		return luaL_error(L, "property 'dx' is readonly.");
	case GameObjectProperty::DY:
		return luaL_error(L, "property 'dy' is readonly.");
	case GameObjectProperty::ROT:
		p->rot = luaL_checknumber(L, 3) * LDEGREE2RAD;
		break;
	case GameObjectProperty::OMIGA:
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
		p->layer = luaL_checknumber(L, 3);
		LIST_INSERT_SORT(p, Render, RenderListSortFunc); // ˢ��p����Ⱦ�㼶
		LASSERT(m_pRenderListHeader.pRenderNext != nullptr);
		LASSERT(m_pRenderListTail.pRenderPrev != nullptr);
		break;
	case GameObjectProperty::GROUP:
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
					LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);  // ˢ��p����ײ����
					LASSERT(m_pCollisionListHeader[group].pCollisionNext != nullptr);
					LASSERT(m_pCollisionListTail[group].pCollisionPrev != nullptr);
				}
			}
		} while (false);
		break;
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
	case GameObjectProperty::A:
		p->a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
		p->UpdateCollisionCirclrRadius();
		break;
	case GameObjectProperty::B:
		p->b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
		p->UpdateCollisionCirclrRadius();
		break;
	case GameObjectProperty::RECT:
		p->rect = lua_toboolean(L, 3) == 0 ? false : true;
		p->UpdateCollisionCirclrRadius();
		break;
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
	lua_rawgeti(L, 1, 2);  // t(object) i(id)
	size_t id = static_cast<size_t>(lua_tonumber(L, -1));

	GameObject* p = m_ObjectPool.Data(id);
	if (!p)
		return luaL_error(L, "invalid lstg object for init function.");

	// ��ѯ����
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
			case GameObjectProperty::OMIGA:
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
				p->layer = luaL_checknumber(L, 3);
				LIST_INSERT_SORT(p, Render, RenderListSortFunc); // ˢ��p����Ⱦ�㼶
				LASSERT(m_pRenderListHeader.pRenderNext != nullptr);
				LASSERT(m_pRenderListTail.pRenderPrev != nullptr);
				break;
			case GameObjectProperty::GROUP:
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
							LIST_INSERT_SORT(p, Collision, ObjectListSortFunc);  // ˢ��p����ײ����
							LASSERT(m_pCollisionListHeader[group].pCollisionNext != nullptr);
							LASSERT(m_pCollisionListTail[group].pCollisionPrev != nullptr);
						}
					}
				} while (false);
				break;
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
			case GameObjectProperty::A:
				p->a = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
				p->UpdateCollisionCirclrRadius();
				break;
			case GameObjectProperty::B:
				p->b = luaL_checknumber(L, 3) * LRES.GetGlobalImageScaleFactor();
				p->UpdateCollisionCirclrRadius();
				break;
			case GameObjectProperty::RECT:
				p->rect = lua_toboolean(L, 3) == 0 ? false : true;
				p->UpdateCollisionCirclrRadius();
				break;
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
		LWARNING("ParticleStop: ��ͼֹͣһ�����������ӷ������Ķ�������ӷ������(uid=%d)", m_iUid);
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
		LWARNING("ParticleFire: ��ͼ����һ�����������ӷ������Ķ�������ӷ������(uid=%d)", m_iUid);
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
		LWARNING("ParticleGetEmission: ��ͼ��ȡһ�����������ӷ������Ķ�������ӷ����ܶ�(uid=%d)", m_iUid);
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
		LWARNING("ParticleSetEmission: ��ͼ����һ�����������ӷ������Ķ�������ӷ����ܶ�(uid=%d)", m_iUid);
		return 0;
	}
	p->ps->SetEmission((float)::max(0., luaL_checknumber(L, 2)));
	return 0;
}

void GameObjectPool::DrawGroupCollider(f2dGraphics2D* graph, f2dGeometryRenderer* grender, int groupId, fcyColor fillColor)
{
	GameObject* p = m_pCollisionListHeader[groupId].pCollisionNext;
	GameObject* pTail = &m_pCollisionListTail[groupId];
	lua_Integer world = LAPP.GetWorldFlag();
	while (p && p != pTail)
	{
		if (p->colli && CheckWorld(p->world, world))
		{
			if (p->rect || p->a!=p->b)
			{
				fcyVec2 tHalfSize((float)p->a, (float)p->b);

				// ��������ε�4������
				f2dGraphics2DVertex tFinalPos[4] =
				{
					{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
					{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
					{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
					{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
				};

				// float tSin = sin(Angle), tCos = cos(Angle);
				float tSin, tCos;
				SinCos((float)p->rot, tSin, tCos);

				// �任
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
				grender->FillCircle(graph, fcyVec2((float)p->x, (float)p->y), (float)p->col_r, fillColor, fillColor,
					p->col_r < 10 ? 3 : (p->col_r < 20 ? 6 : 8));
			}
		}

		p = p->pCollisionNext;
	}
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