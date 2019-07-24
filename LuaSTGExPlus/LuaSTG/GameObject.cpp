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
}
