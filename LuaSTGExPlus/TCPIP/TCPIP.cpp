// TCPIP.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <assert.h>
#pragma comment(lib,"ws2_32.lib")
void cmain(void *p);
void smain(void *p);
volatile long finish = 0;
void test();
int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(1, 1), &wsa);	//initial Ws2_32.dll by a process
	test();
	WSACleanup();
	return 0;
}

//TCP server
//listen port 9102
//receive string and display it

//Visual C++ 6.0





#define BUFLEN 1024


void smain(void *p)
{
	SOCKET serversoc;
	SOCKET clientsoc;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	char buf[BUFLEN];
	int len;

	
	if ((serversoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)	//create a tcp socket
	{
		printf("Create socket fail!\n");
		//return -1;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9102);
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(serversoc, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) != 0)
	{
		printf("Bind fail!\n");
		//return -1;
	}

	//start listen, maximum length of the queue of pending connections is 1
	printf("Start listen...\n");
	if (listen(serversoc, 1) != 0)
	{
		printf("Listen fail!\n");
		//return -1;
	}

	len = sizeof(SOCKADDR_IN);


	//waiting for connecting
	if ((clientsoc = accept(serversoc, (SOCKADDR *)&clientaddr, &len)) <= 0)
	{
		printf("Accept fail!\n");
		//return -1;
	}
	printf("Connected\n");
	while (1)
	{
		//waiting for data receive
		if (recv(clientsoc, buf, BUFLEN, 0) <= 0)
		{
			//some error occur
			printf("Close connection\n");
			closesocket(clientsoc);
			break;
		}
		printf("%s\n", buf);
	}
	InterlockedIncrement(&finish);
	//WSACleanup(); //clean up Ws2_32.dll 
	//return 0;
}


void cmain(void *p)
{
	SOCKET soc;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	char buf[1024];

	//WSADATA wsa;
	//WSAStartup(MAKEWORD(1, 1), &wsa);	//initial Ws2_32.dll by a process

	if ((soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)	//create a tcp socket
	{
		printf("Create socket fail!\n");
		//return -1;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9102);
	serveraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	//connect to server
	printf("Try to connect...\n");
	if (connect(soc, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) != 0)
	{
		printf("Connect fail!\n");
		//return -1;
	}
	printf("Connected\n");
	while (1)
	{
		char buf[1024];
		scanf_s("%s", buf, BUFLEN);
		//send to server
		if (send(soc, buf, strlen(buf) + 1, 0) <= 0)
		{
			printf("Error!\n");
		}
	}

	finish++;
	//WSACleanup(); //clean up Ws2_32.dll 
	//return 0;
}

struct EX_RW_LOCK{
	volatile long mode;
	EX_RW_LOCK(){
		mode = 1;//0 write 1 none 2+ read
	}
	void ReadStart(){
		volatile long current_mode = mode;//Create A SnapShot of current mode
		while (current_mode == 0 || //In read mode
			InterlockedCompareExchange(&mode, current_mode + 1, current_mode) != current_mode){ //mode has changed by another thread
			Sleep(0);//Sleep a little time
			current_mode = mode;//Update snapshot
		}
	}
	void ReadEnd(){
#ifdef _DEBUG
		assert(InterlockedDecrement(&mode) != 0);
#else
		InterlockedDecrement(&mode);
#endif // _DEBUG
	}
	void WriteStart(){
		while (InterlockedCompareExchange(&mode, 0, 1) != 1)
		{ 
			Sleep(0);//Sleep a little time
		}
	}
	void WriteEnd(){
#ifdef _DEBUG
		assert(InterlockedIncrement(&mode) == 1);
#else
		InterlockedIncrement(&mode);
#endif // _DEBUG	
	}
};



#define MAX_TCPIP_CLIENT 16
#define LOCAL_BUFFER_LENGTH 2048
#define EXCI_FREE 0
#define EXCI_CONNECTING 1
#define EXCI_CONNECTED 2
#define EXCI_DISCONNECTED 3
#define EXCI_FULL 4
#define EXCI_CLOSE 5

int EXTCPDEBUG = 1;


class EXSTRINGBUFFER:public EX_RW_LOCK{
	volatile int buffer_head;
	volatile int buffer_tail;
	char *buffer;
	int size;
public:
	EXSTRINGBUFFER(int count){
		size = count;
		buffer = new char[size + 1];
		buffer_head = 0;
		buffer_tail = 0;
	}
	~EXSTRINGBUFFER(){
		delete buffer;
	}
	int Push(const char *p){
		while (*p){
			buffer[buffer_head++] = *(p++);
			buffer_head = buffer_head%size;
			if (buffer_head == buffer_tail){
				return 0;
			}
		}
	}
	int Get(char *out_data, int max_count)
	{
		int i = 0;
		WriteStart();
		if (buffer){
			while (buffer[buffer_tail] && i < max_count && buffer_tail != buffer_head)
			{
				out_data[i] = buffer[buffer_tail];
				buffer_tail = (buffer_tail + 1) % size;
				i++;
			}
			if (i && !buffer[buffer_tail]){
				buffer_tail = (buffer_tail + 1) % size;
			}
		}
		out_data[i] = 0;
		WriteEnd();
		return i;
	}
};

class EXTCPIPSERVERCLIENTINFO:public EX_RW_LOCK{
public:
	EXTCPIPSERVERCLIENTINFO();
	~EXTCPIPSERVERCLIENTINFO();
	volatile int status;
	volatile int buffer_head;
	volatile int buffer_tail;
	char *buffer;
	SOCKET clientsocket;
	SOCKADDR_IN clientaddr;
	void Create(SOCKET s,SOCKADDR_IN s1);
	bool Connect(const char *host,int port);

	void clean_up();

	void Stop();

	bool Send(const char *data, int count);

	int Receive(char *out_data,int max_count);

	static void _server_listen_thread(void *p);
	void server_listen_thread();

};

EXTCPIPSERVERCLIENTINFO::EXTCPIPSERVERCLIENTINFO()
{
	buffer = NULL;
	status = EXCI_FREE;
	clientsocket = NULL;
}

EXTCPIPSERVERCLIENTINFO::~EXTCPIPSERVERCLIENTINFO()
{
	Stop();
}

void EXTCPIPSERVERCLIENTINFO::Create(SOCKET s, SOCKADDR_IN s1)
{
	WriteStart();
	clean_up();
	buffer_head = 0;
	buffer_tail = 0;
	buffer = new char[LOCAL_BUFFER_LENGTH + 1];
	status = EXCI_CONNECTED;
	clientsocket = s;
	clientaddr = s1;
	if (EXTCPDEBUG)printf("SV_CL_Start\n");
	_beginthread(_server_listen_thread, 0, this);
	WriteEnd();
}

bool EXTCPIPSERVERCLIENTINFO::Connect(const char *host, int port)
{
	WriteStart();
	clean_up();
	buffer_head = 0;
	buffer_tail = 0;
	buffer = new char[LOCAL_BUFFER_LENGTH + 1];
	if ((clientsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)	//create a tcp socket
	{
		printf("Create socket fail!\n");
		//return -1;
	}
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(port);
	clientaddr.sin_addr.S_un.S_addr = inet_addr(host);

	//connect to server
	printf("Try to connect...\n");
	if (connect(clientsocket, (SOCKADDR *)&clientaddr, sizeof(clientaddr)) != 0)
	{
		printf("Connect fail!\n");
		return false;
	}
	if (EXTCPDEBUG)printf("SV_CL_Start\n");
	status = EXCI_CONNECTED;
	_beginthread(_server_listen_thread, 0, this);
	WriteEnd();
	return true;
}

void EXTCPIPSERVERCLIENTINFO::clean_up()
{
	if (buffer){
		delete[] buffer;
		buffer = NULL;
	}
	if (clientsocket){
		closesocket(clientsocket);
		clientsocket = NULL;
	}
}

void EXTCPIPSERVERCLIENTINFO::Stop()
{
	bool waitflag = false;
	WriteStart();
	clean_up();
	if (status == EXCI_CONNECTED || status == EXCI_FULL){
		status = EXCI_CLOSE;
		waitflag = true;

	}
	WriteEnd();
	if (waitflag){
		while (status != EXCI_DISCONNECTED){
			Sleep(0);
		}
	}
}

bool EXTCPIPSERVERCLIENTINFO::Send(const char *data, int count)
{
	bool rt = false;
	WriteStart();
	if (status == EXCI_CONNECTED || status == EXCI_FULL){
		//if (EXTCPDEBUG)printf("SV_CL_Send:%s\n", data);
		int t = send(clientsocket, data, count + 1, 0);
		rt = (t == count + 1);
	}
	WriteEnd();
	return rt;
}

int EXTCPIPSERVERCLIENTINFO::Receive(char *out_data, int max_count)
{
	int i = 0;
	WriteStart();
	if (buffer && (status == EXCI_CONNECTED || status == EXCI_FULL)){
		while (buffer[buffer_tail] && i < max_count && buffer_tail != buffer_head)
		{
			out_data[i] = buffer[buffer_tail];
			buffer_tail = (buffer_tail + 1) % LOCAL_BUFFER_LENGTH;
			i++;
		}
		if (i && !buffer[buffer_tail]){
			buffer_tail = (buffer_tail + 1) % LOCAL_BUFFER_LENGTH;
		}
		if (status == EXCI_FULL)status = EXCI_CONNECTED;
	}
	out_data[i] = 0;
	if (EXTCPDEBUG)if (i)printf("SV_CL_Get:%s\n", out_data);
	WriteEnd();
	return i;
}

void EXTCPIPSERVERCLIENTINFO::_server_listen_thread(void *p)
{
	EXTCPIPSERVERCLIENTINFO *self = (EXTCPIPSERVERCLIENTINFO *)p;
	self->server_listen_thread();

	_endthread();
}

void EXTCPIPSERVERCLIENTINFO::server_listen_thread()
{
	char *temp_buffer = new char[LOCAL_BUFFER_LENGTH + 1];
	while (1)
	{
		//waiting for data receive
		int count = recv(clientsocket, temp_buffer, LOCAL_BUFFER_LENGTH, 0);
		WriteStart();
		if (count <= 0)
		{
			if (EXTCPDEBUG)printf("SV_CL_Failed\n");
			clean_up();
			delete[] temp_buffer;
			status = EXCI_DISCONNECTED;
			WriteEnd();
			return;
		}
		else{
			if (EXTCPDEBUG)printf("SV_CL_Receive:%d,%s\n", count,temp_buffer);
			//insert content into buffer
			int i = 0;
			while (count > 0){
				buffer[buffer_head] = temp_buffer[i++];
				buffer_head = (buffer_head + 1) % LOCAL_BUFFER_LENGTH;
				if (buffer_head == buffer_tail){
					//buffer is full,wait for possible get_buffer
					buffer_head--;
					status = EXCI_FULL;
					if (EXTCPDEBUG)printf("SV_CL_Full\n");
					WriteEnd();
					while (status == EXCI_FULL)
					{
						Sleep(0);
					}
					WriteStart();
					if (status != EXCI_CONNECTED){
						if (EXTCPDEBUG)printf("SV_CL_Failed\n");
						clean_up();
						delete[] temp_buffer;
						status = EXCI_DISCONNECTED;
						WriteEnd();
						return;
					}
					buffer_head++;
				}
				count--;
			}
		}
		WriteEnd();
		//printf("%s\n", buf);
	}
}

class EXTCPIPSERVER{
private:
	int status;
	volatile bool stop;
	volatile bool stop_boardcast;
	EXTCPIPSERVERCLIENTINFO m_clients[MAX_TCPIP_CLIENT];
	SOCKET serversocket;
public:
	EXTCPIPSERVER();
	~EXTCPIPSERVER();
	bool Start(int port);
	void Stop();

	static void static_server_thread(void *p);
	void server_thread();

	static void static_server_boardcast_thread(void *p);
	void server_boardcast_thread();

	void Send(const char *data, int count);

};

EXTCPIPSERVER::EXTCPIPSERVER()
{
	status = EXCI_FREE;
	stop = false;
	stop_boardcast = false;
	serversocket = NULL;
}

EXTCPIPSERVER::~EXTCPIPSERVER()
{
	Stop();
}

bool EXTCPIPSERVER::Start(int port)
{
	Stop();
	if ((serversocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)	//create a tcp socket
	{
		printf("Create socket fail!\n");
		//return -1;
	}
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(serversocket, (SOCKADDR *)&serveraddr, sizeof(serveraddr)) != 0)
	{
		printf("Bind fail!\n");
		closesocket(serversocket);
		serversocket = NULL;
		return false;
	}
	status = EXCI_CONNECTED;
	_beginthread(static_server_thread, 0, this);
	_beginthread(static_server_boardcast_thread, 0, this);
	return true;
}

void EXTCPIPSERVER::Stop()
{
	if (serversocket){
		stop_boardcast = true;
		while (stop_boardcast){
			Sleep(0);
		}
		stop = true;
		closesocket(serversocket);
		while (stop){
			Sleep(0);
		}
		for (int i = 0; i < MAX_TCPIP_CLIENT; i++){
			m_clients[i].Stop();
		}
		if (EXTCPDEBUG)printf("SV_Stop\n");
		serversocket = NULL;
	}
}

void EXTCPIPSERVER::static_server_thread(void *p)
{
	EXTCPIPSERVER *self = (EXTCPIPSERVER *)p;
	self->server_thread();
	_endthread();
}

void EXTCPIPSERVER::server_thread()
{
	if (listen(serversocket, 5) != 0)
	{
		printf("Listen fail!\n");
		//return -1;
	}
	while (!stop){
		SOCKET temp_client = NULL;
		SOCKADDR_IN temp_addr;
		int len = sizeof(SOCKADDR_IN);
		if ((int)(temp_client = accept(serversocket, (SOCKADDR *)&temp_addr, &len)) <= 0)
		{
			if (EXTCPDEBUG)printf("SV_Failed\n");
		}
		else{
			if (EXTCPDEBUG)printf("SV_Connected\n");
			//alloc a new client position
			for (int i = 0; i < MAX_TCPIP_CLIENT; i++){
				if (m_clients[i].status == EXCI_FREE){
					m_clients[i].Create(temp_client, temp_addr);
					char hellos[10];
					hellos[0] = 'U';
					hellos[1] = 'S';
					hellos[2] = '1' + i;
					hellos[3] = 0;
					bool rt=m_clients[i].Send(hellos, 3);
					break;
				}
			}
		}
		Sleep(0);
	}
	stop = false;
}

void EXTCPIPSERVER::static_server_boardcast_thread(void *p)
{
	EXTCPIPSERVER *self = (EXTCPIPSERVER *)p;
	self->server_boardcast_thread();
	_endthread();
}

void EXTCPIPSERVER::server_boardcast_thread()
{
	char *temp_buffer = new char[LOCAL_BUFFER_LENGTH];
	while (!stop_boardcast){
		for (int i = 0; i < MAX_TCPIP_CLIENT; i++){
			int t;
			if (m_clients[i].status == EXCI_DISCONNECTED){
				m_clients[i].status = EXCI_FREE;
			}
			if ((t = m_clients[i].Receive(temp_buffer, LOCAL_BUFFER_LENGTH - 1))){
				if (temp_buffer[0] != 'K'){
					printf_s("%d : %s\n", i + 1, temp_buffer,t);
				}
				for (int j = 0; j < MAX_TCPIP_CLIENT; j++){
					if (j != i){
						m_clients[j].Send(temp_buffer, t);
					}
				}
			}
		}
		Sleep(1);
	}
	stop_boardcast = false;
}

void EXTCPIPSERVER::Send(const char *data, int count)
{
	if (serversocket){
		for (int i = 0; i < MAX_TCPIP_CLIENT; i++){
			m_clients[i].Send(data, count);
		}
	}
}

void test2(){
	EXTCPIPSERVER *server = new EXTCPIPSERVER;
	EXTCPIPSERVERCLIENTINFO *a = new EXTCPIPSERVERCLIENTINFO;
	EXTCPIPSERVERCLIENTINFO *b = new EXTCPIPSERVERCLIENTINFO;
	server->Start(9012);
	a->Connect("127.0.0.1", 9012);
	b->Connect("127.0.0.1", 9012);
	a->Send("hello", 5);
	char buf[10];
	Sleep(10000);
	b->Receive(buf, 9);
	delete server;
	Sleep(1000);
	delete a;
	delete b;
}

void luaserv(){
	int c;
	printf("Port:\n");
	scanf_s("%d", &c);
	EXTCPIPSERVER *server = new EXTCPIPSERVER;
	server->Start(c);
	printf("Started at port %d . Enter s to exit.\n",c);
	while (true){
		c = getchar();
		if(c == 's' || c == 'S')break;
	}
	delete server;
}


void test(){
	luaserv();
	return;
	int c = getchar();
	if (c == '0'){
		EXTCPIPSERVER *server = new EXTCPIPSERVER;
		server->Start(26033);
		c = getchar();
		c = getchar();
		c = getchar();
	}
	else{
		EXTCPIPSERVERCLIENTINFO *a = new EXTCPIPSERVERCLIENTINFO;
		a->Connect("127.0.0.1", 26033);
		while (true){
			char buf[1024];
			scanf_s("%s", buf, 1024);
			if (a->Send(buf, strlen(buf)) == 0)break;
		}
		delete a;
	}
	printf("Finished");
	c = getchar();
}