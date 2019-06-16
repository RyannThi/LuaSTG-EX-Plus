#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include "E2DMainFunction.hpp"
#include "E2DLogSystem.hpp"

#define E2DMAINRETURN std::system("pause"); return 0;
#define SAY(W) std::cout<<(W)<<std::endl;

static const char FORMAT_REPLAY_LREP_CHUNK[8] = { 'R','E','P','L', 'A', 'Y', 0, 0 };
static const char FORMAT_REPLAY_INFO_CHUNK[8] = { 'I','N','F','O',   0,   0, 0, 0 };
static const char FORMAT_REPLAY_DATA_CHUNK[8] = { 'D','A','T','A',   0,   0, 0, 0 };

//Replay每帧的输入记录，大小16byte
struct ReplayRecord {
	//按键部分
	union {
		union {
			struct {
				bool left : 1;
				bool right : 1;
				bool up : 1;
				bool down : 1;
				bool slow : 1;
				bool shoot : 1;
				bool spell : 1;
				bool special : 1;
				bool skip : 1;
				bool lmouse : 1;
				bool mmouse : 1;
				bool rmouse : 1;
				bool _1_ : 4;
				bool _2_ : 8;
				bool _3_ : 8;
			};
			uint32_t key;
		};
		uint32_t _1;
	};
	//鼠标滚轮和手柄扳机
	union {
		struct {
			int16_t wheel;
			uint8_t ltrigger;
			uint8_t rtrigger;
		};
		uint32_t _2;
	};
	//手柄轴
	union {
		struct {
			int16_t axisx;
			int16_t axisy;
		};
		uint32_t _3;
	};
	//鼠标坐标 32bit
	union {
		struct {
			int16_t mousex;
			int16_t mousey;
		};
		uint32_t _4;
	};
};

//验证ReplayRecord大小为16byte，出错则编译不通过
static_assert(sizeof(ReplayRecord) == 16U, "ReplayRecord size error.");

//Replay文件数据块类型
enum class ReplayChunkType {
	REPLAY = 1,
	INFO   = 2,
	DATA   = 3,
};

//Replay文件数据块
struct ReplayChunk{
	ReplayChunkType type;//chun type
	std::string data;//string data from lua
	std::vector<ReplayRecord> record;//replay frame record
};

//Replay记录
class ReplayWriter {
private:
	//std::vector<ReplayChunk> m_chunk;
	ReplayChunk m_chunk;
public:
	ReplayWriter() {
		m_chunk.data.clear();
		m_chunk.record.clear();
		m_chunk.type = ReplayChunkType::DATA;
	}
public:
	uint32_t WriteToFile(std::fstream& stream) {
		if (!stream.is_open()) { return 0; }
		uint32_t write = 0;
		stream.write(FORMAT_REPLAY_DATA_CHUNK, 8);
		write += 8;
		{
			uint32_t size = m_chunk.data.size();
			stream.write((char*)(&size), sizeof(uint32_t));
			stream.write(m_chunk.data.data(), size);
			write += size;
		}
		{
			uint32_t usize = sizeof(ReplayRecord);
			uint32_t size = m_chunk.record.size() * usize;
			stream.write((char*)(&size), sizeof(uint32_t));
			ReplayRecord rec;
			for (auto& r : m_chunk.record) {
				rec = r;
				stream.write((char*)(&rec), usize);
			}
			write += size;
		}
		return write;
	}
	void SetChunkInfo(const char* info) { m_chunk.data = info; }
	void SetChunkInfo(std::string info) { m_chunk.data = info; }
	void Record(ReplayRecord rec) { m_chunk.record.push_back(rec); }
	void UndoRecord(uint32_t count = 1) {
		for (uint32_t c = 0; c < (std::min)(count, m_chunk.record.size()); c++)
			m_chunk.record.pop_back();
	}
	void ClearRecord() { m_chunk.record.clear(); }
};

//Replay读取
class ReplayReader {

};




using namespace std;
using namespace Eyes2D;

E2DMAIN {
fstream file;
file.open("replay.bin", ios::out | ios::binary | ios::trunc);
if (file.is_open()) {
	file.clear();
	file.open("replay.bin", ios::out);
	file.close();
	file.open("replay.bin", ios::out | ios::binary | ios::trunc);
}
ReplayRecord rec{ 0 };
memset(&rec, 0, sizeof(ReplayRecord));
rec.mousex = 9961;
rec.mousey = 2333;
file.write((const char*)&rec, sizeof(ReplayRecord));
file.flush();
file.close();
E2DMAINRETURN }




