#ifndef _UTIL_H_
#define _UTIL_H_
#ifdef _XBOX
#include "xekernelp.h"
#endif
#include "dirent.h"
#include "CXeFlashImage.h"
extern void DbgPrint(const char* szFormat, ...);
extern void Log(int priority, const char* szFormat, ...);

typedef ULONGLONG QWORD;
typedef struct _X360_DEVICE
{
		char Device[256];
		char Drive[256];
        BOOL Success;
} X360_DEVICE, *PX360_DEVICE;

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
#pragma warning(suppress: 6328)
        sscanf_s(pos, "%2hhx", &buffer[count]);
        pos += 2 * sizeof(char);
    }

    return 0;
}
inline errno_t readData(PSZ path, BYTE ** data, DWORD* dwLength)
{
	FILE * file;
	errno_t err = fopen_s(&file, path, "rb");
	if(err != 0 || file == 0)
		return err;
	fseek(file, 0, SEEK_END);
	*dwLength = ftell(file);
	*data = (BYTE*)malloc(*dwLength);
	fseek(file, 0, SEEK_SET);
	fread(*data, 1, *dwLength, file);
	fclose(file);
	return 0;
}
inline errno_t saveData(PSZ path, BYTE * data, DWORD dwLength)
{
	FILE* file;
	errno_t err = fopen_s(&file, path, "wb+");
	if(err != 0 || file == 0)
	{
		Log(3, "error writing file %s: 0x%x\n", path, err);
		return err;
	}
	fwrite((void*)data, 1, dwLength, file);
	fclose(file);
	return err;
}
inline BOOL directoryExists(PSZ path)
{
	if(_access(path, 0) != 0)
		return false;
	struct stat status;
	stat(path, &status);
	if(!(status.st_mode & S_IFDIR))
		return false;
	return true;
}

inline errno_t loadMobileFromFile(CXeFlashImage* img, PSZ szPath, BYTE bType)
{
	errno_t ret = 0;
	bType = bType + 0x31;
	BYTE* blData;
	DWORD len;
	if(ret = readData(szPath, &blData, &len) != 0)
		return ret;
	FLASHMOBILEDATA* data;
	ret = img->MobileAddFile(bType, &data);
	data->pbData = blData;
	data->cbData = len;
	return 0;
}
inline errno_t loadConfigBlkFromFile(CXeFlashImage* img, PSZ szPath)
{
	BYTE* blData;
	DWORD len;
	if(errno_t ret = readData(szPath, &blData, &len) != 0)
		return ret;
	img->SetConfigBlocks(blData, len);
	free(blData);
	return 0;
}
inline errno_t loadKVFromFile(CXeFlashImage* img, CXeKeyVault* bldr, PSZ szPath)
{
	BYTE* blData;
	DWORD len;
	if(errno_t ret = readData(szPath, &blData, &len) != 0)
		return ret;
	memcpy(&bldr->xeData, blData, len);
	free(blData);
	bldr->pwKeyVaultVersion = &img->blFlash.wKeyVaultVersion;
	bldr->pbCPUKey = img->pbCPUKey;
	return bldr->Load(false);
}
inline errno_t loadSMCFromFile(CXeSMC* bldr, PSZ szPath)
{
	BYTE* blData;
	DWORD len;
	if(errno_t ret = readData(szPath, &blData, &len) != 0)
		return ret;
	bldr->pbData = blData;
	bldr->cbData = len;
	return bldr->Load(false);
}
inline errno_t addFilesFromDir(CXeFlashImage* img, PSZ path)
{
	if(img->GetFS() == NULL)
		return 3;
	DIR *dir;
	struct dirent *ent;
    dir = opendir (path);
	if (dir != NULL)
	{
		while ((ent = readdir (dir)) != NULL)
		{
			if(ent->d_type == DT_DIR)
				continue;
			if(img->GetFS()->FileSearch(ent->d_name) != NULL)
				continue;
			if(!memcmp(ent->d_name, "Mobile", 6))
				continue;
			char namebuff[4096];
			sprintf_s(namebuff, 4096, "%s%s", path, (char*)&ent->d_name);
			FLASHFILESYSTEM_ENTRY* entr;
			if(img->GetFS()->FileAdd(ent->d_name, &entr) != 0)
			{
				Log(3, "addFilesFromDir: failed adding file %s!\n", ent->d_name);
				return 1;
			}
			BYTE* blData;
			DWORD len;
			if(errno_t ret = readData(namebuff, &blData, &len) != 0)
				return ret;
			if(!img->GetFS()->FileSetData(entr, blData, len))
			{
				Log(3, "addFilesFromDir: failed setting file %s data!\n", ent->d_name);
				return 2;
			}
			free(blData);
		}
		closedir(dir);
	}
	else
	{
		Log(3, "addFilesFromDir: error reading path %s!\n", path);
		return 1;
	}
	return 0;
}
inline errno_t loadBldrFromFile(CXeFlashImage* img, CXeBootloader* bldr, PSZ szPath)
{
	if(strlen(szPath) < 1)
		return 0;
	BYTE* blData;
	DWORD len;
	if(errno_t ret = readData(szPath, &blData, &len) != 0)
		return ret;
	Log(0, "loadBldrFromFile: loading bootloader from %s", szPath);
	bldr->pbCPUKey = img->pbCPUKey;
	bldr->pbData = blData;
	bldr->cbData = len;
	bldr->Load(false);
	return 0;
}
#endif