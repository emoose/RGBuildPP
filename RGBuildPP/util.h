#ifndef _UTIL_H_
#define _UTIL_H_
#ifdef _XBOX
#include "xekernelp.h"
#endif
extern void Log(int priority, const char* szFormat, ...);

typedef ULONGLONG QWORD;
typedef struct _X360_DEVICE
{
		char Device[256];
		char Drive[256];
        BOOL Success;
} X360_DEVICE, *PX360_DEVICE;

inline void DbgPrint(const char* szFormat, ...)
{
	char szBuff[1024];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf_s(szBuff, sizeof(szBuff), szFormat, arg);
	va_end(arg);
#ifndef _XBOX
	OutputDebugString(szBuff);
#endif
	printf(szBuff);
}

// win only byteswap
inline WORD bswap16(WORD x)
{
#ifndef _XBOX
	return (x & 0xFF00) >> 8 | (x & 0x00FF) << 8;
#else
	return x;
#endif
}

// win only byteswap
inline DWORD bswap32(DWORD x)
{
#ifndef _XBOX
	return (x & 0xFF000000) >> 24
		 | (x & 0x00FF0000) >> 8
		 | (x & 0x0000FF00) << 8
		 | (x & 0x000000FF) << 24;
#else
	return x;
#endif
}

// xbox only byteswap
inline DWORD bswap32Xe(DWORD x)
{
#ifdef _XBOX
	return (x & 0xFF000000) >> 24
		 | (x & 0x00FF0000) >> 8
		 | (x & 0x0000FF00) << 8
		 | (x & 0x000000FF) << 24;
#else
	return x;
#endif
}

// platform-independant byteswap
inline DWORD bswap32X(DWORD x)
{
	return (x & 0xFF000000) >> 24
		 | (x & 0x00FF0000) >> 8
		 | (x & 0x0000FF00) << 8
		 | (x & 0x000000FF) << 24;
}

// win only byteswap
inline ULONGLONG bswap64(ULONGLONG x)
{
#ifndef _XBOX
	return (x & 0xFF00000000000000ULL) >> 56
		 | (x & 0x00FF000000000000ULL) >> 40
		 | (x & 0x0000FF0000000000ULL) >> 24
		 | (x & 0x000000FF00000000ULL) >> 8
		 | (x & 0x00000000FF000000) << 8
		 | (x & 0x0000000000FF0000) << 24
		 | (x & 0x000000000000FF00) << 40
		 | (x & 0x00000000000000FF) << 56;
#else
	return x;
#endif
}

inline int strToBytes(PSZ string, BYTE* buffer, DWORD len)
{

	CHAR* pos = (CHAR*)string;
    size_t count = 0;

     /* WARNING: no sanitization or error-checking whatsoever */
    for(count = 0; count < len; count++) {
        sscanf_s(pos, "%2hhx", &buffer[count]);
        pos += 2 * sizeof(char);
    }

    return 0;
}

inline errno_t saveData(PSZ path, BYTE * data, DWORD dwLength)
{
	FILE* file;
	errno_t err = fopen_s(&file, path, "wb+");
	if(err != 0)
	{
		Log(3, "error writing file %s: 0x%x\n", path, err);
		return err;
	}
	fwrite((void*)data, 1, dwLength, file);
	fclose(file);
	return err;
}
#endif