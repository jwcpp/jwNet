#include "Accept.h"
#include "../NetDef.h"

using namespace net;

Accept::Accept()
{
	_listenfd = INVALID_SOCKET;
	_loop = NULL;

	_event._ev.events = 0;
	_event._ev.data.ptr = &_event;
	_event._type = SEpollEvent::eEPV_ACCPET;
	_event.accept = this;
}
Accept::~Accept()
{
	this->close();
}

bool Accept::listen(NetLoop* loop, const char* ip, unsigned short port)
{
	IPAddres addr(ip, port);
	int fd_ = socket(addr.isIpv6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
	if (fd_ == INVALID_SOCKET) {
		return false;
	}

	nonblocking(fd_);
	net::setreuse(fd_);

	if (net::bind(fd_, addr) != 0){
		::close(fd_);
		WRITE_LOG("bind socket error\n");
		return false;
	}

	if (net::listen(fd_, SOMAXCONN) != 0) {
		::close(fd_);
		WRITE_LOG("listen socket error\n");
		return false;
	}

	_event._ev.events = EPOLLET;
	//_event._ev.events = 0;
	if (!loop->registerEpoll(EPOLL_CTL_ADD, fd_, &_event._ev))
		return false;

	_isipv6 = addr.isIpv6();
	_listenfd = fd_;
	_loop = loop;
	return true;
}

bool Accept::accept(const IoSockptr& sioptr)
{
	_event._ev.events = _event._ev.events | EPOLLIN;
	if (!_loop->registerEpoll(EPOLL_CTL_MOD, _listenfd, &_event._ev))
		return false;

	_sioptr = sioptr;
	return true;
}

void Accept::onEpollAccept(uint32 ev)
{
	//È¡ÏûEPOLLIN
	_event._ev.events = _event._ev.events & ~EPOLLIN;
	_loop->registerEpoll(EPOLL_CTL_MOD, _listenfd, &_event._ev);

	IoSockptr tmp(std::move(_sioptr));

	if (ev & EPOLLERR || ev & EPOLLHUP)
	{
		WRITE_LOG("Accept EPOLLERR, errno:%d\n",errno);
		onAccept(tmp, errno);
		return;
	}
	
	if (!(ev & EPOLLIN)) return;

	IPAddres addr(_isipv6);
	socklen_t len = addr.addrSizeof();
	int fd_ = net::accept(_listenfd, addr);
	if (fd_ == -1)
	{
		WRITE_LOG("Accept fd = -1, errno:%d\n", errno);
		onAccept(tmp, errno);
		return;
	}
	nonblocking(fd_);
	tmp->setAddrs(addr);
	tmp->addEpoll(_loop, fd_);
	onAccept(tmp, 0);
}

void Accept::close()
{
	if (_listenfd != INVALID_SOCKET)
	{
		::close(_listenfd);
		_listenfd = INVALID_SOCKET;
	}
}