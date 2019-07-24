#pragma once
#include "Global.h"
#include "ObjectPool.hpp"
#include "CirularQueue.hpp"
#include "ResourceMgr.h"
#include "XCollision.h"
#include "GameObject.hpp"

namespace LuaSTGPlus
{
	//计算两个对象之间的碰撞
	inline bool CollisionCheck(GameObject* p1, GameObject* p2)LNOEXCEPT {
		//忽略不碰撞对象
		if (!p1->colli || !p2->colli)
			return false;//返回点0

#ifdef USING_ADVANCE_COLLIDER
		float la, ra, ba, ta;
		float lb, rb, bb, tb;
		float x1, x2, y1, y2, a1, a2, b1, b2, rot1, rot2, cr1, cr2;

		int cc1 = 0, cc2;
		while (p1->colliders[cc1].type != GameObjectColliderType::None && cc1 < MAX_COLLIDERS_COUNT) {
			x1 = p1->colliders[cc1].absx;
			y1 = p1->colliders[cc1].absy;
			cr1 = p1->colliders[cc1].circum_r;
			a1 = p1->colliders[cc1].a;
			b1 = p1->colliders[cc1].b;
			rot1 = p1->colliders[cc1].absrot;
			
			cc2 = 0;//归位
			while (p2->colliders[cc2].type != GameObjectColliderType::None && cc2 < MAX_COLLIDERS_COUNT) {
				x2 = p2->colliders[cc2].absx;
				y2 = p2->colliders[cc2].absy;
				cr2 = p2->colliders[cc2].circum_r;

				//快速AABB检测
				la = x1 - cr1; ra = x1 + cr1; ba = y1 - cr1; ta = y1 + cr1;
				lb = x2 - cr2; rb = x2 + cr2; bb = y2 - cr2; tb = y2 + cr2;
				if ((la >= rb) || (ra <= lb) || (ba >= tb) || (ta <= bb)) {
					cc2++;
					continue;
				}

				a2 = p2->colliders[cc2].a;
				b2 = p2->colliders[cc2].b;
				rot2 = p2->colliders[cc2].absrot;

				//外接圆碰撞检测，没发生碰撞则直接PASS
				if (!xmath::collision::check(
					xmath::Vec2(x1, y1), cr1, cr1, rot1, XColliderType::Circle,
					xmath::Vec2(x2, y2), cr2, cr2, rot2, XColliderType::Circle)) {
					cc2++;
					continue;
				}

					//精确碰撞检测
				if (xmath::collision::check(
					xmath::Vec2(x1, y1), a1, b1, rot1, p1->colliders[cc1].xtype,
					xmath::Vec2(x2, y2), a2, b2, rot2, p2->colliders[cc2].xtype)) {
					return true;
				}//返回点1

				cc2++;
			}

			cc1++;
		}

		return false;//返回点2
#else
		//快速AABB检测
		if ((p1->x - p1->col_r >= p2->x + p2->col_r) ||
			(p1->x + p1->col_r <= p2->x - p2->col_r) ||
			(p1->y - p1->col_r >= p2->y + p2->col_r) ||
			(p1->y + p1->col_r <= p2->y - p2->col_r))
		{
			return false;
		}

		float x1 = (float)p1->x; float x2 = (float)p2->x; float y1 = (float)p1->y; float y2 = (float)p2->y;
		float a1 = (float)p1->a; float a2 = (float)p2->a; float b1 = (float)p1->b; float b2 = (float)p2->b;
		float rot1 = (float)p1->rot; float rot2 = (float)p2->rot;
		float cr1 = (float)p1->col_r; float cr2 = (float)p2->col_r;

		//外接圆碰撞检测，没发生碰撞则直接PASS
		if (!xmath::collision::check(xmath::Vec2(x1, y1), cr1, cr1, rot1, XColliderType::Circle,
			xmath::Vec2(x2, y2), cr2, cr2, rot2, XColliderType::Circle)) {
			return false;
		}

		//精确碰撞检测
		if (!p1->rect && !p2->rect) {
			//椭圆、椭圆碰撞检测
			return xmath::collision::check(xmath::Vec2(x1, y1), a1, b1, rot1, XColliderType::Ellipse,
				xmath::Vec2(x2, y2), a2, b2, rot2, XColliderType::Ellipse);
		}
		else if (p1->rect && p2->rect) {
			//矩形、矩形碰撞检测
			return xmath::collision::check(xmath::Vec2(x1, y1), a1, b1, rot1, XColliderType::OBB,
				xmath::Vec2(x2, y2), a2, b2, rot2, XColliderType::OBB);
		}
		else
		{
			//矩形、椭圆碰撞检测
			if (p1->rect && (!p2->rect))
			{
				return xmath::collision::check(xmath::Vec2(x1, y1), a1, b1, rot1, XColliderType::OBB,
					xmath::Vec2(x2, y2), a2, b2, rot2, XColliderType::Ellipse);
			}
			else if ((!p1->rect) && p2->rect)
			{
				return xmath::collision::check(xmath::Vec2(x1, y1), a1, b1, rot1, XColliderType::Ellipse,
					xmath::Vec2(x2, y2), a2, b2, rot2, XColliderType::OBB);
			}
		}
		return false;
#endif // USING_ADVANCE_COLLIDER
	}
	
	//曲线激光特化实现
	class GameObjectBentLaser
	{
	public:
		static GameObjectBentLaser* AllocInstance();
		static void FreeInstance(GameObjectBentLaser* p);
	private:
		struct LaserNode
		{
			fcyVec2 pos;		//节点位置
			float half_width;	//半宽
			float rot;			//节点朝向
			float dis;			//到上一个节点的距离
			float x_dir;		//节点朝向垂直方向的x分量
			float y_dir;		//节点朝向垂直方向的y分量
			bool active;		//节点活动状况
			bool sharp;			//相对上一个节点的朝向成钝角

			LaserNode(){
				half_width = 0.0f;
				x_dir = 0.0f;
				y_dir = 0.0f;
				active = true;
				rot = 0.0f;
				dis = 0.0f;
				sharp = false;
			}
		};
	private:
		CirularQueue<LaserNode, LGOBJ_MAXLASERNODE> m_Queue;
		float m_fLength = 0.f;  // 记录激光长度
	public:
		bool Update(size_t id, int length, float width,bool active)LNOEXCEPT;
		bool UpdateByNode(size_t id, int node, int length, float width, bool active)LNOEXCEPT;
		bool RecalRot()LNOEXCEPT;
		bool CompileNode(size_t i)LNOEXCEPT;
		bool UpdatePositionByList(lua_State *L, int length, float width, int index, bool revert)LNOEXCEPT;
		void Release()LNOEXCEPT;
		bool Render(const char* tex_name, BlendMode blend, fcyColor c, float tex_left, float tex_top, float tex_width, float tex_height, float scale)LNOEXCEPT;
		bool CollisionCheck(float x, float y, float rot, float a, float b, bool rect)LNOEXCEPT;
		void RenderCollider(fcyColor fillColor)LNOEXCEPT;
		bool CollisionCheckW(float x, float y, float rot, float a, float b, bool rect, float width)LNOEXCEPT;
		bool BoundCheck()LNOEXCEPT;
		int SampleL(lua_State *L, float length)LNOEXCEPT;
		int UpdateLength() LNOEXCEPT;
		int SampleT(lua_State *L, float delay) LNOEXCEPT;
		void SetAllWidth(float width)LNOEXCEPT;
	protected:
		GameObjectBentLaser();
		~GameObjectBentLaser();
	};

	//游戏对象池
	class GameObjectPool
	{
	private:
		lua_State* L = nullptr;
		uint64_t m_iUid = 0;
		FixedObjectPool<GameObject, LGOBJ_MAXCNT> m_ObjectPool;
		GameObject* m_pCurrentObject = nullptr;

		// Comparer
		struct _less_object {
			bool operator()(const GameObject* x, const GameObject* y) const {
				return x->uid < y->uid;
			}
		};
		struct _less_render {
			bool operator()(const GameObject* x, const GameObject* y) const {
				if (x->layer != y->layer) {
					return x->layer < y->layer;
				}
				else {
					return x->uid < y->uid;
				}
			}
		};
		// GameObject List
		std::set<GameObject*, _less_object> m_UpdateList;
		std::set<GameObject*, _less_render> m_RenderList;
		std::array<std::set<GameObject*, _less_object>, LGOBJ_GROUPCNT> m_ColliList;

		// 场景边界
		lua_Number m_BoundLeft = -100.f;
		lua_Number m_BoundRight = 100.f;
		lua_Number m_BoundTop = 100.f;
		lua_Number m_BoundBottom = -100.f;
	private:
		//准备lua表用于存放对象
		void _PrepareLuaObjectTable();
		
		// 申请一个对象，重置对象并将对象插入到各个链表，不处理lua部分，返回申请的对象
		GameObject* _AllocObject();

		// 释放一个对象，将对象从各个链表中移除，并回收，不处理lua部分和对象资源，返回下一个可用的对象
		GameObject* _ReleaseObject(GameObject* object);

		// 更改指定对象的图层，该操作会刷新对象在渲染链表中的位置
		void _SetObjectLayer(GameObject* object, lua_Number layer);

		// 更改指定对象的碰撞组，该操作会移动对象在碰撞组中的位置
		void _SetObjectColliGroup(GameObject* object, lua_Integer group);

		// 检查指定对象的坐标是否在场景边界内
		inline bool _ObjectBoundCheck(GameObject* object) {
			if (!object->bound)
				return true;
			if (object->x < m_BoundLeft ||
				object->x > m_BoundRight ||
				object->y < m_BoundBottom ||
				object->y > m_BoundTop)
				return false;
			return true;
		}

		// 释放一个对象，完全释放
		GameObject* freeObject(GameObject* p)LNOEXCEPT;
	public:
		int PushCurrentObject(lua_State* L)LNOEXCEPT;
		
		/// @brief 检查是否为主线程
		bool CheckIsMainThread(lua_State* pL)LNOEXCEPT { return pL == L; }

		/// @brief 获取已分配对象数量
		size_t GetObjectCount()LNOEXCEPT { return m_ObjectPool.Size(); }
		
		/// @brief 获取对象
		GameObject* GetPooledObject(size_t i)LNOEXCEPT { return m_ObjectPool.Data(i); }

		/// @brief 执行对象的Frame函数
		void DoFrame()LNOEXCEPT;

		/// @brief 执行对象的Render函数
		void DoRender()LNOEXCEPT;

		/// @brief 获取舞台边界
		fcyRect GetBound()LNOEXCEPT
		{
			return fcyRect((float)m_BoundLeft, (float)m_BoundTop, (float)m_BoundRight, (float)m_BoundBottom);
		}

		/// @brief 设置舞台边界
		void SetBound(lua_Number l, lua_Number r, lua_Number b, lua_Number t)LNOEXCEPT
		{
			m_BoundLeft = l;
			m_BoundRight = r;
			m_BoundTop = t;
			m_BoundBottom = b;
		}

		/// @brief 执行边界检查
		void BoundCheck()LNOEXCEPT;

		/// @brief 碰撞检查
		/// @param[in] groupA 对象组A
		/// @param[in] groupB 对象组B
		void CollisionCheck(size_t groupA, size_t groupB)LNOEXCEPT;

		/// @brief 更新对象的XY坐标偏移量
		void UpdateXY()LNOEXCEPT;

		/// @brief 帧末更新函数
		void AfterFrame()LNOEXCEPT;

		/// @brief 创建新对象
		int New(lua_State* L)LNOEXCEPT;

		/// @brief 将一个对象插入链表
		int Add(lua_State* L)LNOEXCEPT;

		/// @brief 通知对象删除
		int Del(lua_State* L)LNOEXCEPT;
		
		/// @brief 通知对象消亡
		int Kill(lua_State* L)LNOEXCEPT;

		/// @brief 检查对象是否有效
		int IsValid(lua_State* L)LNOEXCEPT;
		
		//重置对象的各项属性，并释放资源，保留uid和id
		bool DirtResetObject(size_t id)LNOEXCEPT;

		/// @brief 求夹角
		bool Angle(size_t idA, size_t idB, double& out)LNOEXCEPT;

		/// @brief 求距离
		bool Dist(size_t idA, size_t idB, double& out)LNOEXCEPT;

		/// @brief 求两个对象是否发生碰撞
		bool ColliCheck(size_t idA, size_t idB, bool ignoreWorldMask, bool& out)LNOEXCEPT;

		/// @brief 计算速度方向和大小
		bool GetV(size_t id, double& v, double& a)LNOEXCEPT;

		/// @brief 设置速度方向和大小
		bool SetV(size_t id, double v, double a, bool updateRot)LNOEXCEPT;

		/// @brief 设置元素的图像状态
		bool SetImgState(size_t id, BlendMode m, fcyColor c)LNOEXCEPT;

		/// @brief 特化设置HGE粒子的渲染状态
		bool SetParState(size_t id, BlendMode m, fcyColor c)LNOEXCEPT;

		/// @brief 范围检查
		bool BoxCheck(size_t id, double left, double right, double top, double bottom, bool& ret)LNOEXCEPT;
		
		/// @brief 清空对象池
		void ResetPool()LNOEXCEPT;

		/// @brief 执行默认渲染
		bool DoDefaultRender(GameObject* p)LNOEXCEPT;

		/// @brief 执行默认渲染
		bool DoDefaultRender(size_t id)LNOEXCEPT {
			return DoDefaultRender(m_ObjectPool.Data(id));
		}

		/// @brief 获取下一个元素的ID
		/// @return 返回-1表示无元素
		int NextObject(int groupId, int id)LNOEXCEPT;

		//返回一个碰撞组迭代器
		int NextObject(lua_State* L)LNOEXCEPT;

		/// @brief 获取列表中的第一个元素ID
		/// @note 为迭代器使用
		/// @return 返回-1表示无元素
		int FirstObject(int groupId)LNOEXCEPT;

		/// @brief 属性读方法
		int GetAttr(lua_State* L)LNOEXCEPT;

		/// @brief 属性写方法
		int SetAttr(lua_State* L)LNOEXCEPT;

		/// @brief 初始化方法
		int InitAttr(lua_State* L)LNOEXCEPT;

		/// @brief 调试目的，获取对象列表
		int GetObjectTable(lua_State* L)LNOEXCEPT;

		/// @brief 对象粒子池相关操作
		int ParticleStop(lua_State* L)LNOEXCEPT;
		int ParticleFire(lua_State* L)LNOEXCEPT;
		int ParticleGetn(lua_State* L)LNOEXCEPT;
		int ParticleGetEmission(lua_State* L)LNOEXCEPT;
		int ParticleSetEmission(lua_State* L)LNOEXCEPT;

		/// @brief 对象资源相关操作
	private:
		///用于多world
		
		lua_Integer m_iWorld = 15;//当前的world mask
		std::array<lua_Integer, 4> m_Worlds = { 15, 0, 0, 0 };//预置的world mask
		//lua_Integer m_Worlds[4] = { 15, 0, 0, 0 };//预置的world mask
	public:
		///用于多world

		//设置当前的world mask
		inline void SetWorldFlag(lua_Integer world)LNOEXCEPT {
			m_iWorld = world;
		}
		//获取当前的world mask
		inline lua_Integer GetWorldFlag()LNOEXCEPT {
			return m_iWorld;
		}
		//设置预置的world mask
		inline void ActiveWorlds(lua_Integer a, lua_Integer b, lua_Integer c, lua_Integer d)LNOEXCEPT {
			m_Worlds[0] = a;
			m_Worlds[1] = b;
			m_Worlds[2] = c;
			m_Worlds[3] = d;
		}
		//检查两个world mask位与或的结果 //静态函数，不应该只用于类内
		static inline bool CheckWorld(lua_Integer gameworld, lua_Integer objworld) {
			return (gameworld == objworld) || (gameworld & objworld);
		}
		//对两个world mask，分别与预置的world mask位与或，用于检查是否在同一个world内
		bool CheckWorlds(int a, int b)LNOEXCEPT {
			if (CheckWorld(a, m_Worlds[0]) && CheckWorld(b, m_Worlds[0]))return true;
			if (CheckWorld(a, m_Worlds[1]) && CheckWorld(b, m_Worlds[1]))return true;
			if (CheckWorld(a, m_Worlds[2]) && CheckWorld(b, m_Worlds[2]))return true;
			if (CheckWorld(a, m_Worlds[3]) && CheckWorld(b, m_Worlds[3]))return true;
			return false;
		}
	private:
		///用于超级暂停

		lua_Integer m_superpause = 0;
		lua_Integer m_nextsuperpause = 0;
	public:
		///用于超级暂停

		//获取可信的超级暂停时间
		inline lua_Integer GetSuperPauseTime()LNOEXCEPT {
			return m_superpause;
		}
		//获取超级暂停剩余时间
		inline lua_Integer GetNextFrameSuperPauseTime()LNOEXCEPT {
			return m_nextsuperpause;
		}
		//设置超级暂停剩余时间
		inline void SetNextFrameSuperPauseTime(lua_Integer time)LNOEXCEPT {
			m_nextsuperpause = time;
		}
		//更新超级暂停的剩余时间并返回当前的可信值
		inline lua_Integer UpdateSuperPause() {
			m_superpause = m_nextsuperpause;
			if (m_nextsuperpause > 0)
				m_nextsuperpause = m_nextsuperpause - 1;
			return m_superpause;
		}
	public:  // 内部使用
		void DrawGroupCollider(f2dGraphics2D* graph, f2dGeometryRenderer* grender, int groupId, fcyColor fillColor);
		void DrawGroupCollider2(int groupId, fcyColor fillColor);
	private:
		GameObjectPool& operator=(const GameObjectPool&);
		GameObjectPool(const GameObjectPool&);
	public:
		GameObjectPool(lua_State* pL);
		~GameObjectPool();
	};
}
