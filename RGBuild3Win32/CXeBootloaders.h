#ifndef _XEBOOTLOADERS_H_
#define _XEBOOTLOADERS_H_
class CXeBootloader_Header
{
public:
	WORD* wMagic;
	WORD* wBuild;
	WORD* wQfe;
	WORD* wFlags;
	DWORD* dwEntrypoint;
	DWORD* dwLength;
};
typedef struct _BOOTLOADER_2BL_PERBOX
{
	BYTE bPairingData;  // 0x00:0x03 bytes
	BYTE bLockDownValue;// 0x03:0x01 bytes
	BYTE bReserved;     // 0x04:0x0C bytes
	BYTE bPerBoxDigest; // 0x10:0x10 bytes
} BOOTLOADER_2BL_PERBOX, *PBOOTLOADER_2BL_PERBOX;
typedef struct _BOOTLOADER_2BL_ALLOWDATA
{
	BYTE bConsoleType; // 0x00:0x01
	BYTE bConsoleSequence; // 0x01:0x01
	WORD wConsoleSequenceAllow; // 0x02:0x02
} BOOTLOADER_2BL_ALLOWDATA, *PBOOTLOADER_2BL_ALLOWDATA;
class CXeBootloader_2BL_Header : CXeBootloader_Header
{
public:
	BOOTLOADER_2BL_PERBOX* blPerBoxData;
	BYTE* bSignature; // 0x100
	BYTE* bAesInvData; //0x110
	ULONG* ulPostOutputAddr;
	ULONG* ulSbFlashAddr;
	ULONG* ulSocMmioAddr;
	BYTE* bRsaPublicKey; // 0x110
	BYTE* b3BLNonce; // 0x10
	BYTE* b3BLSalt; // 0xA
	BYTE* b4BLSalt; // 0xA
	BYTE* b4BLDigest; // 0x14
	BOOTLOADER_2BL_ALLOWDATA* blAllowData;
	DWORD* dwPadding;
};
class CXeBootloader_3BL_Header : CXeBootloader_Header
{
public:
	BYTE * bSignature; // 0x0:0x100 bytes
};
class CXeBootloader_4BL_Header : CXeBootloader_Header
{
public:
	BYTE * bSignature; // 0x100
	BYTE * bRsaPublicKey; // 0x110
	BYTE * b6BLNonce; // 0x10
	BYTE * b6BLSalt; // 0xA
	WORD * wPadding;
	BYTE * b5BLDigest; // 0x14
};
class CXeBootloader_5BL_Header : CXeBootloader_Header
{
	ULONG * ulImageAddr;
	DWORD * dwImageLength;
	DWORD * dwPadding;
};
typedef struct _BOOTLOADER_6BL_PERBOX
{
	BYTE bReserved[0x2B];
	BYTE bUpdateSlotNumber;
	BYTE bPairingData[0x3];
	BYTE bLockDownValue;
	BYTE bPerBoxDigest[0x10];
} BOOTLOADER_6BL_PERBOX, *PBOOTLOADER_6BL_PERBOX;
typedef struct _BOOTLOADER_7BL_PERBOX
{
	WORD wUsedBlocksCount;
	WORD wBlockNumbers[223];
} BOOTLOADER_7BL_PERBOX, *PBOOTLOADER_7BL_PERBOX;
class CXeBootloader_6BL_Header : CXeBootloader_Header
{
public:
	DWORD * dwBuildQfeSource;
	DWORD * dwBuildQfeTarget;
	DWORD * dwReserved;
	DWORD * dw7BLLength;
	BOOTLOADER_7BL_PERBOX * bl7BLPerBoxData;
	BOOTLOADER_6BL_PERBOX * bl6BLPerBoxData;
	BYTE * bSignature; // 0x100
	BYTE * b7BLNonce; // 0x10
	BYTE * b7BLDigest; // 0x14
	DWORD * dwPadding;
};
class CXeBootloader_7BL_Header : CXeBootloader_Header
{
public:
	DWORD * dwSourceImageLength;
	BYTE * bSourceDigest; // 0x14
	DWORD * dwTargetImageLength;
	BYTE * bTargetDigest; // 0x14
	DWORD * dw6BLLength;
};
class CXeBootloader_Flash_Header : CXeBootloader_Header
{
public:
	BYTE * bCopyrightSign;
	CHAR * bCopyright; // 0x3F
	BYTE * bReserved; // 0x10
	DWORD * dwKeyVaultLength;
	DWORD * dwSysUpdateAddr;
	WORD * wSysUpdateCount;
	WORD * wKeyVaultVersion;
	DWORD * dwKeyVaultAddr;
	DWORD * dwFileSystemAddr;
	DWORD * dwSmcConfigAddr;
	DWORD * dwSmcLength;
	DWORD * dwSmcAddr;
	WORD * wRgbpIndicator;
};
class CXeBootloader
{
public:
	BYTE * pbData;
	CXeBootloader_Header blHdr;
	BYTE * pbHmacShaKey;
	BYTE * pbHmacShaNonce;
	int DecryptLoader();
	int Load();
};
class CXeBootloader_2BL : CXeBootloader
{
public:
	CXeBootloader_2BL_Header blHdr;
	int Load();
};
class CXenon_Flash_Image
{
};
#endif