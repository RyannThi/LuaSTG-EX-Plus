#include "ResourceMgr.h"

#include "AppFrame.h"
#include "Utility.h"
#include "ESC.h"

using namespace std;
using namespace LuaSTGPlus;

ResourcePack::ResourcePack(const wchar_t* path, const char* passwd)
	: m_Path(path), m_PathLowerCase(path), m_Password(passwd ? passwd : "")
{
	pathUniform(m_PathLowerCase.begin(), m_PathLowerCase.end());

	zlib_filefunc64_def tZlibFileFunc;
	memset(&tZlibFileFunc, 0, sizeof(tZlibFileFunc));
	fill_wfopen64_filefunc(&tZlibFileFunc);
	//fill_fopen64_filefunc(&tZlibFileFunc);
	m_zipFile = unzOpen2_64(reinterpret_cast<const char*>(path), &tZlibFileFunc);
	if (!m_zipFile) {
		//为了适配ANSI编码
		CODEDSTR path_unicode(CP_UNICODE);
		path_unicode.LoadBuffer(path, CP_UNICODE);
		path_unicode = CP_GBK;
		char buf[500];
		path_unicode.getstring(buf, 499);
		m_zipFile = unzOpen2_64(buf, &tZlibFileFunc);
	}
	if (!m_zipFile)
	{
		LERROR("ResourcePack: 无法打开资源包'%s' (unzOpen失败)", path);
		throw fcyException("ResourcePack::ResourcePack", "Can't open resource pack.");
	}
}

ResourcePack::~ResourcePack()
{
	unzClose(m_zipFile);
}

bool ResourcePack::LoadFile(const wchar_t* path, fcyRefPointer<fcyMemStream> & outBuf)LNOEXCEPT
{
	string tPathInUtf8;
	try
	{
		tPathInUtf8 = fcyStringHelper::WideCharToMultiByte(path, CP_UTF8);
		pathUniform(tPathInUtf8.begin(), tPathInUtf8.end());
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourcePack: 转换资源目录编码时无法分配内存");
		return false;
	}

	int tStatus = unzGoToFirstFile(m_zipFile);
	while (UNZ_OK == tStatus)
	{
		unz_file_info tFileInfo;
		char tZipName[MAX_PATH];

		if (UNZ_OK == unzGetCurrentFileInfo(m_zipFile, &tFileInfo, tZipName, sizeof(tZipName), nullptr, 0, nullptr, 0))
		{
			// 对路径做统一性转换
			pathUniform(tZipName, tZipName + MAX_PATH);

			// 检查路径是否命中
			if (strcmp(tPathInUtf8.c_str(), tZipName) == 0)
			{
#ifdef LSHOWRESLOADINFO
				LINFO("ResourcePack: 资源包'%s'命中文件'%s'", m_Path.c_str(), path);
#endif
				if (unzOpenCurrentFilePassword(m_zipFile, m_Password.length() > 0 ? m_Password.c_str() : nullptr) != UNZ_OK)
				{
					LERROR("ResourcePack: 尝试打开资源包'%s'中的文件'%s'失败(密码错误?)", m_Path.c_str(), path);
					return false;
				}

				try
				{
					outBuf.DirectSet(new fcyMemStream(NULL, tFileInfo.uncompressed_size, true, false));
				}
				catch (const bad_alloc&)
				{
					unzCloseCurrentFile(m_zipFile);
					LERROR("ResourcePack: 无法分配足够内存解压资源包'%s'中的文件'%s'", m_Path.c_str(), path);
					return false;
				}

				if (outBuf->GetLength() > 0)
				{
					if (unzReadCurrentFile(m_zipFile, outBuf->GetInternalBuffer(), tFileInfo.uncompressed_size) < 0)
					{
						unzCloseCurrentFile(m_zipFile);
						LERROR("ResourcePack: 解压资源包'%s'中的文件'%s'失败 (unzReadCurrentFile失败)", m_Path.c_str(), path);
						return false;
					}
				}
				unzCloseCurrentFile(m_zipFile);

				return true;
			}
		}
		else
			LWARNING("ResourcePack: 在资源包'%s'中寻找文件时发生错误 (unzGetCurrentFileInfo失败)", m_Path.c_str());

		tStatus = unzGoToNextFile(m_zipFile);
	}

	return false;
}

bool ResourcePack::FindFiles(lua_State * L, int* cnt, const char* path, const char* ext)LNOEXCEPT // t
{
	string tPathInUtf8 = path;
	string tPackInUtf8;
	try
	{
		//tPathInUtf8 = fcyStringHelper::WideCharToMultiByte(path, CP_UTF8);
		pathUniform(tPathInUtf8.begin(), tPathInUtf8.end());
		tPackInUtf8 = fcyStringHelper::WideCharToMultiByte(GetPathLowerCase().c_str(), CP_UTF8);
	}
	catch (const bad_alloc&)
	{
		LERROR("ResourcePack: 转换资源目录编码时无法分配内存");
		return false;
	}

	int extn = ext ? strlen(ext) : 0;

	int tStatus = unzGoToFirstFile(m_zipFile);
	while (UNZ_OK == tStatus)
	{
		unz_file_info tFileInfo;
		char tZipName[MAX_PATH];

		if (UNZ_OK == unzGetCurrentFileInfo(m_zipFile, &tFileInfo, tZipName, sizeof(tZipName), nullptr, 0, nullptr, 0))
		{
			// 对路径做统一性转换
			pathUniform(tZipName, tZipName + MAX_PATH);

			// 检查路径是否命中
			if (pathHit(tPathInUtf8.c_str(), tZipName))
			{
				int n = strlen(tZipName);
				bool flag = true;
				//检查拓展名是否命中
				if (ext) {
					int extn = strlen(ext);
					if (n > extn)
					{
						for (int i = 0; i < extn; i++) {
							if (tZipName[i + n - extn] != ext[i]) {
								flag = false;
							}
						}
					}
					else {
						flag = false;
					}
				}
				if (flag) {
					lua_newtable(L);// t t1
					lua_pushstring(L, tZipName);  // t t1 s
					lua_rawseti(L, -2, 1);  // t t1
					lua_pushstring(L, tPackInUtf8.c_str());  // t t1 s
					lua_rawseti(L, -2, 2);  // t t1
					lua_rawseti(L, -2, *cnt);  // t
					(*cnt) = (*cnt) + 1;
				}

			}
		}
		else
			LWARNING("ResourcePack: 在资源包'%s'中寻找文件时发生错误 (unzGetCurrentFileInfo失败)", m_Path.c_str());
		tStatus = unzGoToNextFile(m_zipFile);
	}

	return true;
}
