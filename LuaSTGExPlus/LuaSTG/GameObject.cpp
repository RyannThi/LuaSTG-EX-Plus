#include "GameObject.hpp"
#include "AppFrame.h"
#include "ResourceSprite.hpp"
#include "ResourceAnimation.hpp"

namespace LuaSTGPlus {
	void GameObject::Reset() {
		status = (GAMEOBJECTSTATUS)STATUS_FREE;
		id = (size_t)-1;
		uid = 0;
#ifdef USING_ADVANCE_GAMEOBJECT_CLASS
		luaclass.Reset();
#endif // USING_ADVANCE_GAMEOBJECT_CLASS

		x = y = 0.;
		lastx = lasty = 0.;
		dx = dy = 0.;
		rot = omiga = 0.;
		vx = vy = 0.;
		ax = ay = 0.;
		layer = 0.;
		hscale = vscale = 1.;
#ifdef USER_SYSTEM_OPERATION
		maxv = maxvx = maxvy = DBL_HALF_MAX; // 平时应该不会有人弄那么大的速度吧，希望计算时不会溢出（
		ag = 0.;
#endif

		colli = bound = true;
		hide = navi = false;

		group = LGOBJ_DEFAULTGROUP;
		timer = ani_timer = 0;

		res = nullptr;
		ps = nullptr;

		resolve_move = false;
		pause = 0;
		ignore_superpause = false;

		world = 1;

#ifdef USING_ADVANCE_COLLIDER
		colliders[0].reset();
		colliders[0].type = GameObjectColliderType::Ellipse;
		for (int i = 1; i < MAX_COLLIDERS_COUNT; i++) {
			colliders[i].type = GameObjectColliderType::None;
		}
#else
		rect = false;
		a = b = 0.;
		col_r = 0.;
#endif // USING_ADVANCE_COLLIDER

#ifdef USING_ADVANCE_GAMEOBJECT_CLASS
		blendmode = BlendMode::MulAlpha;
		vertexcolor.argb = 0xFFFFFFFF;
#endif // USING_ADVANCE_GAMEOBJECT_CLASS
	}

	void GameObject::DirtReset()
	{
		status = (GAMEOBJECTSTATUS)STATUS_DEFAULT;
#ifdef USING_ADVANCE_GAMEOBJECT_CLASS
		luaclass.Reset();
#endif // USING_ADVANCE_GAMEOBJECT_CLASS

		x = y = 0.;
		lastx = lasty = 0.;
		dx = dy = 0.;
		rot = omiga = 0.;
		vx = vy = 0.;
		ax = ay = 0.;
		layer = 0.;
		hscale = vscale = 1.;
#ifdef USER_SYSTEM_OPERATION
		maxv = maxvx = maxvy = DBL_HALF_MAX; // 平时应该不会有人弄那么大的速度吧，希望计算时不会溢出（
		ag = 0.;
#endif

		colli = bound = true;
		hide = navi = false;

		group = LGOBJ_DEFAULTGROUP;
		timer = ani_timer = 0;

		ReleaseResource();

		resolve_move = false;
		pause = 0;
		ignore_superpause = false;

		world = 1;

#ifdef USING_ADVANCE_COLLIDER
		colliders[0].reset();
		colliders[0].type = GameObjectColliderType::Ellipse;
		for (int i = 1; i < MAX_COLLIDERS_COUNT; i++) {
			colliders[i].type = GameObjectColliderType::None;
		}
#else
		rect = false;
		a = b = 0.;
		col_r = 0.;
#endif // USING_ADVANCE_COLLIDER

#ifdef USING_ADVANCE_GAMEOBJECT_CLASS
		blendmode = BlendMode::MulAlpha;
		vertexcolor.argb = 0xFFFFFFFF;
#endif // USING_ADVANCE_GAMEOBJECT_CLASS
	}

	void GameObject::ReleaseResource()
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

	bool GameObject::ChangeResource(const char* res_name)
	{
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
			colliders[0].a = (float)tSprite->GetHalfSizeX();
			colliders[0].b = (float)tSprite->GetHalfSizeY();
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
			colliders[0].a = (float)tAnimation->GetHalfSizeX();
			colliders[0].b = (float)tAnimation->GetHalfSizeY();
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
		if (tParticle)
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
			colliders[0].a = (float)tParticle->GetHalfSizeX();
			colliders[0].b = (float)tParticle->GetHalfSizeY();
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

	bool CollisionCheck(GameObject* p1, GameObject* p2)LNOEXCEPT {
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
}
