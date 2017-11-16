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

typedef struct _FLASHMOBILEDATA
{
	BYTE bDataType;
	DWORD dwDataSequence;
	DWORD dwPage;
	BYTE* pbData;
	DWORD cbData;
} FLASHMOBILEDATA, *PFLASHMOBILEDATA;

typedef struct _XE_CORONA_FS_DATA
{
    BYTE bSectionDigest[0x14]; // digest of all the data
	DWORD dwUnknown;
	DWORD dwFSVersion;
    WORD wFSBlockIdx;
    WORD wUnknown;
    WORD wMobile1BlockIdx;
	WORD wMobile1Length;
    BYTE bUnknown2[0x8];
    WORD wMobile2BlockIdx;
	WORD wMobile2Length;
	BYTE bUnknown[0x1D0];
} XE_CORONA_FS_DATA, *PXE_CORONA_FS_DATA;

class CXeFlashFileSystemRoot
{
public:
	CXeFlashBlockDriver* xepBlkDriver;
	XE_CORONA_FS_DATA* xepCoronaData;
	int dwVersion;
	WORD wBlkIdx;
	WORD * pwBlockMap;
	DWORD cbBlockMap;
	FLASHFILESYSTEM_ENTRY * pfsEntries;
	DWORD cfsEntries;

	
	CXeFlashFileSystemRoot(){xepCoronaData = 0; xepBlkDriver = 0; dwVersion = 0; wBlkIdx = 0;pwBlockMap = 0;cbBlockMap = 0;pfsEntries = 0;cfsEntries=0;};

	__checkReturn errno_t ChainAllocBlock(PWORD pwBlkIdx);
	__checkReturn errno_t ChainAllocBlocks(WORD wBlksNeeded, WORD wMinBlk, PWORD pwBlkIdx);

	WORD * ChainGetFromStart(WORD wStartBlock, PWORD pwChainLength);
	WORD ChainGetPrevious(WORD wBlkIdx);

	void ChainFreeChain(WORD wBlkIdx);

	WORD * ChainGetChain(WORD wStartBlock, PWORD pwChainLength);
	WORD * ChainGetChain(WORD wStartBlock, DWORD limit, PWORD pwChainLength);
	DWORD ChainSetChainData(WORD wStartBlock, BYTE* pbData, DWORD cbData);

	FLASHFILESYSTEM_ENTRY* FileSearch(PSZ szName);
	BYTE * FileGetData(FLASHFILESYSTEM_ENTRY* fsEntry);
	__checkReturn errno_t FileSetData(FLASHFILESYSTEM_ENTRY* fsEntry, BYTE* pbData, DWORD cbData);
	__checkReturn errno_t FileAdd(PSZ szFileName, FLASHFILESYSTEM_ENTRY** pfsEntry);
	FLASHFILESYSTEM_ENTRY* FindEntry(PSZ szName);

	__checkReturn errno_t Load(WORD wBlkIdx);
	__checkReturn errno_t Save(WORD wBlkIdx);
};
#endif
