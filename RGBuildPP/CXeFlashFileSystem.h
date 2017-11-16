#ifndef _CXeFlashFileSYSTEM_H_
#define _CXeFlashFileSYSTEM_H_
#include "CXeFlashBlockDriver.h"
inline void Log(int priority, const char* szFormat, ...);
typedef struct _FLASHFILESYSTEM_ENTRY

{
	CHAR cFileName[0x16];
	WORD wBlockNumber;
	DWORD dwLength;
	DWORD dwTimeStamp;
} FLASHFILESYSTEM_ENTRY, *PFLASHFILESYSTEM_ENTRY;

class CXeFlashFileSystemRoot
{
public:
	CXeFlashBlockDriver* xepBlkDriver;
	int dwVersion;
	BYTE * pbData;
	DWORD cbData;
	WORD * pwBlockMap;
	FLASHFILESYSTEM_ENTRY * pfsEntries;
	DWORD fscbEntries;

	BOOL ChainAllocBlock(PWORD pwBlkIdx);
	BOOL ChainAllocBlocks(WORD wBlksNeeded, WORD wMinBlk, PWORD pwBlkIdx);

	void ChainFreeChain(WORD wBlkIdx);

	WORD * ChainGetChain(WORD wStartBlock, PWORD pwChainLength);
	WORD * ChainGetChain(WORD wStartBlock, DWORD limit, PWORD pwChainLength);
	DWORD ChainSetChainData(WORD wStartBlock, BYTE* pbData, DWORD cbData);

	BYTE * FileGetData(FLASHFILESYSTEM_ENTRY* fsEntry);
	void FileSetData(FLASHFILESYSTEM_ENTRY* fsEntry, BYTE* pbData, DWORD cbData);

	FLASHFILESYSTEM_ENTRY* FindEntry(PSZ szName);
	errno_t Load();
};
#endif
