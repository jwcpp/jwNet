#pragma once
#include "Platform.h"

#ifdef WIN_OS
	#include "iocp/Accept.h"
#else
	#include "epoll/Accept.h"
#endif