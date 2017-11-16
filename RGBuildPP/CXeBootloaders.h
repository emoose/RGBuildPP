#ifndef _CXEBOOTLOADERS_H
#define _CXEBOOTLOADERS_H
#include "XeCrypt.h"

#define XBOX_BLDR_FLAG_2BL_ZEROPAIR 0x1
#define XBOX_BLDR_FLAG_2BL_DUALLDR 0x800
#define XBOX_BLDR_FLAG_2BL_NEWCRYPTO 0x1000

#define XBOX_BLDR_MAGIC_DEVKIT 0x1000

inline void Log(int priority, const char* szFormat, ...);
class CXeBootloaderHeader
{
public:
	WORD wMagic;
	WORD wBuild;
	WORD wQfe;
	WORD wFlags;
	DWORD dwEntrypoint;
	DWORD dwLength;
};
typedef struct BOOTLOADER_2BL_PERBOX
{
	BYTE bPairingData[0x3];  // 0x00:0x03 bytes
	BYTE bLockDownValue;// 0x03:0x01 bytes
	BYTE bReserved[0xC];     // 0x04:0x0C bytes
	BYTE bPerBoxDigest[0x10]; // 0x10:0x10 bytes
} BOOTLOADER_2BL_PERBOX, *PBOOTLOADER_2BL_PERBOX;
typedef struct BOOTLOADER_2BL_ALLOWDATA
{
	BYTE bConsoleType; // 0x00:0x01
	BYTE bConsoleSequence; // 0x01:0x01
	WORD wConsoleSequenceAllow; // 0x02:0x02
} BOOTLOADER_2BL_ALLOWDATA, *PBOOTLOADER_2BL_ALLOWDATA;
typedef struct BOOTLOADER_6BL_PERBOX
{
	BYTE bReserved[0x2B];
	BYTE bUpdateSlotNumber;
	BYTE bPairingData[0x3];
	BYTE bLockDownValue;
	BYTE bPerBoxDigest[0x10];
} BOOTLOADER_6BL_PERBOX, *PBOOTLOADER_6BL_PERBOX;
typedef struct BOOTLOADER_7BL_PERBOX
{
	WORD wUsedBlocksCount;
	WORD wBlockNumbers[223];
} BOOTLOADER_7BL_PERBOX, *PBOOTLOADER_7BL_PERBOX;

class CXeSMC
{
public:
	BYTE * pbData;
	DWORD cbData;
	BYTE * pbMobo;
	WORD * pwVersion;
	BOOL bIsDecrypted;
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	__checkReturn BOOL UnMunge();
	__checkReturn BOOL Munge();
	PSZ GetMobo();
	__checkReturn BOOL GetHash(BYTE* pbHash, BOOL bUseDecrypted);
	__checkReturn BOOL GetHash(BYTE* pbHash);
};
class CXeBootloader
{
public:
	CXeBootloaderHeader blHdr;
	BYTE * pbData;
	DWORD cbData;
	BYTE * pbCPUKey;
	BYTE * pbPrevBldrKey;
	BYTE bRc4Key[0x10];
	BYTE * pbHmacShaNonce;
	BOOL bIsDecrypted;
	WORD wValidMagic;
	
	CXeBootloader(){pbCPUKey = 0; pbPrevBldrKey = 0; pbData = 0;bIsDecrypted = FALSE;};
	virtual errno_t Load(BOOL isEncrypted) { return -1; };
	virtual errno_t Save(BOOL saveEncrypted) { return -1; };
	void LoadBldrHdr();
	void SaveBldrHdr();
	__checkReturn BOOL Resize(DWORD size);
	void PatchBootloader(BYTE* pbData, DWORD* cbData);
	__checkReturn BOOL isValid();
};
class CXeBootloader1BL : public CXeBootloader
{
public:
	CXeBootloader1BL():CXeBootloader(){this->wValidMagic = 0x4341;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
};
class CXeBootloader2BL : public CXeBootloader
{
public:
	BOOTLOADER_2BL_PERBOX blPerBoxData;
	BYTE* pbSignature; // 0x100
	BYTE* pbAesInvData; //0x110
	ULONGLONG ulPostOutputAddr;
	ULONGLONG ulSbFlashAddr;
	ULONGLONG ulSocMmioAddr;
	BYTE* pbRsaPublicKey; // 0x110
	BYTE* pb3BLNonce; // 0x10
	BYTE* pb3BLSalt; // 0xA
	BYTE* pb4BLSalt; // 0xA
	BYTE* pb4BLDigest; // 0x14
	BOOTLOADER_2BL_ALLOWDATA blAllowData;
	DWORD dwPadding;

	CXeSMC* pxeSMC;

	CXeBootloader2BL():CXeBootloader(){this->wValidMagic = 0x4342;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	void Crypt();
	__checkReturn BOOL FixPerBoxDigest();
};
class CXeBootloader3BL : public CXeBootloader
{
public:
	BYTE * pbSignature; // 0x0:0x100 bytes

	CXeBootloader3BL():CXeBootloader(){this->wValidMagic = 0x4343;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	void Crypt();
};
class CXeBootloader4BL : public CXeBootloader
{
public:
	BYTE * pbSignature; // 0x100
	BYTE * pbRsaPublicKey; // 0x110
	BYTE * pb6BLNonce; // 0x10
	BYTE * pb6BLSalt; // 0xA
	WORD wPadding;
	BYTE * pb5BLDigest; // 0x14
	BOOL bUsesCpuKey;
	BYTE * pbBetaKitNonce;

	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	__checkReturn errno_t Crypt(BOOL isDecrypting);
	CXeBootloader4BL():CXeBootloader(){this->wValidMagic = 0x4344; this->bUsesCpuKey = false;this->pbBetaKitNonce = 0;};
};
class CXeBootloader5BL : public CXeBootloader
{
public:
	ULONGLONG ulImageAddr;
	DWORD dwImageLength;
	DWORD dwPadding;

	CXeBootloader5BL():CXeBootloader(){this->wValidMagic = 0x4345;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	void Crypt();
};
class CXeBootloader6BL : public CXeBootloader
{
public:
	DWORD dwBuildQfeSource;
	DWORD dwBuildQfeTarget;
	DWORD dwReserved;
	DWORD dw7BLLength;
	BOOTLOADER_7BL_PERBOX bl7BLPerBoxData;
	BOOTLOADER_6BL_PERBOX bl6BLPerBoxData;
	BYTE * pbSignature; // 0x100
	BYTE * pb7BLNonce; // 0x10
	BYTE * pb7BLDigest; // 0x14
	DWORD dwPadding;

	CXeBootloader6BL():CXeBootloader(){this->wValidMagic = 0x4346;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	void Crypt();
	__checkReturn BOOL FixPerBoxDigest();
};
class CXeBootloader7BL : public CXeBootloader
{
public:
	DWORD dwSourceImageLength;
	BYTE * pbSourceDigest; // 0x14
	DWORD dwTargetImageLength;
	BYTE * pbTargetDigest; // 0x14
	DWORD dw6BLLength;

	CXeBootloader7BL():CXeBootloader(){this->wValidMagic = 0x4347;};
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	void Crypt();
};
class CXeBootloaderFlash : public CXeBootloader
{
public:
	BYTE bCopyrightSign;
	CHAR bCopyright[0x3F]; // 0x3F
	BYTE bReserved[0x10]; // 0x10
	DWORD dwKeyVaultLength;
	DWORD dwSysUpdateAddr;
	WORD wSysUpdateCount;
	WORD wKeyVaultVersion;
	DWORD dwKeyVaultAddr;
	DWORD dwFileSystemAddr;
	DWORD dwSmcConfigAddr;
	DWORD dwSmcLength;
	DWORD dwSmcAddr;
	WORD wRgbpIndicator;


	__checkReturn errno_t CreateDefaults();
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
};

#endif