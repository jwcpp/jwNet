#pragma once

#include "../SocketAPI.h"
#include "NetLoop.h"
#include "../NetDef.h"
#include <MSWSock.h>
#include <memory>
#include <atomic>

namespace net {

	struct IocpHandle
	{
		OVERLAPPED _overlapped;
		enum {
			eIH_ACCPET,
			eIH_CONNECT,
			eIH_READ,
			eIH_WRITE,
		}  _type;
	};

	struct IocpHandleAccept : public IocpHandle
	{
		class Accept* accept;
	};

	typedef std::shared_ptr<class IoSocket> IoSockptr;
	struct IocpHandleContext : public IocpHandle
	{
		IoSockptr sockptr;
	};

	LPFN_CONNECTEX getConnectEx(SOCKET fd);
	LPFN_ACCEPTEX  getAcceptEx(SOCKET fd);
	LPFN_GETACCEPTEXSOCKADDRS getAcceptExAddrs(SOCKET fd);

	class IoSocket : public std::enable_shared_from_this<IoSocket>
	{
		friend class NetLoop;
	public:
		IoSocket();
		virtual ~IoSocket();
		bool connect(NetLoop* loop, const char * ip, unsigned short port);
		bool reconnect();
		bool write(char * buf, int len);
		bool read(char* buf, int len);
		void close();
		void setAddrs(struct sockaddr* addrs) { _ipaddr = addrs; }
		const IPAddres* getAddrs() const { return &_ipaddr; }
		bool	createIoPort(NetLoop* loop, SOCKET fd);
		inline bool valid() { return _fd != INVALID_SOCKET; }
		inline SOCKET  getSocket() { return _fd; }
	private:
		void onIocpMsg(int type, int bytes, int err);
	protected:
		virtual void onConnect(int err) = 0;
		virtual void onRead(int bytes, int err) = 0;
		virtual void onWrite(int bytes, int err) = 0;

	private:
		SOCKET _fd;
		IPAddres _ipaddr;

		WSABUF	_recvBuf;
		WSABUF	_writeBuf;
		IocpHandleContext _connHandle;
		IocpHandleContext _recvHandle;
		IocpHandleContext _writeHandle;
	};

};