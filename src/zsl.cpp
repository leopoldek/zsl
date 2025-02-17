#include "zsl/core.h"
#if defined(ZSL_WINDOWS)
	#include "os_windows.cpp"
#elif defined(ZSL_LINUX)
	#include "os_linux.cpp"
#endif

#include "memory.cpp"
