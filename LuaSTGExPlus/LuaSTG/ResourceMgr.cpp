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

////////////////////////////////////////////////////////////////////////////////
/// ResAnimation
////////////////////////////////////////////////////////////////////////////////
ResAnimation::ResAnimation(const char* name, fcyRefPointer<ResTexture> tex, float x, float y, float w, float h,
	int n, int m, int intv, double a, double b, bool rect)
	: Resource(ResourceType::Animation, name), m_Interval(intv), m_HalfSizeX(a), m_HalfSizeY(b), m_bRectangle(rect)
{
	LASSERT(LAPP.GetRenderer());

	// 分割纹理
	for (int j = 0; j < m; ++j)  // 行
	{
		for (int i = 0; i < n; ++i)  // 列
		{
			fcyRefPointer<f2dSprite> t;
			if (FCYFAILED(LAPP.GetRenderer()->CreateSprite2D(tex->GetTexture(), fcyRect(
				x + w * i, y + h * j, x + w * (i + 1), y + h * (j + 1)
				), &t)))
			{
				throw fcyException("ResAnimation::ResAnimation", "CreateSprite2D failed.");
			}
			t->SetZ(0.5f);
			t->SetColor(0xFFFFFFFF);
			m_ImageSequences.push_back(t);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// ResMusic
////////////////////////////////////////////////////////////////////////////////
fResult ResMusic::BGMWrapper::Read(fData pBuffer, fuInt SizeToRead, fuInt* pSizeRead)
{
	fResult tFR;

	// 获得单个采样大小
	fuInt tBlockAlign = GetBlockAlign();

	// 计算需要读取的采样个数
	fuInt tSampleToRead = SizeToRead / tBlockAlign;

	// 填充音频数据
	while (tSampleToRead)
	{
		// 获得当前解码器位置(采样)
		fuInt tCurSample = (fuInt)GetPosition() / tBlockAlign;

		// 检查读取位置是否超出循环节
		if (tCurSample + tSampleToRead > m_pLoopEndSample)
		{
			// 填充尚未填充数据
			if (tCurSample < m_pLoopEndSample)
			{
				fuInt tVaildSample = m_pLoopEndSample - tCurSample;
				fuInt tVaildSize = tVaildSample * tBlockAlign;

				if (FAILED(tFR = m_pDecoder->Read(pBuffer, tVaildSize, pSizeRead)))
					return tFR;

				// 指针后移
				pBuffer += tVaildSize;

				// 减少采样
				tSampleToRead -= tVaildSample;
			}

			// 跳到循环头
			SetPosition(FCYSEEKORIGIN_BEG, m_pLoopStartSample * tBlockAlign);
		}
		else
		{
			// 直接填充数据
			if (FAILED(tFR = m_pDecoder->Read(pBuffer, tSampleToRead * tBlockAlign, pSizeRead)))
				return tFR;

			break;
		}
	}

	if (pSizeRead)
		*pSizeRead = SizeToRead;

	return FCYERR_OK;
}

ResMusic::BGMWrapper::BGMWrapper(fcyRefPointer<f2dSoundDecoder> pOrg, fDouble LoopStart, fDouble LoopEnd)
	: m_pDecoder(pOrg)
{
	LASSERT(pOrg);

	// 计算参数
	m_TotalSample = m_pDecoder->GetBufferSize() / m_pDecoder->GetBlockAlign();

	if (LoopStart < 0)
		LoopStart = 0;
	m_pLoopStartSample = (fuInt)(LoopStart * m_pDecoder->GetSamplesPerSec());

	if (LoopEnd <= 0)
		m_pLoopEndSample = m_TotalSample;
	else
		m_pLoopEndSample = min(m_TotalSample, (fuInt)(LoopEnd * m_pDecoder->GetSamplesPerSec()));

	if (m_pLoopEndSample < m_pLoopStartSample)
		std::swap(m_pLoopStartSample, m_pLoopEndSample);

	if (m_pLoopEndSample == m_pLoopStartSample)
		throw fcyException("ResMusic::BGMWrapper::BGMWrapper", "Invalid loop period.");
}

////////////////////////////////////////////////////////////////////////////////
/// ResourceMgr
////////////////////////////////////////////////////////////////////////////////
ResourceMgr::ResourceMgr()
	: m_GlobalResourcePool(this, ResourcePoolType::Global), m_StageResourcePool(this, ResourcePoolType::Stage)
{
}

void ResourceMgr::ClearAllResource()LNOEXCEPT
{
	m_GlobalResourcePool.Clear();
	m_StageResourcePool.Clear();
	m_ActivedPool = ResourcePoolType::Global;
	m_GlobalImageScaleFactor = 1.;
	m_GlobalSoundEffectVolume = 1.0f;
	m_GlobalMusicVolume = 1.0f;
}

bool ResourceMgr::LoadPack(const wchar_t* path, const char* passwd)LNOEXCEPT
{
	try
	{
		wstring tPath = path;
		pathUniform(tPath.begin(), tPath.end());
		for (auto& i : m_ResPackList)
		{
			if (i.GetPathLowerCase() == tPath)
			{
				LWARNING("ResourceMgr: 资源包'%s'已加载，不能重复加载", path);
				return true;
			}
		}
		m_ResPackList.emplace_front(path, passwd);
		LINFO("ResourceMgr: 已装载资源包'%s'", path);
		return true;
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 加载资源包时无法分配足够内存");
	}
	catch (const fcyException&)
	{
	}
	return false;
}

void ResourceMgr::UnloadPack(const wchar_t* path)LNOEXCEPT
{
	try
	{
		wstring tPath = path;
		pathUniform(tPath.begin(), tPath.end());
		for (auto i = m_ResPackList.begin(); i != m_ResPackList.end(); ++i)
		{
			if (i->GetPathLowerCase() == tPath)
			{
				m_ResPackList.erase(i);
				LINFO("ResourceMgr: 已卸载资源包'%s'", path);
				return;
			}
		}
		LWARNING("ResourceMgr: 资源包'%s'未加载，无法卸载", path);
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 卸载资源包时无法分配足够内存");
	}
}

LNOINLINE bool ResourceMgr::LoadPack(const char* path, const char* passwd)LNOEXCEPT
{
	try
	{
		wstring tPath = fcyStringHelper::MultiByteToWideChar(path, CP_UTF8);
		return LoadPack(tPath.c_str(), passwd);
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
		return false;
	}
}

LNOINLINE void ResourceMgr::UnloadPack(const char* path)LNOEXCEPT
{
	try
	{
		wstring tPath = fcyStringHelper::MultiByteToWideChar(path, CP_UTF8);
		UnloadPack(tPath.c_str());
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
	}
}

LNOINLINE bool ResourceMgr::LoadFile(const wchar_t* path, fcyRefPointer<fcyMemStream>& outBuf, const wchar_t *packname)LNOEXCEPT
{
	// 尝试从各个资源包加载
	for (auto& i : m_ResPackList)
	{
		if (packname){
			if (i.GetPathLowerCase() != packname){
				continue;
			}
		}
		if (i.LoadFile(path, outBuf))
			return true;
	}

	// 尝试从本地加载
#ifdef LSHOWRESLOADINFO
	LINFO("ResourceMgr: 尝试从本地加载文件'%s'", path);
#endif
	fcyRefPointer<fcyFileStream> pFile;
	try
	{
		pFile.DirectSet(new fcyFileStream(path, false));
		outBuf.DirectSet(new fcyMemStream(NULL, pFile->GetLength(), true, false));
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 无法分配足够内存从本地加载文件'%s'", path);
		return false;
	}
	catch (const fcyException& e)
	{
		LERROR("ResourceMgr: 装载本地文件'%s'失败，文件不存在？ (异常信息'%m' 源'%m')", path, e.GetDesc(), e.GetSrc());
		return false;
	}

	if (pFile->GetLength() > 0)
	{
		if (FCYFAILED(pFile->ReadBytes((fData)outBuf->GetInternalBuffer(), outBuf->GetLength(), nullptr)))
		{
			LERROR("ResourceMgr: 读取本地文件'%s'失败 (fcyFileStream::ReadBytes失败)", path);
			return false;
		}
	}

	return true;
}

LNOINLINE bool ResourceMgr::LoadFile(const char* path, fcyRefPointer<fcyMemStream>& outBuf, const char *packname)LNOEXCEPT
{
	try
	{
		wstring tPath = fcyStringHelper::MultiByteToWideChar(path, CP_UTF8);
		wstring tPack = packname?fcyStringHelper::MultiByteToWideChar(packname, CP_UTF8):L"";
		pathUniform(tPack.begin(), tPack.end());
		return LoadFile(tPath.c_str(), outBuf, packname?tPack.c_str():NULL);
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
	}
	return false;
}

bool ResourceMgr::ExtractRes(const wchar_t* path, const wchar_t* target)LNOEXCEPT
{
	fcyRefPointer<fcyMemStream> tBuf;

	// 读取文件
	if (LoadFile(path, tBuf))
	{
		// 打开本地文件
		fcyRefPointer<fcyFileStream> pFile;
		try
		{
			pFile.DirectSet(new fcyFileStream(target, true));
			if (FCYFAILED(pFile->SetLength(0)))
			{
				LERROR("ResourceMgr: 无法清空文件'%s' (fcyFileStream::SetLength 失败)", target);
				return false;
			}
			if (tBuf->GetLength() > 0)
			{
				if (FCYFAILED(pFile->WriteBytes((fcData)tBuf->GetInternalBuffer(), tBuf->GetLength(), nullptr)))
				{
					LERROR("ResourceMgr: 无法向文件'%s'写出数据", target);
					return false;
				}
			}
		}
		catch (const bad_alloc&)
		{
			LERROR("ResourceMgr: 无法分配足够内存来向文件'%s'写出数据", target);
			return false;
		}
		catch (const fcyException& e)
		{
			LERROR("ResourceMgr: 打开本地文件'%s'失败 (异常信息'%m' 源'%m')", target, e.GetDesc(), e.GetSrc());
			return false;
		}
	}
	return true;
}

LNOINLINE bool ResourceMgr::ExtractRes(const char* path, const char* target)LNOEXCEPT
{
	try
	{
		wstring tPath = fcyStringHelper::MultiByteToWideChar(path, CP_UTF8);
		wstring tTarget = fcyStringHelper::MultiByteToWideChar(target, CP_UTF8);
		return ExtractRes(tPath.c_str(), tTarget.c_str());
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
	}
	return false;
}

bool listFiles(lua_State *L,const char * dirw,const char *ext,int *cnt)
{
	
	//*
	char dir[400];
	char fulldir[400];
	strcpy_s<400>(dir, dirw);
	int pn = strlen(dirw);
	if (dir[pn - 1] != '\\' && dir[pn - 1] != '/'){
		dir[pn] = '\\';
		pn++;
	}
	intptr_t handle;
	_finddata_t findData;
	
	GetCurrentDirectoryA(400, fulldir); // workdir 
	strcat(fulldir, "\\");              // workdir\ 
	strcat(fulldir, dir);               // workdir\searchdir 
	strcat(fulldir, "*.*");             // workdir\searchdir\*.* 
	handle = _findfirst(fulldir, &findData);    // 查找目录中的第一个文件
	if (handle == -1)
	{
		return false;
	}

	do
	{
		if (findData.attrib & _A_SUBDIR
			&& strcmp(findData.name, ".") == 0
			&& strcmp(findData.name, "..") == 0
			)    // 是否是子目录并且不为"."或".."
			;
		else
		{
			//cout << findData.name << "\t" << findData.size << endl;
			int n = strlen(findData.name);
			bool flag = true;
			if (ext){
				int extn = strlen(ext);
				if (n > extn)
				{
					for (int i = 0; i < extn; i++){
						if (findData.name[i + n - extn] != ext[i]){
							flag = false;
						}
					}
				}
				else{
					flag = false;
				}
			}
			if (flag){
				strcpy(dir + pn, findData.name);

				string a = dir;
				wstring temp = fcyStringHelper::MultiByteToWideChar(a, CP_ACP);
				a = fcyStringHelper::WideCharToMultiByte(temp, CP_UTF8);

				char utf8fer[460];
				CODEDSTR str;
				str.LoadBuffer(dir,0);
				str = CP_UTF8;
				str.getstring(utf8fer, 459);

				lua_newtable(L);// t t1
				lua_pushstring(L, a.c_str());  // t t1 s
				lua_rawseti(L, -2, 1);  // t t1
				lua_rawseti(L, -2, *cnt);  // t
				(*cnt)++;
			}
		}
	} while (_findnext(handle, &findData) == 0);    // 查找目录中的下一个文件

	_findclose(handle);    // 关闭搜索句柄
	//*/
	return true;
}

LNOINLINE bool ResourceMgr::FindFiles(lua_State *L,const char* path,const char *ext, const char *packname)LNOEXCEPT
{
	// 尝试从各个资源包加载
	lua_newtable(L);
	int cnt = 1;
	/*
	//为什么用path，不应该是用packname吗艹
	wstring tPath;
	try
	{
		tPath = fcyStringHelper::MultiByteToWideChar(path, CP_UTF8);
		pathUniform(tPath.begin(), tPath.end());
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
	}
	*/
	wstring tPackPath;
	try
	{
		tPackPath = fcyStringHelper::MultiByteToWideChar(packname, CP_UTF8);
		pathUniform(tPackPath.begin(), tPackPath.end());
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourceMgr: 转换字符编码时无法分配内存");
	}

	for (auto& i : m_ResPackList)
	{
		if (packname){
			//LINFO("尝试将资源包路径'%s'与查找资源包路径'%s'对比", i.GetPathLowerCase().c_str(), tPackPath.c_str());
			if (i.GetPathLowerCase() != tPackPath){
				continue;
			}
		}
		//LINFO("资源包'%s'匹配成功", i.GetPathLowerCase().c_str());
		i.FindFiles(L, &cnt, path, ext);
	}
	if (!packname || packname[0]==0)
		listFiles(L, path, ext, &cnt);
	return true;
}
