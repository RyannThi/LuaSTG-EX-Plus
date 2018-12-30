#pragma once
#include <cstring>

namespace LuaSTGPlus
{
	//使用最小完美哈希算法进行字符串和枚举值配对
	//该代码由string2enum.py+GameObjectPropertyHash.json生成，源代码by 9CHU，适配python3 by Xiliusha
	//重要提示：只看这里的代码来研究其工作原理是没有用的

	enum class GameObjectProperty
	{
		X = 0,
		Y = 1,
		DX = 2,
		DY = 3,
		ROT = 4,
		OMIGA = 5,
		TIMER = 6,
		VX = 7,
		VY = 8,
		MAXV = 9,
		MAXVX = 10,
		MAXVY = 11,
		AX = 12,
		AY = 13,
		AG = 14,
		LAYER = 15,
		GROUP = 16,
		HIDE = 17,
		BOUND = 18,
		NAVI = 19,
		COLLI = 20,
		STATUS = 21,
		HSCALE = 22,
		VSCALE = 23,
		CLASS = 24,
		A = 25,
		B = 26,
		RECT = 27,
		IMG = 28,
		ANI = 29,
		PAUSE = 30,
		RESOLVEMOVE = 31,
		IGNORESUPERPAUSE = 32,
		VANGLE = 33,
		VSPEED = 34,
		WORLD = 35,
		_KEY_NOT_FOUND = -1
	};

	inline GameObjectProperty GameObjectPropertyHash(const char* key)
	{
		static const char* s_orgKeyList[] =
		{
			"x",
			"y",
			"dx",
			"dy",
			"rot",
			"omiga",
			"timer",
			"vx",
			"vy",
			"maxv",
			"maxvx",
			"maxvy",
			"ax",
			"ay",
			"ag",
			"layer",
			"group",
			"hide",
			"bound",
			"navi",
			"colli",
			"status",
			"hscale",
			"vscale",
			"class",
			"a",
			"b",
			"rect",
			"img",
			"ani",
			"pause",
			"rmove",
			"nopause",
			"_angle",
			"_speed",
			"world",
		};

		static const unsigned int s_bestIndices[] =
		{
			0, 1, 4,
		};

		static const unsigned int s_hashTable1[] =
		{
			199, 14, 137,
		};

		static const unsigned int s_hashTable2[] =
		{
			234, 67, 180,
		};

		static const unsigned int s_hashTableG[] =
		{
			0, 3, 11, 0, 9, 14, 35, 6, 0, 0,
			0, 0, 0, 25, 0, 0, 21, 15, 28, 0,
			0, 0, 0, 0, 19, 8, 0, 0, 22, 2,
			34, 7, 15, 18, 24, 0, 3, 8, 0, 9,
			11, 28, 25, 0, 0, 31, 7, 0, 4, 5,
			4, 0, 7, 31, 0, 0, 3, 0, 12,
		};

		unsigned int f1 = 0, f2 = 0, len = strlen(key);
		for (unsigned int i = 0; i < 3; ++i)
		{
			unsigned int idx = s_bestIndices[i];
			if (idx < len)
			{
				f1 = (f1 + s_hashTable1[i] * (unsigned int)key[idx]) % 59;
				f2 = (f2 + s_hashTable2[i] * (unsigned int)key[idx]) % 59;
			}
			else
				break;
		}

		unsigned int hash = (s_hashTableG[f1] + s_hashTableG[f2]) % 36;
		if (strcmp(s_orgKeyList[hash], key) == 0)
			return static_cast<GameObjectProperty>(hash);
		return GameObjectProperty::_KEY_NOT_FOUND;
	}
}

namespace LuaSTGPlusESC
{
	//ESC写的hash配对算法，由于其实现限制，支持的属性数量到33以上后就出现重复值了
	//该实现现已搁置，等待优化
	
	//属性数量
	static const char s_propertyCount = 32;

	//属性枚举
	enum class GameObjectProperty
	{
		X = 0,
		Y = 1,
		DX = 2,
		DY = 3,
		ROT = 4,
		OMIGA = 5,
		TIMER = 6,
		VX = 7,
		VY = 8,
		AX = 9,
		AY = 10,
		LAYER = 11,
		GROUP = 12,
		HIDE = 13,
		BOUND = 14,
		NAVI = 15,
		COLLI = 16,
		STATUS = 17,
		HSCALE = 18,
		VSCALE = 19,
		CLASS = 20,
		A = 21,
		B = 22,
		RECT = 23,
		IMG = 24,
		ANI = 25,
		PAUSE = 26,
		RESOLVEMOVE = 27,
		IGNORESUPERPAUSE = 28,
		VANGLE = 29,
		VSPEED = 30,
		WORLD = 31,
		_KEY_NOT_FOUND = -1
	};
	
	//对应的字符串
	static const char* s_orgKeyList[] = {
		"x",
		"y",
		"dx",
		"dy",
		"rot",
		"omiga",
		"timer",
		"vx",
		"vy",
		"ax",
		"ay",
		"layer",
		"group",
		"hide",
		"bound",
		"navi",
		"colli",
		"status",
		"hscale",
		"vscale",
		"class",
		"a",
		"b",
		"rect",
		"img",
		"ani",
		"pause",
		"rmove",
		"nopause",
		"_angle",
		"_speed",
		"world"
	};
	
	//初始化hash表
	static const int HLEN = 173;
	static char g_FullHashIndex[65536] = { 0 };
	bool _init_hash_table(){
		for (int i = 0; i < s_propertyCount; i++){
			g_FullHashIndex[((const unsigned short *)(s_orgKeyList[i]))[0] % HLEN] = i + 1;
		}
		return true;
	}
	bool g_HashInit = _init_hash_table();

	//从字符串到枚举
	inline GameObjectProperty GameObjectPropertyHash(const char* key){
		char hash = g_FullHashIndex[((const unsigned short *)(key))[0] % HLEN] - 1;
		if (hash >= 0 && (strcmp(s_orgKeyList[hash], key) == 0))
			return static_cast<GameObjectProperty>(hash);
		return GameObjectProperty::_KEY_NOT_FOUND;
	}
	inline GameObjectProperty GameObjectPropertyHashOld(const char* key)
	{
		
		
		static const unsigned int s_bestIndices[] =
		{
			0, 1, 
		};
		
		static const unsigned int s_hashTable1[] =
		{
			191, 127, 
		};
		
		static const unsigned int s_hashTable2[] =
		{
			239, 14, 
		};
		
		static const unsigned int s_hashTableG[] =
		{
			0, 0, 0, 0, 15, 0, 0, 0, 0, 16, 
			13, 0, 19, 0, 9, 0, 12, 0, 0, 5, 
			4, 22, 8, 23, 13, 16, 22, 0, 0, 15, 
			22, 20, 8, 14, 19, 0, 14, 21, 6, 0, 
			20, 17, 0, 
		};
		
		unsigned int f1 = 0, f2 = 0, len = strlen(key);
		for (unsigned int i = 0; i < 2; ++i)
		{
			unsigned int idx = s_bestIndices[i];
			if (idx < len)
			{
				f1 = (f1 + s_hashTable1[i] * (unsigned int)key[idx]) % 43;
				f2 = (f2 + s_hashTable2[i] * (unsigned int)key[idx]) % 43;
			}
			else
				break;
		}
		
		unsigned int hash = (s_hashTableG[f1] + s_hashTableG[f2]) % 26;
		if (strcmp(s_orgKeyList[hash], key) == 0)
			return static_cast<GameObjectProperty>(hash);
		return GameObjectProperty::_KEY_NOT_FOUND;
	}
}
