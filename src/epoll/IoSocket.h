#pragma once

#include "../SocketAPI.h"
#include "../NetDef.h"
#include "NetLoop.h"
#include <memory>

namespace net {
	struct SEpollEvent
	{
		epoll_event _ev;
		enum {
			eEPV_ACCPET,
			eEPV_SOCKET,
		}  _type;
	};

	struct SEpollAccept : public SEpollEvent
	{
		class Accept* accept;
	};

	typedef std::shared_ptr<class IoSocket> IoSockptr;
	struct SEpollSocket : public SEpollEvent
	{
		IoSockptr inptr;
		IoSockptr outptr;
	};

	struct IoRecive
	{
		IoRecive() {
			reset();
		}
		char* _recvBuf;
		int   _recvLen;
		bool  _isRead;

		inline bool isValid()
		{
			return _recvBuf && _recvLen > 0;
		}

		void reset() {
			_recvBuf = NULL;
			_recvLen = 0;
			_isRead = false;
		}
	};


	class IoSocket : public std::enable_shared_from_this<IoSocket>
	{
	public:
		IoSocket();
		virtual ~IoSocket();

		bool connect(NetLoop* loop, const char* ip, unsigned short port);
		bool reconnect();
		bool write(char* buf, int len);
		bool read(char* buf, int len);

		void onEpollEv(uint32 ev);
		void handleRead();
		void addEpoll(NetLoop * loop, int fd);
		void close();
		void setAddrs(struct sockaddr* addrs) { _ipaddr = addrs; }

	protected:
		virtual void onConnect(int err) = 0;
		virtual void onRead(int bytes, int err) = 0;
		virtual void onWrite(int bytes, int err) = 0;

	private:
		SOCKET _fd;
		NetLoop* _loop;
		IPAddres _ipaddr;
		SEpollSocket _event;
		bool isconnect = false;

		IoRecive _recv;
		char* _writeBuf;
		int   _writeLen;
	};
}
