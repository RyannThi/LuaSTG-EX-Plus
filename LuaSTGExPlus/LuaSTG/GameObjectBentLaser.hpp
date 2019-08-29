#pragma once
#include "Global.h"
#include "CirularQueue.hpp"
#include "GameObject.hpp"

namespace LuaSTGPlus
{
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

			LaserNode() {
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
		bool Update(size_t id, int length, float width, bool active)LNOEXCEPT;
		bool UpdateByNode(size_t id, int node, int length, float width, bool active)LNOEXCEPT;
		bool RecalRot()LNOEXCEPT;
		bool CompileNode(size_t i)LNOEXCEPT;
		bool UpdatePositionByList(lua_State* L, int length, float width, int index, bool revert)LNOEXCEPT;
		void Release()LNOEXCEPT;
		bool Render(const char* tex_name, BlendMode blend, fcyColor c, float tex_left, float tex_top, float tex_width, float tex_height, float scale)LNOEXCEPT;
		bool CollisionCheck(float x, float y, float rot, float a, float b, bool rect)LNOEXCEPT;
		void RenderCollider(fcyColor fillColor)LNOEXCEPT;
		bool CollisionCheckW(float x, float y, float rot, float a, float b, bool rect, float width)LNOEXCEPT;
		bool BoundCheck()LNOEXCEPT;
		int SampleL(lua_State* L, float length)LNOEXCEPT;
		int UpdateLength() LNOEXCEPT;
		int SampleT(lua_State* L, float delay) LNOEXCEPT;
		void SetAllWidth(float width)LNOEXCEPT;
	protected:
		GameObjectBentLaser();
		~GameObjectBentLaser();
	};
}
