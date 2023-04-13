# jwNet
一个跨平台事件网络库，支持epoll/iocp ipv4/ipv6。化繁为简，极度精简其接口。和著名的ASIO保持一样的Proactor设计风格，优雅大气，揣摩chriskohlhoff大师的设计理念。


```cpp
#include "Network.h"
using namespace net;

#define BUFFLEN 1024

class Connect : public IoSocket
{
protected:
	virtual void onConnect(int err) {}
	virtual void onWrite(int bytes, int err) {}

	virtual void onRead(int bytes, int err){
		if (bytes <= 0 || err) {
			return;
		}
		printf("recv msg: %s\n", readbuff);
		postRead();
	}

public:
	void postRead(){
		//Request to read data
		IoSocket::read(readbuff, BUFFLEN);
	}

private:
	char readbuff[BUFFLEN];
};

class IServer : public Accept
{
public:
	virtual void onAccept(IoSockptr& sptr, int err) {
		if (err){
			accept(sptr);
		}
		else {
			((Connect*)sptr.get())->postRead();
			accept(std::make_shared<Connect>());
		}
	}
};

int main()
{
	NetLoop loop;
	loop.init();

	IServer accept;
	accept.listen(&loop, "127.0.0.1", 6601);
	accept.accept(std::make_shared<Connect>());

	while (1) {
		loop.run(5);
	}
	return 0;
}
```

更多项目：[jwEngine](https://github.com/jwcpp/jwEngine) 
