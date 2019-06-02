#pragma once
#include "E2DGlobal.hpp"
#include "fcyIO/fcyStream.h"

namespace Eyes2D {
	namespace IO {
		//压缩包
		class EYESDLLAPI Archive {
		public:
			struct ArchiveSort {
				int priority;//优先级，越大越靠前
				bool operator()(const Archive& l, const Archive& r) const {
					//优先级大的排在前面
					return l.sort.priority >= r.sort.priority;
				}
			};
		private:
			struct Impl;
			Impl* m_Impl;
			ArchiveSort sort;//优先级，越大越靠前
		private:
			//内部方法，加载压缩包
			void class_init(const char* path, const char* password = nullptr);
			//内部方法，卸载压缩包
			void class_del();
			//内部方法，检查文件是否存在并返回索引或者错误码
			long long file_precheck(const char* filepath);
		public:
			//连路径都莫得
			Archive();
			//只有路径
			Archive(const char* path);
			//路径+优先级
			Archive(const char* path, int priority);
			//路径+密码
			Archive(const char* path, const char* password);
			//路径+优先级+密码
			Archive(const char* path, int priority, const char* password);
			~Archive();
		public:
			//指定文件是否存在
			bool FileExist(const char* filepath);
			//加载文件到内存流，如果被加密了则使用默认密码，加载失败则返回nullptr
			fcyStream* LoadFile(const char* filepath);
			//加载加密文件到内存流，加载失败则使用默认密码，还加载失败则返回nullptr
			fcyStream* LoadEncryptedFile(const char* filepath, const char* password);
			//列出所有文件，输出到log
			void ListFile();
		};
		
		//文件读取、文件系统
		class EYESDLLAPI FileManager {
		private:
			struct Impl;
			Impl* m_Impl;
		public:
			FileManager();
			~FileManager();
		public:
			bool LoadArchive(const char* name);//加载压缩包，如果文件不存在、加载失败或者格式不支持则返回false
			void UnloadArchive(const char* name);//卸载压缩包
		};
	}
}
