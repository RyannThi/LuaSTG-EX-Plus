#pragma once

#include <iostream>

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <queue>
#include <vector>
#include <string>

//取消windows.h自带的winsock1.1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "Ws2_32.lib")

//SB 微软
#ifdef ERROR
#undef ERROR
#endif // ERROR

//控制台log细节
enum class LOGLEVEL
{
	DEBUG   = 1,
	INFO    = 2,
	WARN    = 3,
	ERROR   = 4,
	DISABLE = 5,
};

//全局Log等级
static const LOGLEVEL G_LOGLEVEL = LOGLEVEL::DEBUG;

bool IFDEBUG() {
	return (G_LOGLEVEL <= LOGLEVEL::DEBUG);
}
bool IFINFO() {
	return (G_LOGLEVEL <= LOGLEVEL::INFO);
}
bool IFWARN() {
	return (G_LOGLEVEL <= LOGLEVEL::WARN);
}
bool IFERROR() {
	return (G_LOGLEVEL <= LOGLEVEL::ERROR);
}

//======================================
//EX+网络库基础

//读写锁
struct EX_RW_LOCK {
	volatile long mode;
	EX_RW_LOCK() {
		mode = 1;//0 write 1 none 2+ read
	}
	void ReadStart() {
		volatile long current_mode = mode;//Create A SnapShot of current mode
		while (current_mode == 0 || //In read mode
			InterlockedCompareExchange(&mode, current_mode + 1, current_mode) != current_mode) { //mode has changed by another thread
			Sleep(0);//Sleep a little time
			current_mode = mode;//Update snapshot
		}
	}
	void ReadEnd() {
#ifdef _DEBUG
		assert(InterlockedDecrement(&mode) != 0);
#else
		InterlockedDecrement(&mode);
#endif // _DEBUG
	}
	void WriteStart() {
		while (InterlockedCompareExchange(&mode, 0, 1) != 1)
		{
			Sleep(0);//Sleep a little time
		}
	}
	void WriteEnd() {
#ifdef _DEBUG
		assert(InterlockedIncrement(&mode) == 1);
#else
		InterlockedIncrement(&mode);
#endif // _DEBUG	
	}
};

//根据C++17重写的读写锁
struct ETC_RW_LOCK {
	mutable std::shared_mutex mutex_object;
	void ReadStart() {
		mutex_object.lock_shared();
	}
	void ReadEnd() {
		mutex_object.unlock_shared();
	}
	void WriteStart() {
		mutex_object.lock();
	}
	void WriteEnd() {
		mutex_object.unlock();
	}
};

//定长字符串缓冲区
//可回环式储存数据，当到达缓冲区尾部，则自动跳转到缓冲区头
//！警告，如果获取数据时数据长度过长，会获得重复的数据
class EXSTRINGBUFFER :public EX_RW_LOCK {
	volatile int buffer_head;
	volatile int buffer_tail;
	char *buffer;
	int size;
public:
	EXSTRINGBUFFER(int count) {
		size = count;
		buffer = new char[size + 1];
		buffer_head = 0;
		buffer_tail = 0;
	}
	~EXSTRINGBUFFER() {
		delete buffer;
	}
	int Push(const char *p) {
		while (*p) {
			buffer[buffer_head++] = *(p++);
			buffer_head = buffer_head % size;
			if (buffer_head == buffer_tail) {
				return 0;
			}
		}
	}
	int Get(char *out_data, int max_count)
	{
		int i = 0;
		WriteStart();
		if (buffer) {
			while (buffer[buffer_tail] && i < max_count && buffer_tail != buffer_head)
			{
				out_data[i] = buffer[buffer_tail];
				buffer_tail = (buffer_tail + 1) % size;
				i++;
			}
			if (i && !buffer[buffer_tail]) {
				buffer_tail = (buffer_tail + 1) % size;
			}
		}
		out_data[i] = 0;
		WriteEnd();
		return i;
	}
};

//动态字符串缓冲区，使用了新的读写锁
//使用string和queue重写
class StringBuffer :public ETC_RW_LOCK {
private:
	std::queue<std::string> queue_object;
public:
	StringBuffer() {
		Clear();
	}
	~StringBuffer() {
		Clear();
	}
	//向缓冲区尾推入一个字符串
	void Push(std::string str) {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		queue_object.push(str);
	}
	//获取最先放入的字符串，获取后从队列中pop掉
	std::string Get()
	{
		std::string str_out;
		{
			std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
			str_out = std::move(queue_object.front());//移动操作，节省资源，还能防止误操作（x
			queue_object.pop();
		}
		return str_out;
	}
	//清除字符串缓冲区
	void Clear() {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		while (!queue_object.empty()) {
			queue_object.pop();
		}
	}
	//是空的吗
	bool Empty() {
		std::shared_lock<std::shared_mutex> lock(mutex_object);//共享锁，自动销毁对象
		return queue_object.empty();
	}
};

//======================================
//EX+联机功能

//最大连接数
constexpr unsigned int MAX_TCP_CLIENT = 16;

//TCP状态
enum class TCPStatus {
	none    = 0,//初始状态
	free    = 1,//已设置地址和套接字
	install = 2,//已经连接上
	connect = 3,//收发连接状态
	close   = 4,//关闭
	invalid = 5,//出错，需要强行关闭套接字和收发线程
};

//WSA状态
static bool G_INIT_WSA = false;
static std::shared_mutex G_WSA_RW_LOCK;

//装载WSA
void InitWSA() {
	std::unique_lock<std::shared_mutex> lock(G_WSA_RW_LOCK);
	if (G_INIT_WSA) {
		return;
	}
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if ((iResult != 0) && IFERROR()) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return;
	}
	else if (IFINFO()) {
		std::cout << "WSAStartup successfully." << std::endl;
	}
	G_INIT_WSA = true;
}

//关闭WSA
void DelWSA() {
	std::unique_lock<std::shared_mutex> lock(G_WSA_RW_LOCK);
	if (!G_INIT_WSA) {
		return;
	}
	WSACleanup();
	G_INIT_WSA = false;
}

//获取WSA装载状态
bool IsWSAvalid() {
	std::shared_lock<std::shared_mutex> lock(G_WSA_RW_LOCK);
	return G_INIT_WSA;
}

//TCP组件
class TCPObject :public ETC_RW_LOCK {
public:
	TCPStatus m_status;
private:
	SOCKET socket_object;
	SOCKADDR_IN m_clientaddr;
	StringBuffer send_buffer_object;
	StringBuffer receive_buffer_object;
	std::thread send_thread_object;
	std::thread receive_thread_object;
	bool stop_send_signal;
	bool stop_recv_signal;
public:
	TCPObject() {
		InitWSA();//每一次启动都开一次WSA看看
		socket_object = INVALID_SOCKET;
		send_buffer_object.Clear();
		receive_buffer_object.Clear();
		stop_send_signal = false;
		stop_recv_signal = false;
		m_status = TCPStatus::none;
	};
	~TCPObject() {
		Stop();
	}
	
	//本体

	//直接将已有的套接字对象传入，进入install状态
	void Create(SOCKET s, SOCKADDR_IN s_in) {
		Stop();
		{
			std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
			socket_object = s;
			m_clientaddr = s_in;
			m_status = TCPStatus::install;
		}
		if (IFDEBUG()) {
			std::cout << "TCP free state." << std::endl;
		}
	};
	
	//连接到某个网络位置，进入install状态，失败则返回free状态
	bool Connect(std::string host, int port) {
		using std::cout;
		using std::endl;
		
		Stop();
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		bool finish = true;
		
		//create a tcp socket
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		socket_object = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
		if ((socket_object == INVALID_SOCKET) && IFERROR())
		{
			cout << "TCP create socket fail: " << WSAGetLastError() << endl;
			finish = false;
		}
		else if (IFINFO()) {
			cout << "TCP create socket successfully." << endl;
		}

		//connect to server
		m_clientaddr.sin_family = AF_INET;
		m_clientaddr.sin_port = htons(port);
		m_clientaddr.sin_addr.S_un.S_addr = inet_addr(host.data());
		if (IFINFO()) {
			cout << "TCP try to connect." << endl;
		}
		if ((connect(socket_object, (SOCKADDR *)&m_clientaddr, sizeof(m_clientaddr)) == SOCKET_ERROR) && IFERROR())
		{
			cout << "TCP connect fail: " << WSAGetLastError() << endl;
			finish = false;
		}
		if ((socket_object == INVALID_SOCKET) && IFERROR())
		{
			cout << "TCP connect fail: " << WSAGetLastError() << endl;
			finish = false;
		}

		//final
		if (finish) {
			m_status = TCPStatus::install;
			if (IFINFO()) {
				std::cout << "TCP connected successfully." << std::endl;
			}
		}
		else {
			m_status = TCPStatus::free;
			if (IFINFO()) {
				std::cout << "TCP connect fail." << std::endl;
			}
		}
		return finish;
	}
	
	//通过消息队列缓冲区来实现数据的发送
	void Send(std::string data) {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		if (IFDEBUG()) {
			std::cout << "TCP want to send: " << data << std::endl;
		}
		send_buffer_object.Push(data);
	}

	//通过消息队列缓冲区来实现数据的接收
	bool Receive(std::string &out_str) {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		bool result = !receive_buffer_object.Empty();
		if (result) {
			out_str = receive_buffer_object.Get();
			if (IFDEBUG()) {
				std::cout << "TCP want to get: " << out_str << std::endl;
			}
		}
		else {
			if (IFDEBUG()) {
				std::cout << "TCP receive queue is empty" << std::endl;
			}
		}
		return result;
	};
	
	//多线程部分

	//启动收发线程，从install状态转为connect状态
	//不在install状态则跳出
	void Start() {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		if (m_status != TCPStatus::install) {
			if (IFERROR()) {
				std::cout << "TCP status reqire install." << std::endl;
			}
			return;
		}
		stop_send_signal = false;
		stop_recv_signal = false;
		m_status = TCPStatus::connect;
		send_buffer_object.Clear();
		receive_buffer_object.Clear();
		//开始收发线程
		StartSendThread();
		StartReceiveThread();
	}

	//invalid状态时紧急关闭到stop状态
	//connect时正常关闭到stop状态
	//free时检查socket
	void Stop() {
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		send_buffer_object.Clear();
		receive_buffer_object.Clear();
		if (m_status != TCPStatus::close) {
			m_status = TCPStatus::close;
			stop_send_signal = true;
			stop_recv_signal = true;
			if (socket_object != INVALID_SOCKET) {
				
				closesocket(socket_object);
				socket_object = INVALID_SOCKET;
			}
		}
		/*
		std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
		if (socket_object) {
			closesocket(socket_object);
		}
		socket_object = INVALID_SOCKET;
		m_status = TCPStatus::free;
		*/
	};
	
	//发送线程，只能保持在connect状态，出错会转入invalid状态
	void SendThread() {
		using std::cout;
		using std::endl;
		using namespace std::chrono_literals;
		
		if (IFDEBUG()) {
			cout << "TCP send thread start." << endl;
		}

		//还未发出停止信号，发送消息循环
		while (true) {
			//检查发送条件
			{
				std::shared_lock<std::shared_mutex> lock(mutex_object);//共享锁，自动销毁对象
				if (stop_send_signal || (m_status != TCPStatus::connect)) {
					if (IFDEBUG()) {
						cout << "TCP send thread stop." << endl;
					}
					break;
				}
			}
			//发送
			if (!send_buffer_object.Empty()) {
				//连接状态且有数据可以发送
				std::string data = send_buffer_object.Get();
				int count;
				{
					std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
					count = send(socket_object, data.data(), data.length(), 0);
				}
				if (count == SOCKET_ERROR) {
					//出错，关闭
					{
						std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
						if (IFERROR()) {
							cout << "TCP send failed: " << WSAGetLastError() << endl;
						}
						m_status = TCPStatus::invalid;
					}
					Stop();
					break;
				}
				else if (IFDEBUG()) {
					cout << "TCP send: " << count << "->" << data << endl;
				}
			}
			else {
				//无数据可发送，休眠
				std::this_thread::sleep_for(1ms);
			}
		}
	}
	void StartSendThread() {
		std::thread td(this->ReceiveThread);
		std::swap(td, send_thread_object);
	}
	
	//接收线程，只能保持在connect状态，出错会转为invalid状态
	void ReceiveThread() {
		using std::cout;
		using std::endl;
		
		if (IFDEBUG()) {
			cout << "TCP recieve thread start." << endl;
		}

		const unsigned int BUFFER_LENGTH = 1024;
		char *temp_buffer = new char[BUFFER_LENGTH];
		//数据接收循环
		while (true)
		{
			//检查接收条件
			{
				std::shared_lock<std::shared_mutex> lock(mutex_object);//共享锁，自动销毁对象
				if (stop_recv_signal || (m_status != TCPStatus::connect)) {
					if (IFDEBUG()) {
						cout << "TCP recieve thread stop." << endl;
					}
					break;
				}
			}
			memset(temp_buffer, 0, BUFFER_LENGTH);
			int count;
			{
				std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
				count = recv(socket_object, temp_buffer, BUFFER_LENGTH, 0);
			}
			if (count <= 0)
			{
				//出错，关闭
				{
					std::unique_lock<std::shared_mutex> lock(mutex_object);//独占互斥锁，自动销毁对象
					if (IFERROR()) {
						cout << "TCP receive failed: " << WSAGetLastError() << endl;
					}
					m_status = TCPStatus::invalid;
				}
				Stop();
				break;
			}
			else {
				//收到消息，存到字符串缓冲区中
				if (IFDEBUG()) {
					cout << "TCP receive: " << count << "<-" << temp_buffer << endl;
				}
				std::string data = temp_buffer;
				receive_buffer_object.Push(data);
			}
		}
		delete[] temp_buffer;
	}
	void StartReceiveThread() {
		std::thread td(this->ReceiveThread);
		std::swap(td, receive_thread_object);
	}
};

//EX+联机服务器
class TCPGameServer {
private:
	SOCKET serversocket_object;
	std::vector<TCPObject> client_objects;
public:
	TCPGameServer();
	~TCPGameServer();

	//基于本地的IP地址，所以只用考虑端口即可
	bool Start(int port);

	//停止服务器
	void Stop();

	static void static_server_thread(void *p);
	void server_thread();

	static void static_server_boardcast_thread(void *p);
	void server_boardcast_thread();

	void Send(const char *data, int count);

};
