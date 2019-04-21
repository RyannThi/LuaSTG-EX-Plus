﻿#pragma once
#include "Global.h"
#include "ObjectPool.hpp"
#include "CirularQueue.hpp"
#include "ResourceMgr.h"

namespace LuaSTGPlus
{
	/// @brief 游戏对象状态
	enum GAMEOBJECTSTATUS
	{
		STATUS_FREE = 0,  // 空闲状态、用于标识链表伪头部
		STATUS_DEFAULT,  // 正常状态
		STATUS_KILL,  // 被kill事件触发
		STATUS_DEL  // 被del事件触发
	};
	
	/// @brief 游戏对象
	struct GameObject
	{
		GAMEOBJECTSTATUS status;  // (不可见)对象状态
		size_t id;  // (不可见)对象在对象池中的id
		int64_t uid;  // (不可见)对象唯一id

		lua_Number x, y;  // 中心坐标
		lua_Number lastx, lasty;  // (不可见)上一帧中心坐标
		lua_Number dx, dy;  // (只读)上一帧中心坐标相对中心坐标的偏移量
		lua_Number rot, omiga;  // 旋转角度与角度增量
		lua_Number vx, vy;  // 速度
#ifdef USER_SYSTEM_OPERATION
		lua_Number maxv, maxvx, maxvy; // 速度限制
		lua_Number ag;  // 重力加速度
#endif
		lua_Number ax, ay;  // 加速度
		//lua_Number va, speed; // 速度方向 速度值
		lua_Number layer;  // 图层
		lua_Number a, b;  // 单位的横向、纵向碰撞大小的一半
		lua_Number hscale, vscale;  // 横向、纵向拉伸率，仅影响渲染

		bool colli;  // 是否参与碰撞
		bool rect;  // 是否为矩形碰撞盒
		bool bound;  // 是否越界清除
		bool hide;  // 是否隐藏
		bool navi;  // 是否自动转向

		//EX+
		bool resolve_move; //是否为计算速度而非计算位置
		lua_Integer pause; //对象被暂停的时间(帧) 对象被暂停时，将跳过速度计算，但是timer会增加，frame仍会调用
		bool ignore_superpause; //是否无视超级暂停。 超级暂停时，timer不会增加，frame不会调用，但render会调用。
		lua_Integer world; //世界标记位

		// 受colli,a,b,rect参数影响的碰撞盒外圆半径
		lua_Number col_r;

		lua_Integer group;  // 对象所在的碰撞组
		lua_Integer timer, ani_timer;  // 计数器

		Resource* res;  // 渲染资源
		ResParticle::ParticlePool* ps;  // 粒子系统

		// 链表域
		GameObject *pObjectPrev, *pObjectNext;
		GameObject *pRenderPrev, *pRenderNext;
		GameObject *pCollisionPrev, *pCollisionNext;
		
		void Reset()
		{
			status = STATUS_FREE;
			id = (size_t)-1;
			uid = 0;

			x = y = 0.;
			lastx = lasty = 0.;
			dx = dy = 0.;
			rot = omiga = 0.;
			vx = vy = 0.;
			ax = ay = 0.;
			layer = 0.;
			a = b = 0.;
			hscale = vscale = 1.;
#ifdef USER_SYSTEM_OPERATION
			maxv = maxvx = maxvy = DBL_HALF_MAX; // 平时应该不会有人弄那么大的速度吧，希望计算时不会溢出（
			ag = 0.;
#endif
			
			colli = bound = true;
			rect = hide = navi = false;

			col_r = 0.;

			group = LGOBJ_DEFAULTGROUP;
			timer = ani_timer = 0;

			res = nullptr;
			ps = nullptr;

			pObjectPrev = pObjectNext = nullptr;
			pRenderPrev = pRenderNext = nullptr;
			pCollisionPrev = pCollisionNext = nullptr;

			resolve_move = false;
			pause = 0;
			ignore_superpause = false;

			world = 1;
		}

		void UpdateCollisionCirclrRadius()
		{
			if (rect || (a!=b)) {
				//矩形或者椭圆
				col_r = ::sqrt(a * a + b * b);
			}
			else {
				//严格的正圆
				col_r = (a + b) / 2;
			}
		}

		void ReleaseResource()
		{
			if (res)
			{
				if (res->GetType() == ResourceType::Particle)
				{
					LASSERT(ps);
					static_cast<ResParticle*>(res)->FreeInstance(ps);
					ps = nullptr;
				}
				res->Release();
				res = nullptr;
			}
		}

		bool ChangeResource(const char* res_name);

		template <typename T>
		//bool ChangeResourceEx(T res_set);
		bool ChangeResourceEx(T res_set)
		{
			if (res) {
				if (!(res_set->GetType() == res->GetType() && res_set->GetResName() == res->GetResName())) {
						ReleaseResource();
				}
			}
			switch (res_set->GetType()) {
			case ResourceType::Sprite: {
				res = res_set;
				res->AddRef();
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				a = res_set->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
				b = res_set->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
				a = res_set->GetHalfSizeX();
				b = res_set->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
				rect = res_set->IsRectangle();
				UpdateCollisionCirclrRadius();
				return true;
			}
			case ResourceType::Animation: {
				res = res_set;
				res->AddRef();
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				a = res_set->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
				b = res_set->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
				a = res_set->GetHalfSizeX();
				b = res_set->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
				rect = res_set->IsRectangle();
				UpdateCollisionCirclrRadius();
				return true;
			}
			case ResourceType::Particle: {
				//*
				fcyRefPointer<ResParticle> _res = static_cast<fcyRefPointer<ResParticle>>(res_set);
				res = _res;
				if (!(ps = _res->AllocInstance()))
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
#ifdef GLOBAL_SCALE_COLLI_SHAPE
				a = _res->GetHalfSizeX() * LRES.GetGlobalImageScaleFactor();
				b = _res->GetHalfSizeY() * LRES.GetGlobalImageScaleFactor();
#else
				a = _res->GetHalfSizeX();
				b = _res->GetHalfSizeY();
#endif // GLOBAL_SCALE_COLLI_SHAPE
				rect = _res->IsRectangle();
				UpdateCollisionCirclrRadius();
				return true;
				//*/
			}
			}
			return false;
		}
	};

	/// @brief 曲线激光特化实现
#define DEFINE_GAME_OBJECT_BENTLAZER_CLASS //防止重复定义
	class GameObjectBentLaser
	{
	public:
		static GameObjectBentLaser* AllocInstance();
		static void FreeInstance(GameObjectBentLaser* p);
	private:
		struct LaserNode
		{
			fcyVec2 pos;
			float half_width;
			float rot;
			float dis;
			float x_dir;
			float y_dir;
			bool active;
			bool sharp;

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

	/// @brief 游戏对象池
	class GameObjectPool
	{
	private:
		lua_State* L = nullptr;
		FixedObjectPool<GameObject, LGOBJ_MAXCNT> m_ObjectPool;

		// 链表伪头部
		uint64_t m_iUid = 0;
		GameObject m_pObjectListHeader, m_pObjectListTail;
		GameObject m_pRenderListHeader, m_pRenderListTail;
		GameObject m_pCollisionListHeader[LGOBJ_GROUPCNT], m_pCollisionListTail[LGOBJ_GROUPCNT];

		GameObject* m_pCurrentObject;

		// 场景边界
		lua_Number m_BoundLeft = -100.f;
		lua_Number m_BoundRight = 100.f;
		lua_Number m_BoundTop = 100.f;
		lua_Number m_BoundBottom = -100.f;
	private:
		GameObject* freeObject(GameObject* p)LNOEXCEPT;
	public:
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
		//void BoundCheckWorld(lua_Number worldflag)LNOEXCEPT;

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
		bool DoDefaultRender(size_t id)LNOEXCEPT;

		/// @brief 获取下一个元素的ID
		/// @return 返回-1表示无元素
		int NextObject(int groupId, int id)LNOEXCEPT;

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
		
	public:  // 内部使用
		void DrawGroupCollider(f2dGraphics2D* graph, f2dGeometryRenderer* grender, int groupId, fcyColor fillColor);
		static bool CheckWorld(lua_Integer gameworld, lua_Integer objworld){
			return (gameworld == objworld) || (gameworld&objworld);
		}
		int PushCurrentObject(lua_State* L)LNOEXCEPT;
	private:
		GameObjectPool& operator=(const GameObjectPool&);
		GameObjectPool(const GameObjectPool&);
	public:
		GameObjectPool(lua_State* pL);
		~GameObjectPool();
	};
}
