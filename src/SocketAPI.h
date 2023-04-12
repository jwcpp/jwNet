#pragma once

#include "IPAddres.h"

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif


namespace net
{

#ifdef WIN_OS
	inline int blocking(int sockfd) {
		unsigned long nb = 0;
		return ioctlsocket(sockfd, FIONBIO, &nb);
	}
	inline int nonblocking(int sockfd) {
		unsigned long nb = 1;
		return ioctlsocket(sockfd, FIONBIO, &nb);
	}
	inline bool setnodelay(SOCKET s) {
		BOOL bTrue = TRUE;
		return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0; 
	}
	inline bool setreuse(SOCKET s) {
		BOOL bReUseAddr = TRUE;
		return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&bReUseAddr, sizeof(BOOL)) == 0;
	}
#else

#define closesocket(fd) close(fd)
#define blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) |  O_NONBLOCK)
	inline bool setnodelay(int fd) {
		int bTrue = true ? 1 : 0;
		return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&bTrue, sizeof(bTrue)) == 0;
	}
	inline bool setreuse(int fd) {
		int bReuse = 1;
		return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &bReuse, sizeof(bReuse)) == 0;
	}
#endif

	int bind(SOCKET fd, IPAddres const& addr);
	int listen(SOCKET fd, int backlog);
	SOCKET accept(SOCKET fd, IPAddres& addr);
	int connect(SOCKET fd, IPAddres addr);

}//namespace