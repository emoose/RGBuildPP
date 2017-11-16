#ifndef _CXeFLASHIMAGE_H_
#define _CXeFLASHIMAGE_H_
#include "CXeBootloaders.h"
#include "CXeKeyVault.h"
#include "CXeFlashBlockDriver.h"
#include "CXeFlashFileSystem.h"

inline void Log(int priority, const char* szFormat, ...);

typedef struct _FLASHMOBILEDATA
{
	BYTE bDataType;
	DWORD dwDataSequence;
	DWORD dwPage;
	BYTE* pbData;
	DWORD cbData;
} FLASHMOBILEDATA, *PFLASHMOBILEDATA;
class CXeFlashImage
{
public:
	BYTE * pb1BLKey;
	BYTE * pbCPUKey;

	CXeBootloaderFlash blFlash;
	CSMC xeSMC;
	CXeKeyVault xeKeyVault;
	CXeKeyVault xeAltKeyVault;
	CXeBootloader2BL bl2BL[2];
	CXeBootloader3BL bl3BL;
	CXeBootloader4BL bl4BL;
	CXeBootloader5BL bl5BL;
	CXeBootloader6BL bl6BL[2];
	CXeBootloader7BL bl7BL[2];

	CXeFlashBlockDriver xeBlkDriver;
	CXeFlashFileSystemRoot * pxeFileSystems;
	DWORD xecbFileSystems;
	FLASHMOBILEDATA * pxeMobileData;
	DWORD xecbMobileData;
	DWORD xedwLatestMobileData[9]; // B/C/D/E/F/G/H/I/J
	DWORD xedwLatestFileSystem;

	BYTE* GetConfigBlocks();
	CHAR* GetMobileName(BYTE bType);
	CXeFlashFileSystemRoot* GetFS();

	errno_t LoadImageFile(PSZ szPath);
	errno_t SaveImageFile(PSZ szPath);

	errno_t LoadFlashDevice();
	errno_t SaveFlashDevice();

	errno_t LoadContinue();
	errno_t SaveContinue();

	errno_t LoadFileSystems();
	errno_t SaveFileSystems();

	errno_t LoadBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey);
	errno_t LoadBootloaders();
	errno_t SaveBootloaders();

	errno_t LoadKeyVaults();
	errno_t SaveKeyVaults();

	errno_t DumpFiles(PSZ path);
	errno_t DumpSMC(PSZ path);
	errno_t DumpKeyVaults(PSZ path);
	errno_t DumpBootloaders(PSZ path);
};
#endif