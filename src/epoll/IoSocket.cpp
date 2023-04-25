#include "IoSocket.h"

namespace net
{

	IoSocket::IoSocket():
		_close(false)
	{
		_fd = INVALID_SOCKET;
		_loop = NULL;
		_event._type = SEpollEvent::eEPV_SOCKET;
		_event._ev.data.ptr = &_event;

		_isconnect = false;
		_recv.reset();
		_writeBuf = NULL;
		_writeLen = 0;
	}

	IoSocket::~IoSocket()
	{
		finalClose();
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
		if (!loop->registerEpoll(EPOLL_CTL_ADD, fd, &_event._ev))
		{
			return false;
		}

		_fd = fd;
		_loop = loop;
		_isconnect = true;
		_event.sockptr = shared_from_this();
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
		if (!_loop->registerEpoll(EPOLL_CTL_ADD, fd, &_event._ev))
		{
			return false;
		}
		
		_fd = fd;
		_isconnect = true;
		_event.sockptr = shared_from_this();
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
			if (!_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev)){
				return false;
			}
			_event.sockptr = shared_from_this();
		}
		else
		{
			onWrite(ret, 0);
		}

		return true;
		
	}

	bool IoSocket::read(char* buf, int len)
	{
		if (buf == NULL || len <= 0) return false;

		if (!(_event._ev.events & EPOLLIN))
		{
			_event._ev.events = _event._ev.events | EPOLLIN;
			if (!_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev)){
				return false;
			}
			_event.sockptr = shared_from_this();
		}

		_recv._recvBuf = buf;
		_recv._recvLen = len;
		if (_recv._hasData) {
			handleRead();
		}
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
		if (_isconnect)
		{
			_isconnect = false;
			if (ev & EPOLLERR || ev & EPOLLHUP){
				this->finalClose(); //reconnect close this socket
				onConnect(errno == 0 ? 0xffffffff : errno);
			}
			else if (ev & EPOLLOUT){ //connect success
				_event._ev.events = _event._ev.events & ~EPOLLOUT;
				_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);
				onConnect(0);
			}
			else{
				assert(0);
			}
			return;
		}

		if (ev & EPOLLHUP ||//shtdown (recive bytes == 0 && errno == 0)
			ev & EPOLLERR){ //opposite close (errno > 0)
			onRead(0, errno == 0 ? 0xffffffff: errno);
			return;
		}


		if (ev & EPOLLIN)
		{
			_recv._hasData = true;
			if (_recv.isValid()) {
				handleRead();
			}
		}
		else if (ev & EPOLLOUT)
		{
			int err = 0;
			while(1)
			{
				int ret = send(_fd, _writeBuf, _writeLen, 0);
				if (ret < 0)
				{
					if (errno == EINTR) continue;
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						return;
					}
					else {
						err = errno;
					}
				}

				//È¡ÏûEPOLLOUT
				_event._ev.events = _event._ev.events & ~EPOLLOUT;
				_loop->registerEpoll(EPOLL_CTL_MOD, _fd, &_event._ev);

				onWrite(ret, err);
				return;//exec one
			};
		}
	}

	bool IoSocket::handleRead()
	{
		while(1)
		{
			int result = 0;
			int bytes = recv(_fd, _recv._recvBuf, _recv._recvLen, 0);
			if (bytes < 0) {
				if (errno == EINTR) continue;
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					return false;
				}
				_recv._hasData = false;
				result = errno;
			}
			else if (bytes == 0){//close
				_recv._hasData = false;
				result = errno;
			}

			_recv.setInvalid();
			onRead(bytes, result);
			return true;//exec one

		};
	}

	void IoSocket::close()
	{
		if (_close) return;
		_close = true;

		if (_fd != INVALID_SOCKET){
			::shutdown(_fd, SHUT_RDWR);
		}
	}

	void IoSocket::finalClose()
	{
		if (_fd != INVALID_SOCKET)
		{
			if (_loop) {
				_loop->registerEpoll(EPOLL_CTL_DEL, _fd, &_event._ev);
			}
			::close(_fd);
			_fd = INVALID_SOCKET;
		}
	}
}