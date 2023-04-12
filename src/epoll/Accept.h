#pragma once
#include "IoSocket.h"
#include "NetLoop.h"

namespace net
{
	class Accept
	{
		friend class NetLoop;
	public:
		Accept();
		~Accept();

		bool listen(NetLoop* loop, const char* ip, unsigned short port);
		bool accept(const IoSockptr& sioptr);
		void close();
	private:
		void onEpollAccept(uint32 ev);

	protected:
		virtual void onAccept(IoSockptr& sioptr, int err) = 0;
	private:
		int _listenfd;
		NetLoop * _loop;
		SEpollAccept _event;
		bool	  _isipv6 = false;
		IoSockptr _sioptr = nullptr;
	};
}

