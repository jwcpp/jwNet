#include "Accept.h"
#include "../NetDef.h"
#include <assert.h>

namespace net {
	Accept::Accept() :
		_loop(NULL),
		_listenfd(INVALID_SOCKET),
		_socket(INVALID_SOCKET),
		_isipv6(false)
	{
		_acceptHandle._type = IocpHandle::eIH_ACCPET;
		memset(&_acceptHandle._overlapped, 0, sizeof(OVERLAPPED));
	}

	Accept::~Accept()
	{

	}


	bool Accept::listen(NetLoop * loop, const char* ip, unsigned short port)
	{
		IPAddres addr(ip, port);
		SOCKET listenfd = WSASocket(addr.isIpv6() ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (listenfd == INVALID_SOCKET)
		{
			WRITE_LOG("WSASocket() error! ");
			return false;
		}

		setreuse(listenfd);
		if (net::bind(listenfd, addr) != 0)
		{
			WRITE_LOG("bind() error! ");
			close();
			return false;
		}
		
		{//环回快速路径
			if (true)
			{
				int OptionValue = 1;
				DWORD NumberOfBytesReturned = 0;
				DWORD SIO_LOOPBACK_FAST_PATH_A = 0x98000010;

				WSAIoctl(
					listenfd,
					SIO_LOOPBACK_FAST_PATH_A,
					&OptionValue,
					sizeof(OptionValue),
					NULL,
					0,
					&NumberOfBytesReturned,
					0,
					0);
			}
		}

		if (net::listen(listenfd, SOMAXCONN) != 0)
		{
			WRITE_LOG("listen() error! errno:%d\n", WSAGetLastError());
			close();
			return false;
		}

		ULONG_PTR key = 0;
		if (CreateIoCompletionPort((HANDLE)listenfd, loop->io(), key, 1) == NULL)
		{
			WRITE_LOG("CreateIoCompletionPort() error! errno:%d\n", WSAGetLastError());
			close();
			return false;
		}

		_loop = loop;
		_isipv6 = addr.isIpv6();
		_listenfd = listenfd;

		//funcptr
		_acceptex = getAcceptEx(listenfd);
		_acceptexaddrs = getAcceptExAddrs(listenfd);
		_acceptHandle.accept = this;

		return true;
	}

	bool Accept::accept(const IoSockptr& sioptr)
	{
		SOCKET fd = WSASocket(_isipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (fd == INVALID_SOCKET)
		{
			WRITE_LOG("WSASocket() error! errno:%d\n", WSAGetLastError());
			return false;
		}
		setnodelay(fd);
		unsigned int addrLen = (_isipv6 ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN)) + 16;
		//AcceptEx()
		assert(_acceptex);
		if (!_acceptex(_listenfd, fd, _recvbuf, 0, addrLen, addrLen, &_recvlen, &_acceptHandle._overlapped))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				WRITE_LOG("AcceptEx() error! errno:%d\n", WSAGetLastError());
				closesocket(fd);
				return false;
			}
		}
		
		_sioptr = sioptr;
		_socket = fd;
		return true;
	}

	void Accept::onIocpAccept(int err)
	{
		//wait accept() complete
		while (_socket == INVALID_SOCKET) Sleep(5);

		if (err == ERROR_SUCCESS)
		{
			setsockopt(_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&_listenfd, sizeof(_listenfd));

			struct sockaddr* plocaladdr = NULL;
			struct sockaddr* ppeeraddr = NULL;
			socklen_t localaddrlen;
			socklen_t peeraddrlen;
			DWORD dwAddressLength = (_isipv6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) + 16;

			assert(_acceptexaddrs);
			_acceptexaddrs(_recvbuf, _recvlen, dwAddressLength, dwAddressLength,
				&plocaladdr, &localaddrlen, &ppeeraddr, &peeraddrlen);
			//addr
			_sioptr->setAddrs(ppeeraddr);
			_sioptr->createIoPort(_loop, _socket);
		}
		else
		{
			closesocket(_socket);
			//_socket = INVALID_SOCKET;
		}

		
		_socket = INVALID_SOCKET;
		//Reference Count
		IoSockptr tmpptr(std::move(_sioptr));
		onAccept(tmpptr, err);
		
	}

	void Accept::close()
	{
		if (_listenfd != INVALID_SOCKET)
		{
			closesocket(_listenfd);
			_listenfd = INVALID_SOCKET;
		}
	}
}