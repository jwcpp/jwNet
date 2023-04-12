#pragma once

#include "../Platform.h"
#include "../Utils.h"
#include "../PostParam.h"
#include "../concurrentqueue.h"

namespace net
{
#define EPOLL_WAIT_MAX 2048
	class NetLoop : public base::Nocopyable
	{
	public:
		NetLoop();
		~NetLoop();
		bool init();
		void destroy();
		int  run(int timeout = 5);
		bool registerEpoll(int op, int fd, epoll_event * ev);

		template<typename FUNC>
		void post(FUNC&& func) {
			auto ptr = new PostNone<FUNC>(std::forward<FUNC>(func));
			post((void*)ptr);
		}
		template<typename FUNC, typename... ARGS>
		void post(FUNC&& func, ARGS &&...args) {
			auto ptr = new PostArge<FUNC, ARGS...>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
			post((void*)ptr);
		}
	private:
		void post(void* data);
	private:
		int _epoll = INVALID_SOCKET;
		epoll_event _events[EPOLL_WAIT_MAX];
		moodycamel::ConcurrentQueue<void*> posts;
	};
}