#pragma once
#include "Platform.h"

#ifdef _SET_LOG
	extern void writeLog(char const* const _Format, ...);
	#define WRITE_LOG writeLog

#else
	#define WRITE_LOG printf

#endif