#include "ResourceMgr.h"
#include "AppFrame.h"
#include "Utility.h"

#include <iowin32.h>
#include <io.h>
#include "ESC.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace std;
using namespace LuaSTGPlus;

#if (defined LDEVVERSION) || (defined LDEBUG)
#define LDEBUG_RESOURCETIMER float tResourceLoadingTime
#define LDEBUG_RESOURCESCOPE TimerScope tLoadingTimer(tResourceLoadingTime)
#define LDEBUG_RESOURCEHINT(t, path) \
    LAPP.SendResourceLoadedHint(t, m_iType, name, path, tResourceLoadingTime)
#else
#define LDEBUG_RESOURCETIMER
#define LDEBUG_RESOURCESCOPE
#define LDEBUG_RESOURCEHINT
#endif

void ResourcePool::Clear()LNOEXCEPT
{
	m_TexturePool.clear();
	m_SpritePool.clear();
	m_AnimationPool.clear();
	m_MusicPool.clear();
	m_SoundSpritePool.clear();
	m_ParticlePool.clear();
	m_SpriteFontPool.clear();
	m_TTFFontPool.clear();
	m_FXPool.clear();

#if (defined LDEVVERSION) || (defined LDEBUG)
	LAPP.SendResourceClearedHint(m_iType);
#endif
}

void ResourcePool::RemoveResource(ResourceType t, const char* name)LNOEXCEPT
{
	switch (t)
	{
	case ResourceType::Texture:
		removeResource(m_TexturePool, name);
		break;
	case ResourceType::Sprite:
		removeResource(m_SpritePool, name);
		break;
	case ResourceType::Animation:
		removeResource(m_AnimationPool, name);
		break;
	case ResourceType::Music:
		removeResource(m_MusicPool, name);
		break;
	case ResourceType::SoundEffect:
		removeResource(m_SoundSpritePool, name);
		break;
	case ResourceType::Particle:
		removeResource(m_ParticlePool, name);
		break;
	case ResourceType::SpriteFont:
		removeResource(m_SpriteFontPool, name);
		break;
	case ResourceType::TrueTypeFont:
		removeResource(m_TTFFontPool, name);
		break;
	case ResourceType::FX:
		removeResource(m_FXPool, name);
		break;
	default:
		return;
	}

#if (defined LDEVVERSION) || (defined LDEBUG)
	LAPP.SendResourceRemovedHint(t, m_iType, name);
#endif
}

void ResourcePool::ExportResourceList(lua_State* L, ResourceType t)const LNOEXCEPT
{
	int cnt = 1;
	switch (t)
	{
	case ResourceType::Texture:
		lua_newtable(L);  // t
		for (auto& i : m_TexturePool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::Sprite:
		lua_newtable(L);  // t
		for (auto& i : m_SpritePool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::Animation:
		lua_newtable(L);  // t
		for (auto& i : m_AnimationPool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::Music:
		lua_newtable(L);  // t
		for (auto& i : m_MusicPool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::SoundEffect:
		lua_newtable(L);  // t
		for (auto& i : m_SoundSpritePool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::Particle:
		lua_newtable(L);  // t
		for (auto& i : m_ParticlePool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::SpriteFont:
		lua_newtable(L);  // t
		for (auto& i : m_SpriteFontPool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::TrueTypeFont:
		lua_newtable(L);  // t
		for (auto& i : m_TTFFontPool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	case ResourceType::FX:
		lua_newtable(L);  // t
		for (auto& i : m_FXPool)
		{
			lua_pushstring(L, i.second->GetResName().c_str());  // t s
			lua_rawseti(L, -2, cnt++);  // t
		}
		break;
	default:
		lua_pushnil(L);
		break;
	}
}

bool ResourcePool::LoadTexture(const char* name, const std::wstring& path, bool mipmaps)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderDev());

		if (m_TexturePool.find(name) != m_TexturePool.end())
		{
			LWARNING("LoadTexture: ����'%m'�Ѵ��ڣ���ͼʹ��'%s'���صĲ����ѱ�ȡ��", name, path.c_str());
			return true;
		}

		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		fcyRefPointer<f2dTexture2D> tTexture;
		if (FCYFAILED(LAPP.GetRenderDev()->CreateTextureFromMemory((fcData)tDataBuf->GetInternalBuffer(), tDataBuf->GetLength(), 0, 0, false, mipmaps, &tTexture)))
		{
			LERROR("LoadTexture: ���ļ�'%s'��������'%m'ʧ��", path.c_str(), name);
			return false;
		}

		try
		{
			fcyRefPointer<ResTexture> tRes;
			tRes.DirectSet(new ResTexture(name, tTexture));
			m_TexturePool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadTexture: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadTexture: ����'%s'��װ�� -> '%m' (%s)", path.c_str(), name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Texture, path.c_str());
	return true;
}

bool ResourcePool::LoadModel(const char* name, const std::wstring& path)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderDev());

		if (m_TexturePool.find(name) != m_TexturePool.end())
		{
			LWARNING("LoadModel: ģ��'%m'�Ѵ��ڣ���ͼʹ��'%s'���صĲ����ѱ�ȡ��", name, path.c_str());
			return true;
		}

		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;
		fcyRefPointer<fcyMemStream> tDataBuf2;
		std::wstring path2 = path;
		int i = path2.length();
		path2[i - 3] = 'm';
		path2[i - 2] = 't';
		path2[i - 1] = 'l';
		if (!m_pMgr->LoadFile(path2.c_str(), tDataBuf2))
			return false;
		void* model = NULL;
		void* LoadObj(const string & id, const string & path, const string & path2);
		string buf((char*)tDataBuf->GetInternalBuffer(), tDataBuf->GetLength());
		string buf2((char*)tDataBuf2->GetInternalBuffer(), tDataBuf2->GetLength());
		model = LoadObj(name, buf, buf2);

		try
		{
			fcyRefPointer<ResModel> tRes;
			tRes.DirectSet(new ResModel(name, model));
			m_ModelPool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadModel: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadTexture: ����'%s'��װ�� -> '%m' (%s)", path.c_str(), name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Texture, path.c_str());
	return true;
}

bool ResourcePool::LoadModel(const char* name, const char* path)LNOEXCEPT
{
	try
	{
		return LoadModel(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8));
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadModel: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadTexture(const char* name, const char* path, bool mipmaps)LNOEXCEPT
{
	try
	{
		return LoadTexture(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), mipmaps);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadTexture: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadImage(const char* name, const char* texname,
	double x, double y, double w, double h, double a, double b, bool rect)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderer());

		if (m_SpritePool.find(name) != m_SpritePool.end())
		{
			LWARNING("LoadImage: ͼ��'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		fcyRefPointer<ResTexture> pTex = m_pMgr->FindTexture(texname);
		if (!pTex)
		{
			LWARNING("LoadImage: ����ͼ��'%m'ʧ��, �޷��ҵ�����'%m'", name, texname);
			return false;
		}

		fcyRefPointer<f2dSprite> pSprite;
		fcyRect tRect((float)x, (float)y, (float)(x + w), (float)(y + h));
		if (FCYFAILED(LAPP.GetRenderer()->CreateSprite2D(pTex->GetTexture(), tRect, &pSprite)))
		{
			LERROR("LoadImage: �޷�������'%m'����ͼ��'%m' (CreateSprite2D failed)", texname, name);
			return false;
		}

		try
		{
			fcyRefPointer<ResSprite> tRes;
			tRes.DirectSet(new ResSprite(name, pSprite, a, b, rect));
			m_SpritePool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadImage: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadImage: ͼ��'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Sprite, L"N/A");
	return true;
}

bool ResourcePool::LoadAnimation(const char* name, const char* texname,
	double x, double y, double w, double h, int n, int m, int intv, double a, double b, bool rect)
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		if (m_AnimationPool.find(name) != m_AnimationPool.end())
		{
			LWARNING("LoadAnimation: ����'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		fcyRefPointer<ResTexture> pTex = m_pMgr->FindTexture(texname);
		if (!pTex)
		{
			LWARNING("LoadAnimation: ���ض���'%m'ʧ��, �޷��ҵ�����'%m'", name, texname);
			return false;
		}

		try
		{
			fcyRefPointer<ResAnimation> tRes;
			tRes.DirectSet(new ResAnimation(name, pTex, (float)x, (float)y, (float)w, (float)h, n, m, intv, a, b, rect));
			m_AnimationPool.emplace(name, tRes);
		}
		catch (const fcyException&)
		{
			LERROR("LoadAnimation: ���춯��'%m'ʱʧ��", name);
			return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadAnimation: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadAnimation: ����'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Animation, L"N/A");
	return true;
}

bool ResourcePool::LoadMusic(const char* name, const std::wstring& path, double start, double end)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		//�����Ƶϵͳ�Ƿ��Ѿ���ʼ��
		LASSERT(LAPP.GetSoundSys());

		//������Ƶ�ļ��������ص��ڴ�
		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		try
		{
			//���ؽ�����������OGG������������ʧ����ʹ��WAV����������ʧ����error����
			fcyRefPointer<f2dSoundDecoder> tDecoder;
			if (FCYFAILED(LAPP.GetSoundSys()->CreateOGGVorbisDecoder(tDataBuf, &tDecoder)))
			{
				tDataBuf->SetPosition(FCYSEEKORIGIN_BEG, 0);
				if (FCYFAILED(LAPP.GetSoundSys()->CreateWaveDecoder(tDataBuf, &tDecoder)))
				{
					LERROR("LoadMusic: �޷������ļ�'%s'", path.c_str());
					return false;
				}
			}

			//���ؽ�����
			fcyRefPointer<ResMusic::BGMWrapper> tWrapperedBuffer;
			tWrapperedBuffer.DirectSet(new ResMusic::BGMWrapper(tDecoder, start, end));

			//������Ƶ����������̬������
			fcyRefPointer<f2dSoundBuffer> tBuffer;
			if (FCYFAILED(LAPP.GetSoundSys()->CreateDynamicBuffer(tWrapperedBuffer, LSOUNDGLOBALFOCUS, &tBuffer)))
			{
				LERROR("LoadMusic: �޷�������Ƶ���������ļ�'%s' (f2dSoundSys::CreateDynamicBuffer failed.)", path.c_str());
				return false;
			}

			//������Դ��
			fcyRefPointer<ResMusic> tRes;
			tRes.DirectSet(new ResMusic(name, tBuffer));
			m_MusicPool.emplace(name, tRes);
		}
		catch (const fcyException& e)
		{
			LERROR("LoadMusic: �����ļ�'%s'����Ƶ����ʱ�������󣬸�ʽ��֧�֣� (�쳣��Ϣ'%m' Դ'%m')", path.c_str(), e.GetDesc(), e.GetSrc());
			return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadMusic: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadMusic: BGM'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Music, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadMusic(const char* name, const char* path, double start, double end)LNOEXCEPT
{
	try
	{
		return LoadMusic(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), start, end);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadMusic: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadSound(const char* name, const std::wstring& path)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetSoundSys());

		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		try
		{
			fcyRefPointer<f2dSoundDecoder> tDecoder;
			if (FCYFAILED(LAPP.GetSoundSys()->CreateWaveDecoder(tDataBuf, &tDecoder)))
			{
				tDataBuf->SetPosition(FCYSEEKORIGIN_BEG, 0);
				if (FCYFAILED(LAPP.GetSoundSys()->CreateOGGVorbisDecoder(tDataBuf, &tDecoder)))
				{
					LERROR("LoadSound: �޷������ļ�'%s'", path.c_str());
					return false;
				}
			}

			fcyRefPointer<f2dSoundBuffer> tBuffer;
			if (FCYFAILED(LAPP.GetSoundSys()->CreateStaticBuffer(tDecoder, LSOUNDGLOBALFOCUS, &tBuffer)))
			{
				LERROR("LoadSound: �޷�������Ƶ���������ļ�'%s' (f2dSoundSys::CreateStaticBuffer failed.)", path.c_str());
				return false;
			}

			fcyRefPointer<ResSound> tRes;
			tRes.DirectSet(new ResSound(name, tBuffer));
			m_SoundSpritePool.emplace(name, tRes);
		}
		catch (const fcyException& e)
		{
			LERROR("LoadSound: �����ļ�'%s'����Ƶ����ʱ�������󣬸�ʽ��֧�֣� (�쳣��Ϣ'%m' Դ'%m')", path.c_str(), e.GetDesc(), e.GetSrc());
			return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadSound: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("LoadSound: ��Ч'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::SoundEffect, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadSound(const char* name, const char* path)LNOEXCEPT
{
	try
	{
		return LoadSound(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8));
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadSound: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadParticle(const char* name, const std::wstring& path, const char* img_name, double a, double b, bool rect)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderer());

		if (m_ParticlePool.find(name) != m_ParticlePool.end())
		{
			LWARNING("LoadParticle: ����'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		fcyRefPointer<ResSprite> pSprite = m_pMgr->FindSprite(img_name);
		fcyRefPointer<f2dSprite> pClone;
		if (!pSprite)
		{
			LWARNING("LoadParticle: ��������'%m'ʧ��, �޷��ҵ�����'%m'", name, img_name);
			return false;
		}
		else
		{
			// ��¡һ���������
			if (FCYFAILED(LAPP.GetRenderer()->CreateSprite2D(pSprite->GetSprite()->GetTexture(), pSprite->GetSprite()->GetTexRect(), pSprite->GetSprite()->GetHotSpot(), &pClone)))
			{
				LERROR("LoadParticle: ��¡ͼƬ'%m'ʧ��", img_name);
				return false;
			}
			pClone->SetColor(0, pSprite->GetSprite()->GetColor(0U));
			pClone->SetColor(1, pSprite->GetSprite()->GetColor(1U));
			pClone->SetColor(2, pSprite->GetSprite()->GetColor(2U));
			pClone->SetColor(3, pSprite->GetSprite()->GetColor(3U));
			pClone->SetZ(pSprite->GetSprite()->GetZ());
		}

		fcyRefPointer<fcyMemStream> outBuf;
		if (!LRES.LoadFile(path.c_str(), outBuf))
			return false;
		if (outBuf->GetLength() != sizeof(ResParticle::ParticleInfo))
		{
			LERROR("LoadParticle: ���Ӷ����ļ�'%s'��ʽ����ȷ", path.c_str());
			return false;
		}

		try
		{
			ResParticle::ParticleInfo tInfo;
			memcpy(&tInfo, outBuf->GetInternalBuffer(), sizeof(ResParticle::ParticleInfo));
			tInfo.iBlendInfo = (tInfo.iBlendInfo >> 16) & 0x00000003;

			BlendMode tBlendInfo = BlendMode::AddAlpha;
			if (tInfo.iBlendInfo & 1)  // ADD
			{
				if (tInfo.iBlendInfo & 2)  // ALPHA
					tBlendInfo = BlendMode::AddAlpha;
				else
					tBlendInfo = BlendMode::AddAdd;
			}
			else  // MUL
			{
				if (tInfo.iBlendInfo & 2)  // ALPHA
					tBlendInfo = BlendMode::MulAlpha;
				else
					tBlendInfo = BlendMode::MulAdd;
			}

			fcyRefPointer<ResParticle> tRes;
			tRes.DirectSet(new ResParticle(name, tInfo, pClone, tBlendInfo, a, b, rect));
			m_ParticlePool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadParticle: �ڴ治��");
			return false;
		}
#ifdef LSHOWRESLOADINFO
		LINFO("LoadParticle: ����'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Particle, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadParticle(const char* name, const char* path, const char* img_name, double a, double b, bool rect)LNOEXCEPT
{
	try
	{
		return LoadParticle(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), img_name, a, b, rect);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadParticle: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadSpriteFont(const char* name, const std::wstring& path, bool mipmaps)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderer());

		if (m_SpriteFontPool.find(name) != m_SpriteFontPool.end())
		{
			LWARNING("LoadFont: ����'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		std::unordered_map<wchar_t, f2dGlyphInfo> tOutputCharset;
		std::wstring tOutputTextureName;

		// ��ȡ�ļ�
		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		// ת������
		wstring tFileData;
		try
		{
			if (tDataBuf->GetLength() > 0)
			{
				// stupid
				tFileData = fcyStringHelper::MultiByteToWideChar(string((const char*)tDataBuf->GetInternalBuffer(), (size_t)tDataBuf->GetLength()), CP_UTF8);
			}
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: ת������ʱ�޷������ڴ�");
			return false;
		}

		// ��ȡHGE���嶨��
		try
		{
			ResFont::HGEFont::ReadDefine(tFileData, tOutputCharset, tOutputTextureName);
		}
		catch (const fcyException& e)
		{
			LERROR("LoadFont: װ��HGE���嶨���ļ�'%s'ʧ�� (�쳣��Ϣ'%m' Դ'%m')", path.c_str(), e.GetDesc(), e.GetSrc());
			return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: �ڴ治��");
			return false;
		}

		// װ������
		try
		{
			if (!m_pMgr->LoadFile((fcyPathParser::GetPath(path) + tOutputTextureName).c_str(), tDataBuf))
				return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: �ڴ治��");
			return false;
		}

		fcyRefPointer<f2dTexture2D> tTexture;
		if (FCYFAILED(LAPP.GetRenderDev()->CreateTextureFromMemory((fcData)tDataBuf->GetInternalBuffer(), tDataBuf->GetLength(), 0, 0, false, mipmaps, &tTexture)))
		{
			LERROR("LoadFont: ���ļ�'%s'��������'%m'ʧ��", tOutputTextureName.c_str(), name);
			return false;
		}

		// ��������
		try
		{
			fcyRefPointer<f2dFontProvider> tFontProvider;
			tFontProvider.DirectSet(new ResFont::HGEFont(std::move(tOutputCharset), tTexture));

			fcyRefPointer<ResFont> tRes;
			tRes.DirectSet(new ResFont(name, tFontProvider));
			m_SpriteFontPool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: �ڴ治��");
			return false;
		}
#ifdef LSHOWRESLOADINFO
		LINFO("LoadFont: ��������'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::SpriteFont, path.c_str());
	return true;
}

bool ResourcePool::LoadSpriteFont(const char* name, const std::wstring& path, const std::wstring& tex_path, bool mipmaps)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderer());

		if (m_SpriteFontPool.find(name) != m_SpriteFontPool.end())
		{
			LWARNING("LoadFont: ����'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		// ��ȡ�ļ�
		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		// ת������
		wstring tFileData;
		try
		{
			if (tDataBuf->GetLength() > 0)
			{
				// stupid
				tFileData = fcyStringHelper::MultiByteToWideChar(string((const char*)tDataBuf->GetInternalBuffer(), (size_t)tDataBuf->GetLength()), CP_UTF8);
			}
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: ת������ʱ�޷������ڴ�");
			return false;
		}

		// װ������
		try
		{
			if (!m_pMgr->LoadFile((fcyPathParser::GetPath(path) + tex_path).c_str(), tDataBuf))
			{
				if (!m_pMgr->LoadFile(tex_path.c_str(), tDataBuf))
					return false;
			}
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: �ڴ治��");
			return false;
		}

		fcyRefPointer<f2dTexture2D> tTexture;
		if (FCYFAILED(LAPP.GetRenderDev()->CreateTextureFromMemory((fcData)tDataBuf->GetInternalBuffer(), tDataBuf->GetLength(), 0, 0, false, mipmaps, &tTexture)))
		{
			LERROR("LoadFont: ���ļ�'%s'��������'%m'ʧ��", tex_path.c_str(), name);
			return false;
		}

		// ��������
		try
		{
			fcyRefPointer<f2dFontProvider> tFontProvider;
			if (FCYFAILED(LAPP.GetRenderer()->CreateFontFromTex(tFileData.c_str(), tTexture, &tFontProvider)))
			{
				LERROR("LoadFont: ���ļ�'%s'������������ʧ��", path.c_str());
				return false;
			}

			fcyRefPointer<ResFont> tRes;
			tRes.DirectSet(new ResFont(name, tFontProvider));
			m_SpriteFontPool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFont: �ڴ治��");
			return false;
		}
#ifdef LSHOWRESLOADINFO
		LINFO("LoadFont: ��������'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::SpriteFont, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadSpriteFont(const char* name, const char* path, bool mipmaps)LNOEXCEPT
{
	try
	{
		return LoadSpriteFont(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), mipmaps);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadSpriteFont: ת������ʱ�޷������ڴ�");
		return false;
	}
}

LNOINLINE bool ResourcePool::LoadSpriteFont(const char* name, const char* path, const char* tex_path, bool mipmaps)LNOEXCEPT
{
	try
	{
		return LoadSpriteFont(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), fcyStringHelper::MultiByteToWideChar(tex_path, CP_UTF8), mipmaps);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadSpriteFont: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadTTFFont(const char* name, const std::wstring& path, float width, float height)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderer());

		if (m_TTFFontPool.find(name) != m_TTFFontPool.end())
		{
			LWARNING("LoadTTFFont: ����'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		fcyRefPointer<f2dFontProvider> tFontProvider;

		// ��ȡ�ļ�
		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
		{
			LINFO("LoadTTFFont: �޷���·��'%s'�ϼ������壬������ϵͳ����Դ�������ϵͳ����", path.c_str());
			if (FCYFAILED(LAPP.GetRenderer()->CreateSystemFont(path.c_str(), 0, fcyVec2(width, height), F2DFONTFLAG_NONE, &tFontProvider)))
			{
				//LERROR("LoadTTFFont: ����ʧ�ܣ��޷���·��'%s'�ϼ�������", path.c_str());
				LWARNING("LoadTTFFont: ����ʧ�ܣ��޷���ϵͳ������������'%s'", path.c_str());//��lua�㷵�ش��󣬶�����ֱ�ӱ���Ϸ
				return false;
			}
		}

		// ��������
		try
		{
			if (!tFontProvider)
			{
				if (FCYFAILED(LAPP.GetRenderer()->CreateFontFromFile(tDataBuf, 0, fcyVec2(width, height), F2DFONTFLAG_NONE, &tFontProvider)))
				{
					LERROR("LoadTTFFont: ���ļ�'%s'����TrueType����ʧ��", path.c_str());
					return false;
				}
			}
#ifdef LSHOWRESLOADINFO
			LINFO("���λ���������%d�����λ�����ͼ��С��%dx%d", tFontProvider->GetCacheCount(), tFontProvider->GetCacheTexSize(), tFontProvider->GetCacheTexSize());
#endif
			fcyRefPointer<ResFont> tRes;
			tRes.DirectSet(new ResFont(name, tFontProvider));
			tRes->SetBlendMode(BlendMode::AddAlpha);
			m_TTFFontPool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadTTFFont: �ڴ治��");
			return false;
		}
#ifdef LSHOWRESLOADINFO
		LINFO("LoadTTFFont: truetype����'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::TrueTypeFont, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadTTFFont(const char* name, const char* path, float width, float height)LNOEXCEPT
{
	try
	{
		return LoadTTFFont(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8), width, height);
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadTTFFont: ת������ʱ�޷������ڴ�");
		return false;
	}
}

bool ResourcePool::LoadFX(const char* name, const std::wstring& path)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderDev());

		if (m_FXPool.find(name) != m_FXPool.end())
		{
			LWARNING("LoadFX: FX'%m'�Ѵ��ڣ����ز����ѱ�ȡ��", name);
			return true;
		}

		// ��ȡ�ļ�
		fcyRefPointer<fcyMemStream> tDataBuf;
		if (!m_pMgr->LoadFile(path.c_str(), tDataBuf))
			return false;

		try
		{
			fcyRefPointer<f2dEffect> tEffect;
			if (FCYFAILED(LAPP.GetRenderDev()->CreateEffect(tDataBuf, false, &tEffect)))
			{
				LERROR("LoadFX: ����shader���ļ�'%s'ʧ�� (lasterr=%m)", path.c_str(), LAPP.GetEngine()->GetLastErrDesc());
				return false;
			}

			fcyRefPointer<ResFX> tRes;
			tRes.DirectSet(new ResFX(name, tEffect));
			m_FXPool.emplace(name, tRes);
		}
		catch (const fcyException& e)
		{
			LERROR("LoadFX: �󶨱������ļ�'%s'ʧ�� (�쳣��Ϣ'%m' Դ'%m')", path.c_str(), e.GetDesc(), e.GetSrc());
			return false;
		}
		catch (const bad_alloc&)
		{
			LERROR("LoadFX: �ڴ治��");
			return false;
		}
#ifdef LSHOWRESLOADINFO
		LINFO("LoadFX: FX'%m'��װ�� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::FX, path.c_str());
	return true;
}

LNOINLINE bool ResourcePool::LoadFX(const char* name, const char* path)LNOEXCEPT
{
	try
	{
		return LoadFX(name, fcyStringHelper::MultiByteToWideChar(path, CP_UTF8));
	}
	catch (const bad_alloc&)
	{
		LERROR("LoadFX: ת������ʱ�޷������ڴ�");
		return false;
	}
}

LNOINLINE bool ResourcePool::CreateRenderTarget(const char* name)LNOEXCEPT
{
	LDEBUG_RESOURCETIMER;

	{
		LDEBUG_RESOURCESCOPE;

		LASSERT(LAPP.GetRenderDev());

		if (m_TexturePool.find(name) != m_TexturePool.end())
		{
			LWARNING("CreateRenderTarget: '%m'�Ѵ��ڣ����������ѱ�ȡ��", name);
			return true;
		}

		fcyRefPointer<f2dTexture2D> tTexture;
		if (FCYFAILED(LAPP.GetRenderDev()->CreateRenderTarget(LAPP.GetRenderDev()->GetBufferWidth(),
			LAPP.GetRenderDev()->GetBufferHeight(), true, &tTexture)))
		{
			LERROR("CreateRenderTarget: ������ȾĿ��'%m'ʧ��", name);
			return false;
		}

		try
		{
			fcyRefPointer<ResTexture> tRes;
			tRes.DirectSet(new ResTexture(name, tTexture));
			m_TexturePool.emplace(name, tRes);
		}
		catch (const bad_alloc&)
		{
			LERROR("CreateRenderTarget: �ڴ治��");
			return false;
		}

#ifdef LSHOWRESLOADINFO
		LINFO("CreateRenderTarget: '%m'�Ѵ��� (%s)", name, getResourcePoolTypeName());
#endif
	}

	LDEBUG_RESOURCEHINT(ResourceType::Texture, L"[RenderTarget]");
	return true;
}
