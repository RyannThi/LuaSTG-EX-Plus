﻿////////////////////////////////////////////////////////////////////////////////
/// @file  f2dRendererImpl.h
/// @brief fancy2D渲染器接口实现
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../RenderDev/f2dRenderDeviceImpl.h"

#include "../f2dRenderer.h"

////////////////////////////////////////////////////////////////////////////////
/// @brief fancy2D渲染器实现
////////////////////////////////////////////////////////////////////////////////
class f2dRendererImpl :
	public fcyRefObjImpl<f2dRenderer>
{
private:
	class DefaultListener :
		public f2dRenderDeviceEventListener
	{
	protected:
		f2dEngineImpl* m_pEngine;
	public:
		void OnRenderDeviceLost();
		void OnRenderDeviceReset();
	public:
		DefaultListener(f2dEngineImpl* pEngine)
			: m_pEngine(pEngine) {}
	};
private:
	f2dEngineImpl* m_pEngine;

	f2dRenderDeviceImpl* m_pDev;
	DefaultListener m_DefaultListener;
public:
	fcyFileStream* enumSystemFont(fcStrW FontName);
public: // 接口实现
	f2dRenderDevice* GetDevice();

	fResult CreateSprite2D(f2dTexture2D* pTex, f2dSprite** pOut);
	fResult CreateSprite2D(f2dTexture2D* pTex, const fcyRect& Org, f2dSprite** pOut);
	fResult CreateSprite2D(f2dTexture2D* pTex, const fcyRect& Org, const fcyVec2& HotSopt, f2dSprite** pOut);

	fResult CreateGeometryRenderer(f2dGeometryRenderer** pOut);

	fResult CreateFontRenderer(f2dFontProvider* pProvider, f2dFontRenderer** pOut);
	fResult CreateFontFromFile(f2dStream* pStream, fuInt FaceIndex, const fcyVec2& FontSize, const fcyVec2& BBoxSize, F2DFONTFLAG Flag, f2dFontProvider** pOut);
	fResult CreateSystemFont(fcStrW FaceName, fuInt FaceIndex, const fcyVec2& FontSize, F2DFONTFLAG Flag, f2dFontProvider** pOut);
	fResult CreateFontFromTex(f2dStream* pDefineFile, f2dTexture2D* pTex, f2dFontProvider** pOut);
	fResult CreateFontFromTex(fcStrW pDefineText, f2dTexture2D* pTex, f2dFontProvider** pOut);
	fResult CreateSpriteAnimation(f2dSpriteAnimation** pOut);
	fResult CreateParticlePool(f2dParticlePool** pOut);
public:
	f2dRendererImpl(f2dEngineImpl* pEngine, fuInt BackBufferWidth, fuInt BackBufferHeight, fBool Windowed, fBool VSync, F2DAALEVEL AALevel);
	~f2dRendererImpl();
};
