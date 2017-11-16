#ifndef _CXeFLASHIMAGE_H_
#define _CXeFLASHIMAGE_H_
#include "CXeBootloaders.h"
#include "CXeKeyVault.h"
#include "CXeFlashBlockDriver.h"
#include "CXeFlashFileSystem.h"

#define MAX_RGB_PAYLOADS 5

inline void Log(int priority, const char* szFormat, ...);

typedef struct _RGBUILD_PAYLOAD
{
	DWORD dwOffset;
	DWORD cbData;
	BYTE bDescLength;
	CHAR* szDescription;
	BYTE* pbData;
} RGBUILD_PAYLOAD, *PRGBUILD_PAYLOAD;

class CXeFlashImage
{
public:
	BYTE * pb1BLKey;
	BYTE * pbCPUKey;

	CXeBootloaderFlash blFlash;
	CXeSMC xeSMC;
	CXeKeyVault xeKeyVault;
	CXeKeyVault xeAltKeyVault;
	CXeBootloader1BL bl1BL; // beta kits only, might add support for stock cpu 1BL some time
	CXeBootloader2BL bl2BL[2];
	CXeBootloader3BL bl3BL;
	CXeBootloader4BL bl4BL;
	CXeBootloader5BL bl5BL;
	CXeBootloader6BL bl6BL[2];
	CXeBootloader7BL bl7BL[2];
	CXeFlashBlockDriver xeBlkDriver;
	CXeFlashFileSystemRoot * pxeFileSystems;
	DWORD xecbFileSystems;
	XE_CORONA_FS_DATA xeCoronaData[2];
	FLASHMOBILEDATA * pxeMobileData;
	DWORD xecbMobileData;
	int xedwLatestMobileData[9]; // B/C/D/E/F/G/H/I/J
	DWORD xedwLatestFileSystem;
	RGBUILD_PAYLOAD xePayloads[MAX_RGB_PAYLOADS];
	DWORD cbPayloads;
	
	CXeFlashImage(){for(int i=0;i<9;i++){xedwLatestMobileData[i] = -1;}pbCPUKey = 0; pb1BLKey = 0; pxeFileSystems = 0; pxeMobileData = 0; xecbFileSystems = 0; xecbMobileData = 0; xedwLatestFileSystem = 0;};
	__checkReturn errno_t MobileAddFile(BYTE bType, FLASHMOBILEDATA** pfsEntry);
	__checkReturn errno_t PayloadAddFile(PSZ szName, DWORD dwOffset, BYTE* pbData, DWORD cbData);
	__checkReturn errno_t SetConfigBlocks(BYTE* pbData, DWORD cbData);
	BYTE* GetConfigBlocks();
	CHAR* GetMobileName(BYTE bType);
	CXeFlashFileSystemRoot* GetFS();
	__checkReturn errno_t CreateFileSystem();
	__checkReturn errno_t CreateDefaults(DWORD imgLen, DWORD pageLen, DWORD spareType, DWORD flashConfig, DWORD fsOffset);
	__checkReturn errno_t LoadImageFile(PSZ szPath);
	__checkReturn errno_t SaveImageFile(PSZ szPath);

	__checkReturn errno_t LoadFlashDevice();
	__checkReturn errno_t SaveFlashDevice();

	__checkReturn errno_t LoadContinue();
	__checkReturn errno_t SaveContinue();

	__checkReturn errno_t LoadFileSystems();
	__checkReturn errno_t SaveFileSystems();

	__checkReturn errno_t LoadBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey);
	__checkReturn errno_t LoadBootloaders();
	__checkReturn errno_t SaveBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey, int dwLimit);
	__checkReturn errno_t SaveBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey);
	__checkReturn errno_t SaveBootloaders();

	__checkReturn errno_t LoadKeyVaults();
	__checkReturn errno_t SaveKeyVaults();

	__checkReturn errno_t WriteImageIni(PSZ inipath);
	__checkReturn errno_t ReadImageIni(PSZ inipath);
	__checkReturn errno_t DumpFiles(PSZ path);
	__checkReturn errno_t DumpSMC(PSZ path);
	__checkReturn errno_t DumpKeyVaults(PSZ path);
	__checkReturn errno_t DumpBootloaders(PSZ path);

	__checkReturn errno_t Close();
};
#endif