#pragma once

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	#define WIN_OS
	#include <ws2tcpip.h>
	#include <windows.h>
#else
	#define LINUX_OS
	#include <errno.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/time.h>
	#include <unistd.h>
	#include <sys/epoll.h>
	#include <fcntl.h>

	typedef int	SOCKET;
	#define INVALID_SOCKET -1
#endif

#include <stdint.h>
#include <chrono>

typedef int8_t int8;
typedef uint8_t uint8;

typedef int16_t int16;
typedef uint16_t uint16;

typedef int32_t int32;
typedef uint32_t uint32;

typedef int64_t int64;
typedef uint64_t uint64;

namespace base {
	inline int32 getProcessPID()
	{
#ifdef WIN_OS
		return (int32)GetCurrentProcessId();
#else
		return getpid();
#endif
	}

	inline char* strerror(int ierrorno = 0)
	{
#ifdef WIN_OS
		if (ierrorno == 0)
			ierrorno = GetLastError();

		static char lpMsgBuf[256] = { 0 };

		/*
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			ierrorno,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			1024,
			NULL
		);
		*/
		snprintf(lpMsgBuf, 256, "errorno=%d", ierrorno);
		return lpMsgBuf;
#else
		if (ierrorno != 0)
			return strerror(ierrorno);
		return strerror(errno);
#endif
	}

	inline int lasterror()
	{
#ifdef WIN_OS
		return GetLastError();
#else
		return errno;
#endif
	}

	/** 获取系统时间(精确到毫秒) */
#ifdef WIN_OS
	inline uint32 getSystemTime()
	{
		// 注意这个函数windows上只能正确维持49天。
		return ::GetTickCount();
	};
#else
	inline uint32 getSystemTime()
	{
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	};
#endif

	/* get time in millisecond 64 */
	inline uint64 getTimeMs()
	{
#ifdef WIN_OS
		auto timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		return timeNow.count();
#else
		timeval time;
		gettimeofday(&time, NULL);
		return (uint64)((time.tv_sec * 1000) + (time.tv_usec / 1000));
#endif
	}

#ifdef WIN_OS
	inline void sleep(uint32 ms)
	{
		::Sleep(ms);
	}
#else
	inline void sleep(uint32 ms)
	{
		struct timeval tval;
		tval.tv_sec = ms / 1000;
		tval.tv_usec = (ms * 1000) % 1000000;
		select(0, NULL, NULL, NULL, &tval);
	}
#endif
};//namespace