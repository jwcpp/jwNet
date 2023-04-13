#include "IoSocket.h"

namespace net
{

	IoSocket::IoSocket()
	{
		_fd = INVALID_SOCKET;
		_loop = NULL;
		_event._type = SEpollEvent::eEPV_SOCKET;
		_event._ev.data.ptr = &_event;

		isconnect = false;
		_recv.reset();
		_writeBuf = NULL;
		_writeLen = 0;
	}

	IoSocket::~IoSocket()
	{
		close();
	}

	bool IoSocket::connect(NetLoop* loop, const char* ip, unsigned short port)
	{
		_ipaddr = IPAddres(ip, port);
		int type = (_ipaddr.isIpv6() ? AF_INET6 : AF_INET);
		int fd = socket(type, SOCK_STREAM, 0);
		if (fd == -1)
		{
			WRITE_LOG("socket() ret -1\n");
			return false;
		}

		nonblocking(fd);

		int ret = net::connect(fd, _ipaddr);
		if (ret < 0 && errno != EINPROGRESS) {
			return false;
		}

		_event._ev.events = EPOLLET | EPOLLOUT;
		_event.outptr = shared_from_this();
		loop->registerEpoll(EPOLL_CTL_ADD, fd, &_event._ev);

		_fd = fd;
		_loop = loop;
		isconnect = true;

		return true;
	}

	bool IoSocket::reconnect()
	{
		int type = (_ipaddr.isIpv6() ? AF_INET6 : AF_INET);
		int fd = socket(type, SOCK_STREAM, 0);
		if (fd == -1)
		{
			WRITE_LOG("socket() ret -1\n");
			return false;
		}

		nonblocking(fd);

		int ret = net::connect(fd, _ipaddr);
		if (ret < 0 && errno != EINPROGRESS) {
			return false;
		}

		_event._ev.events = EPOLLET | EPOLLOUT;
		_event.outptr = shared_from_this();
		_loop->registerEpoll(EPOLL_CTL_ADD, fd, &_event._ev);

		_fd = fd;
		isconnect = true;
		return true;
	}
	bool IoSocket::write(char* buf, int len)
	{
		int ret = send(_fd, buf, len, 0);
		if (ret <= 0) {
			if (errno == EAGAIN) {
				//
			}

			_writeBuf = buf;
			_writeLen = len;

			//EPOLLOUT
			_event._ev.events = _event._ev.events | EPOLLOUT;
			_event.outptr = shared_from_this();
			_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);
		}
		else
		{
			onWrite(ret, 0);
		}

		return true;
		
	}
	bool IoSocket::read(char* buf, int len)
	{
		if (!(_event._ev.events & EPOLLIN))
		{
			_event._ev.events = _event._ev.events | EPOLLIN;
			_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);
		}

		_event.inptr = shared_from_this();

		_recv._recvBuf = buf;
		_recv._recvLen = len;

		handleRead();

		return true;
	}

	void IoSocket::addEpoll(NetLoop* loop, int fd)
	{
		_event._ev.events = EPOLLET;
		if (loop->registerEpoll(EPOLL_CTL_ADD, fd, &_event._ev)) {
			_fd = fd;
			_loop = loop;
		}
	}

	void IoSocket::onEpollEv(uint32 ev)
	{
		if (isconnect)
		{
			int result = 0;
			if (ev & EPOLLERR || ev & EPOLLHUP)
			{
				//WRITE_LOG("IoSocket::onEpollEv EPOLLERR || EPOLLHUP errno: %d error:%s, line:%d\n", errno, strerror(errno), __LINE__);
				result = errno;
			}
			else if (ev & EPOLLOUT)
			{
				//连接成功
				result = 0;
			}
			else
			{
				WRITE_LOG("IoSocket::onEpollEv Handling Exceptions line:%d\n", __LINE__);
				return;
			}

			isconnect = false;
			IoSockptr tmpptr(std::move(_event.outptr));
			if (result == 0) {
				_event._ev.events = _event._ev.events & ~EPOLLOUT;
				_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);
			}
			else{
				this->close();
			}
			onConnect(result);
			return;
		}

		if (ev & EPOLLHUP || ev & EPOLLERR)
		{
			WRITE_LOG("IoSocket::onEpollEv EPOLLERR || EPOLLHUP errno: %d error:%s, line:%d\n", errno, strerror(errno), __LINE__);
			IoSockptr inptr(std::move(_event.inptr));
			IoSockptr outptr(std::move(_event.outptr));
			this->close();
			onRead(0, errno);
			return;
		}

		//read
		if (ev & EPOLLIN)
		{

			_recv._isRead = true;
			handleRead();
#if 0
			int ret = recv(_fd, _recvBuf, _recvLen, 0);
			if (ret < 0){
				if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK){
					return;
				}
				else{
					onRead(ret, errno);
					return;
				}
			}
			else if (ret == 0){
				//close
				onRead(ret, errno);
				return;
			}
			onRead(ret, 0);
#endif
		}

		//write
		if (ev & EPOLLOUT)
		{
			int err = 0;
			int ret = send(_fd, _writeBuf, _writeLen, 0);
			if (ret < 0)
			{
				if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
					return;
				}
				else {
					//onWrite(ret, errno);
					//return;
					err = errno;
				}
			}

			//取消EPOLLOUT
			_event._ev.events = _event._ev.events & ~EPOLLOUT;
			_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);
			IoSockptr tmpptr(std::move(_event.outptr));

			onWrite(ret, err);
		}
	}

	void IoSocket::handleRead()
	{
		if (!_recv.isValid() || !_recv._isRead) return;

		int result = 0;
		//while(1){ recv();} ==> 替换成在onRead递归
		int ret = recv(_fd, _recv._recvBuf, _recv._recvLen, 0);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				return;
			}
			else {
				_recv._isRead = false;
				result = errno;
			}
		}
		else if (ret == 0) {
			//close
			_recv._isRead = false;
			result = errno;
		}
		else {
			//ret > 0
		}

		_recv._recvBuf = NULL;
		_recv._recvLen = 0;
		IoSockptr tmpptr(std::move(_event.inptr));
		onRead(ret, result);
	}

	void IoSocket::close()
	{
		if (_fd != INVALID_SOCKET)
		{
			_event.inptr = nullptr;
			_event.outptr = nullptr;

			if (_loop) {
				_loop->registerEpoll(EPOLL_CTL_DEL, _fd, &_event._ev);
			}
			::shutdown(_fd, SHUT_RDWR);
			::close(_fd);
			_fd = INVALID_SOCKET;
		}
	}
}