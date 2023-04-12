#pragma once

#include "../Platform.h"
#include "../Utils.h"
#include "../PostParam.h"

namespace net
{
#define POST_USER_TYPE 'P'
	class NetLoop : public base::Nocopyable
	{
	public:
		NetLoop();
		~NetLoop();
		bool init();
		void destroy();
		int  run(int timeout = 5);
		inline HANDLE io() { return _io; }

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
		void* _io;
	};
}