// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <direct.h>
#include <io.h>   // For access().
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().

#ifdef _XBOX
#include <xtl.h>
#include <xboxmath.h>
#else

#ifdef _WIN32
#include <Windows.h>
#else
#endif

#endif

// TODO: reference additional headers your program requires here
