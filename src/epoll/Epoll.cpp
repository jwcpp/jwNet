#include "NetLoop.h"
#include <assert.h>
#include "IoSocket.h"
#include "Accept.h"

namespace net
{
	NetLoop::NetLoop():
		_epoll(INVALID_SOCKET)
	{
	
	}
	NetLoop::~NetLoop()
	{
		destroy();
	}
	bool NetLoop::init()
	{
		assert(_epoll == INVALID_SOCKET);

		_epoll = epoll_create(64);
		if (_epoll == INVALID_SOCKET)
		{
			return false;
		}

		memset(_events, 0, sizeof(epoll_event) * EPOLL_WAIT_MAX);

		return true;
	}
	void NetLoop::destroy()
	{
	
	}

	void NetLoop::post(void* data)
	{
		posts.enqueue(data);
	}

	int  NetLoop::run(int timeout)
	{
		//post data
		void* postptr = NULL;
		while (posts.try_dequeue(postptr))
		{
			PostBase* async = (PostBase*)postptr;
			if (async) {
				async->call();
				delete async;
			}
		}

		//epoll
		int ret = epoll_wait(_epoll, _events, EPOLL_WAIT_MAX, timeout);
		if (ret == 0) return 0;

		if (ret == -1){
			if (errno != EINTR){
				WRITE_LOG("NetLoop::run() epoll_wait errno: %d error:%s\n",errno, strerror(errno));
				return -1;
			}
			return -1;
		}

		for (int i = 0; i < ret; i++)
		{
			void * vdata = _events[i].data.ptr;
			if (!vdata) continue;

			SEpollEvent* epdata = (SEpollEvent*)vdata;
			if (epdata->_type == SEpollEvent::eEPV_ACCPET) {
				SEpollAccept* acceptr = (SEpollAccept*)epdata;
				if (acceptr) {
					acceptr->accept->onEpollAccept(_events[i].events);
				}
			}
			else if (epdata->_type == SEpollEvent::eEPV_SOCKET)
			{
				SEpollSocket* ioptr = (SEpollSocket*)epdata;
				if (ioptr) {
					IoSockptr& ptr = ioptr->inptr ? ioptr->inptr : ioptr->outptr;
					if(ptr) ptr->onEpollEv(_events[i].events);
				}
			}
		}
		
		return 0;
	}

	bool NetLoop::registerEpoll(int op, int fd, epoll_event* ev)
	{
		if (epoll_ctl(_epoll, op, fd, ev) != 0)
		{
			WRITE_LOG("epoll_ctl error, fd:%d\n", fd);
			return false;
		}
		return true;
	}
}