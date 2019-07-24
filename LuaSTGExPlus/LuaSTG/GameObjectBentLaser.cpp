#include "Global.h"
#include "AppFrame.h"
#include "GameObjectPool.h"
#include "CollisionDetect.h"

using namespace std;
using namespace LuaSTGPlus;

//======================================

static fcyMemPool<sizeof(GameObjectBentLaser)> s_GameObjectBentLaserPool(1024);

GameObjectBentLaser* GameObjectBentLaser::AllocInstance()
{
	// ! 潜在bad_alloc
	GameObjectBentLaser* pRet = new(s_GameObjectBentLaserPool.Alloc()) GameObjectBentLaser();
	return pRet;
}

void GameObjectBentLaser::FreeInstance(GameObjectBentLaser* p)
{
	p->~GameObjectBentLaser();
	s_GameObjectBentLaserPool.Free(p);
}

//======================================

GameObjectBentLaser::GameObjectBentLaser()
{
}

GameObjectBentLaser::~GameObjectBentLaser()
{
}

bool GameObjectBentLaser::Update(size_t id, int length, float width, bool active)LNOEXCEPT
{
	GameObject* p = LPOOL.GetPooledObject(id);
	if (!p)
		return false;
	if (length <= 1)
	{
		LERROR("lstgBentLaserData: 无效的参数length");
		return false;
	}

	// ！循环队列的头部是最早创建的，尾部才是最新放入的！

	// 移除多余的节点，保证长度在length范围内
	while (m_Queue.IsFull() || m_Queue.Size() >= (size_t)length)
	{
		LaserNode tLastPop;
		m_Queue.Pop(tLastPop);

		// 减少总长度
		if (!m_Queue.IsEmpty())
		{
			LaserNode& tFront = m_Queue.Front();
			// 如果最后两个节点都是激活的，根据节点间的距离减少曲线激光总长度
			if (tLastPop.active && tFront.active) {
				m_fLength -= tFront.dis;
			}
			tFront.dis = 0;
		}
	}

	// 添加新节点
	if (m_Queue.Size() < (size_t)length)
	{
		LaserNode tNode;
		tNode.pos.Set((float)p->x, (float)p->y);
		tNode.half_width = width / 2.f;
		tNode.active = active;
		m_Queue.Push(tNode);

		// 增加总长度
		if (m_Queue.Size() > 1)
		{
			LaserNode& tNodeLast = m_Queue.Back();
			LaserNode& tNodeBeforeLast = m_Queue[m_Queue.Size() - 2];
			fcyVec2 dpos = tNodeLast.pos - tNodeBeforeLast.pos;
			float l = dpos.Length();
			tNodeLast.dis = l;
			if (l == 0) {
				tNodeLast.rot = tNodeBeforeLast.rot;
			}
			else {
				tNodeLast.rot = dpos.CalcuAngle();
				if (tNodeLast.active && active) {
					m_fLength += l;
				}
			}
			CompileNode(m_Queue.Size() - 1);
			CompileNode(m_Queue.Size() - 2);
		}
		else {
			LaserNode& tNodeLast = m_Queue.Back();
			tNodeLast.rot = atan2(p->vy, p->vx);
			tNodeLast.dis = 0;
		}
	}

	return true;
}

bool GameObjectBentLaser::UpdateByNode(size_t id, int node, int length, float width, bool active)LNOEXCEPT
{
	GameObject* p = LPOOL.GetPooledObject(id);
	if (!p)
		return false;
	if (length <= 1)
	{
		LERROR("lstg.BentLaserData: 无效的参数length");
		return false;
	}

	if (node < 0) {
		node = m_Queue.Size() + node;
	}

	// 添加新节点
	if (node < m_Queue.Size() && node >= 0)
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
		if (l == 0) {
			next.rot = cur.rot;
		}
		else {
			next.rot = dpos.CalcuAngle();
		}
		if (i == 0) {
			cur.rot = next.rot;
		}
	}
	for (size_t i = 0; i < sz; ++i) {
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

	if (i == 0) {
		cur.x_dir = b;
		cur.y_dir = -a;
		return true;
	}

	if (i < p) {
		LaserNode& next = m_Queue[i + 1];
		c = cos(next.rot);
		d = sin(next.rot);
	}
	else {
		cur.x_dir = b;
		cur.y_dir = -a;
		return true;
	}

	float d1 = a * d - c * b;
	if (d1 < 0.3 && d1 > -0.3)
	{
		float d2 = a * c + b * d;
		if (d2 > 0) {
			//小于90度
			if (d1<0.01 && d1>-0.01)
			{
				cur.x_dir = (b + d) / 2;
				cur.y_dir = (-a - c) / 2;
				return true;
			}
		}
		else {
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
	// 忽略只有一个节点的情况
	if (m_Queue.Size() <= 1)
		return true;

	fcyRefPointer<ResTexture> pTex = LRES.FindTexture(tex_name);
	if (!pTex)
	{
		LERROR("lstgBentLaserData: 找不到纹理资源'%m'", tex_name);
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

		if (!cur.active || !next.active) {
			continue;
		}

		if (cur.sharp) {
			flip = !flip;
		}


		float expX1 = cur.x_dir * scale * cur.half_width;
		float expY1 = cur.y_dir * scale * cur.half_width;

		if (flip) {
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

		if (flip) {
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

			// 计算宽度上的扩展长度(旋转270度)

			/*
		//	fcyVec2 expandVec = offsetNA;//offsetA.GetNormalize();
		//	std::swap(expandVec.x, expandVec.y);
		//	expandVec.y = -expandVec.y;

			if (i == 0)  // 如果是第一个节点，则其宽度扩展使用expandVec计算
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
			else  // 否则，拷贝1和2
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
				else // 计算角平分线到角两边距离为next.half_width * scale的偏移量
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
	// 忽略只有一个节点的情况
	if (m_Queue.Size() <= 1)
		return false;

#ifdef USING_ADVANCE_COLLIDER
	GameObject testObjA;
	testObjA.Reset();
	testObjA.colliders[0].type = GameObjectColliderType::Ellipse;

	GameObject testObjB;
	testObjB.Reset();
	testObjB.x = x;
	testObjB.y = y;
	testObjB.rot = rot;
	testObjB.colliders[0].a = a;
	testObjB.colliders[0].b = b;
	if (rect) {
		testObjB.colliders[0].type = GameObjectColliderType::OBB;
	}
	else {
		testObjB.colliders[0].type = GameObjectColliderType::Ellipse;
	}
	testObjB.colliders[0].calcircum();
	testObjB.colliders[0].caloffset(x, y, rot);

	int sn = m_Queue.Size();
	for (size_t i = 0; i < sn; ++i)
	{
		LaserNode& n = m_Queue[i];
		if (!n.active)continue;
		if (i > 0) {
			LaserNode& last = m_Queue[i - 1];
			if (!last.active) {
				float df = n.dis;
				if (df > n.half_width) {
					fcyVec2 c = (last.pos + n.pos) * 0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.colliders[0].type = GameObjectColliderType::OBB;
					testObjA.rot = n.rot;
					testObjA.colliders[0].a = df / 2;
					testObjA.colliders[0].b = n.half_width;
					testObjA.colliders[0].calcircum();
					testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
					if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB)) { return true; }
				}
			}
		}
		
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.colliders[0].a = testObjA.colliders[0].b = n.half_width;
		testObjA.colliders[0].type = GameObjectColliderType::Ellipse;
		testObjA.colliders[0].calcircum();
		testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
		if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB)) { return true; }
	}
	
	return false;
#else
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
		if (i > 0) {
			LaserNode& last = m_Queue[i - 1];
			if (!last.active) {
				float df = n.dis;
				if (df > n.half_width) {
					fcyVec2 c = (last.pos + n.pos) * 0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.rect = true;
					testObjA.rot = n.rot;
					testObjA.a = df / 2;
					testObjA.b = n.half_width;
					testObjA.UpdateCollisionCirclrRadius();
					if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB))
						return true;

				}
			}
		}
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.a = testObjA.b = n.half_width;
		testObjA.rect = false;
		testObjA.UpdateCollisionCirclrRadius();
		if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB))
			return true;
	}
	return false;
#endif // USING_ADVANCE_COLLIDER
}

void GameObjectBentLaser::RenderCollider(fcyColor fillColor)LNOEXCEPT {
	// 忽略只有一个节点的情况
	int sn = m_Queue.Size();
	if (sn <= 1)
		return;

#ifdef USING_ADVANCE_COLLIDER
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

	GameObject testObjA;
	testObjA.Reset();
	testObjA.colliders[0].type = GameObjectColliderType::Ellipse;

	for (size_t i = 0; i < sn; ++i)
	{
		LaserNode& n = m_Queue[i];
		if (!n.active)
			continue;
		if (i > 0) {
			LaserNode& last = m_Queue[i - 1];
			if (!last.active) {
				float df = n.dis;
				if (df > n.half_width) {
					//计算部分
					fcyVec2 c = (last.pos + n.pos) * 0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.colliders[0].type = GameObjectColliderType::OBB;
					testObjA.rot = n.rot;
					testObjA.colliders[0].a = df / 2;
					testObjA.colliders[0].b = n.half_width;
					testObjA.colliders[0].calcircum();
					testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
					//渲染部分
					{
						fcyVec2 tHalfSize(testObjA.colliders[0].a, testObjA.colliders[0].b);
						// 计算出矩形的4个顶点
						f2dGraphics2DVertex tFinalPos[4] =
						{
							{ -tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 0.0f },
							{ tHalfSize.x, -tHalfSize.y, 0.5f, fillColor.argb, 0.0f, 1.0f },
							{ tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 1.0f },
							{ -tHalfSize.x, tHalfSize.y, 0.5f, fillColor.argb, 1.0f, 0.0f }
						};
						float tSin, tCos;
						LuaSTGPlus::SinCos(testObjA.colliders[0].absrot, tSin, tCos);
						// 变换
						for (int i = 0; i < 4; i++)
						{
							fFloat tx = tFinalPos[i].x * tCos - tFinalPos[i].y * tSin,
								ty = tFinalPos[i].x * tSin + tFinalPos[i].y * tCos;
							tFinalPos[i].x = tx + testObjA.colliders[0].absx;
							tFinalPos[i].y = ty + testObjA.colliders[0].absy;
						}
						graph->DrawQuad(nullptr, tFinalPos);
					}
				}
			}
		}
		
		//计算部分
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.colliders[0].a = testObjA.colliders[0].b = n.half_width;
		testObjA.colliders[0].type = GameObjectColliderType::Ellipse;
		testObjA.colliders[0].calcircum();
		testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
		//渲染部分
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
					vert[i].x = testObjA.colliders[0].a * std::cosf(angle);
					vert[i].y = testObjA.colliders[0].b * std::sinf(angle);
					vert[i].z = 0.5f;//2D下固定z0.5
					vert[i].color = fillColor.argb;
					vert[i].u = 0.0f; vert[i].v = 0.0f;//没有使用到贴图，uv是多少无所谓
				}
			}
			// 变换
			{
				float tSin, tCos;
				LuaSTGPlus::SinCos(testObjA.colliders[0].absrot, tSin, tCos);
				for (int i = 0; i < vertcount; i++)
				{
					fFloat tx = vert[i].x * tCos - vert[i].y * tSin,
						   ty = vert[i].x * tSin + vert[i].y * tCos;
					vert[i].x = tx + testObjA.colliders[0].absx;
					vert[i].y = ty + testObjA.colliders[0].absy;
				}
			}
			//绘制
			graph->DrawRaw(nullptr, vertcount, indexcount, vert, index, false);
		}
	}

	graph->SetBlendState(stState);
	graph->SetColorBlendType(txState);
#endif // USING_ADVANCE_COLLIDER
}

bool GameObjectBentLaser::CollisionCheckW(float x, float y, float rot, float a, float b, bool rect, float width)LNOEXCEPT
{
	// 忽略只有一个节点的情况
	if (m_Queue.Size() <= 1)
		return false;

#ifdef USING_ADVANCE_COLLIDER
	width = width / 2;
	GameObject testObjA;
	testObjA.Reset();
	testObjA.colliders[0].type = GameObjectColliderType::Ellipse;

	GameObject testObjB;
	testObjB.Reset();
	testObjB.x = x;
	testObjB.y = y;
	testObjB.rot = rot;
	testObjB.colliders[0].a = a;
	testObjB.colliders[0].b = b;
	if (rect) {
		testObjB.colliders[0].type = GameObjectColliderType::OBB;
	}
	else {
		testObjB.colliders[0].type = GameObjectColliderType::Ellipse;
	}
	testObjB.colliders[0].calcircum();
	testObjB.colliders[0].caloffset(x, y, rot);

	int sn = m_Queue.Size();
	for (size_t i = 0; i < sn; ++i)
	{
		LaserNode& n = m_Queue[i];
		if (!n.active)continue;
		if (i > 0) {
			LaserNode& last = m_Queue[i - 1];
			if (!last.active) {
				float df = n.dis;
				if (df > width) {
					fcyVec2 c = (last.pos + n.pos) * 0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.colliders[0].type = GameObjectColliderType::OBB;
					testObjA.rot = n.rot;
					testObjA.colliders[0].a = df / 2;
					testObjA.colliders[0].b = width;
					testObjA.colliders[0].calcircum();
					testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
					if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB)) { return true; }
				}
			}
		}
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.colliders[0].a = testObjA.colliders[0].b = n.half_width;
		testObjA.colliders[0].type = GameObjectColliderType::Ellipse;
		testObjA.colliders[0].calcircum();
		testObjA.colliders[0].caloffset(testObjA.x, testObjA.y, testObjA.rot);
		if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB)) { return true; }
	}
	
	return false;
#else
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
		if (i > 0) {
			LaserNode& last = m_Queue[i - 1];
			if (!last.active) {
				float df = n.dis;
				if (df > width) {
					fcyVec2 c = (last.pos + n.pos) * 0.5;
					testObjA.x = c.x;
					testObjA.y = c.y;
					testObjA.rect = true;
					testObjA.rot = n.rot;
					testObjA.a = df / 2;
					testObjA.b = width;
					testObjA.UpdateCollisionCirclrRadius();
					if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB))
						return true;

				}
			}
		}
		testObjA.x = n.pos.x;
		testObjA.y = n.pos.y;
		testObjA.a = testObjA.b = width;
		testObjA.rect = false;
		testObjA.UpdateCollisionCirclrRadius();
		if (LuaSTGPlus::CollisionCheck(&testObjA, &testObjB))
			return true;
	}
	return false;
#endif // USING_ADVANCE_COLLIDER
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
	// 越界时返回false，只有当所有的弹幕越界才返回false
	return false;
}

bool LuaSTGPlus::GameObjectBentLaser::UpdatePositionByList(lua_State * L, int length, float width, int index, bool revert) LNOEXCEPT// ... t(list) //lua index从1开始
{
	// ... t(list)
	int push_count = 0;//以插入头的节点数量 

	for (int i = 0; i < length; i++)
	{
		//获得x,y
		lua_rawgeti(L, -1, i + 1);// ... t(list) t(object)
		lua_pushstring(L, "x");// ... t(list) t(object) 'x'
		lua_gettable(L, -2);// ... t(list) t(object) x
		float x = luaL_optnumber(L, -1, 0.0);
		lua_pop(L, 1);
		lua_pushstring(L, "y");// ... t(list) t(object) 'y'
		lua_gettable(L, -2);// ... t(list) t(object) y
		float y = luaL_optnumber(L, -1, 0.0);// ... t(list) t(object) y
		lua_pop(L, 2);// ... t(list)

		//得到index
		//顶点处在队列前边
		int cindex = push_count + index - 1 + (revert ? -i : i);
		if (cindex < 0) {
			int j = cindex;
			LaserNode np;
			np.active = false;
			while (j > 0) {
				m_Queue.Push(np);
				j--;
				push_count++;
			}
		}

		int size = m_Queue.Size();
		//顶点处在队列后边
		if (cindex >= size) {
			int j = cindex - size + 1;
			LaserNode np;
			np.active = false;
			while (j > 0) {
				m_Queue.PushBack(np);
				j--;
			}
		}
		size = m_Queue.Size();
		//设置顶点
		LaserNode* tNode = &m_Queue[size - cindex - 1];
		tNode->active = true;
		tNode->half_width = width / 2;
		tNode->pos.Set(x, y);
	}
	UpdateLength();
	RecalRot();
	return true;
}

int LuaSTGPlus::GameObjectBentLaser::SampleL(lua_State * L, float length) LNOEXCEPT
{
	//插入一个数组
	lua_newtable(L); //... t(list)
	// 忽略没有节点的情况
	if (m_Queue.Size() <= 1)
		return true;

	float fLeft = 0;// 剩余长度
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
		float angle = expandVec.CalcuAngle() * LRAD2DEGREE + 180;
		while (fLeft - lenOffsetA <= 0) {
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
	if (m_Queue.Size() > 0) {
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
		if (cur.active && next.active) {
			m_fLength += lenOffsetA;
		}
	}
	return 1;
}

int LuaSTGPlus::GameObjectBentLaser::SampleT(lua_State * L, float delay) LNOEXCEPT
{
	//插入一个数组
	lua_newtable(L); //... t(list)
	// 忽略没有节点的情况
	if (m_Queue.Size() <= 1)
		return true;

	float fLeft = 0;// 剩余长度
	int count = 0;

	float tVecLength = 0;
	for (size_t i = m_Queue.Size() - 1; i > 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		LaserNode& next = m_Queue[i - 1];


		fcyVec2 vn = cur.pos;
		fcyVec2 offsetA = next.pos - cur.pos;
		float lenOffsetA = offsetA.Length();
		float angle = offsetA.CalcuAngle() * LRAD2DEGREE + 180;
		while (fLeft - 1 <= 0) {
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
	for (int i = (unsigned int)m_Queue.Size() - 1; i >= 0; --i)
	{
		LaserNode& cur = m_Queue[i];
		cur.half_width = width / 2.f;
	}
}
