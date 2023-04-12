#pragma once
#include "IoSocket.h"
#include "NetLoop.h"

namespace net {
	class Accept
	{
		friend class NetLoop;
	public:
		Accept();
		virtual ~Accept();

		bool listen(NetLoop * loop, const char * ip, unsigned short port);
		bool accept(const IoSockptr& sioptr);
		void close();
	private:
		void onIocpAccept(int err);
	protected:
		virtual void onAccept(IoSockptr& sioptr, int err) = 0;

	private:
		NetLoop*		_loop;
		SOCKET			_listenfd;
		IocpHandleAccept _acceptHandle;
		LPFN_ACCEPTEX	_acceptex;
		LPFN_GETACCEPTEXSOCKADDRS _acceptexaddrs;

		bool	  _isipv6 = false;
		SOCKET	  _socket;
		IoSockptr _sioptr = nullptr;
		char	  _recvbuf[200];
		DWORD     _recvlen = 0;
	};
}