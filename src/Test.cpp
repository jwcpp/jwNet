#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "Network.h"

using namespace net;
#define BUFFLEN 1024

NetLoop loop;

class MySocket : public IoSocket
{
public:
	MySocket()
	{
	}
	~MySocket()
	{
		printf("~MySocket()\n");
	}

protected:
	virtual void onConnect(int err) {
		if (!err)
		{
			printf("onConnect ok\n");

			static char buff[100] = "hello world!";
			//write(buff, strlen(buff) + 1);
			posRead();
		}
		else
		{
			printf("onConnect fail:%d\n", err);
			reconnect();
		}
	}

	virtual void onRead(int bytes, int err) {
		printf("onRead  ==> bytes:%d err:%d\n", bytes, err);
		if (bytes <= 0 || err) {
			printf("socket close\n");
			return;
		}

		printf("%s\n", _rbuff);
		posRead();

		//strcpy(_wbuff, _rbuff);
		//write(_wbuff, bytes);
	}
	virtual void onWrite(int bytes, int err) {
		printf("onWrite: bytes:%d, err:%d\n", bytes, err);
	}

public:
	void posRead()
	{
		read(_rbuff, BUFFLEN);
	}
private:
	char _rbuff[BUFFLEN];
	char _wbuff[BUFFLEN];
};

class MyAccept : public Accept
{
public:
	virtual void onAccept(IoSockptr& sioptr, int err) {
		printf("onAccept: errno:%d\n", err);
		if (err)
		{
			accept(sioptr);
		}
		else {
			
			((MySocket *)sioptr.get())->posRead();
			accept(std::make_shared<MySocket>());
		}
	}
};


void client()
{
	loop.init();

	{
		IoSockptr ptr = std::make_shared<MySocket>();
		ptr->connect(&loop, "192.168.198.128", 6601);
	}

	while (1)
	{
		loop.run(5);
	}
}

void server()
{
	loop.init();

	MyAccept accept;
	accept.listen(&loop, "192.168.1.3", 6601);
	accept.accept(std::make_shared<MySocket>());

	while (1)
	{
		loop.run(5);
	}
}

int main(int arg, char* argv[])
{
#ifdef _WIN32
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

	if (1)
	{
		server();
	}
	else
	{
		client();
	}
}