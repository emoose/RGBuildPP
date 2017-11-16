// RGBuild3.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "CXeFlashImage.h"
#include "util.h"
#include "version.h"
using namespace std;
static int loglvl = -1;
inline void Log(int priority, const char* szFormat, ...)
{
	if(priority < loglvl)
		return;
	char szFmtBuff[1024];
	memset(szFmtBuff, 0, 1024);
	switch(priority)
	{
	case -1:
		strcat_s(szFmtBuff, "\n** [DBG] ");
		break;
	case 0:
		strcat_s(szFmtBuff, "** [DBG] ");
		break;
	case 1:
		strcat_s(szFmtBuff, "* [INF] ");
		break;
	case 2:
		strcat_s(szFmtBuff, "## [WRN] ");
		break;
	case 3:
		strcat_s(szFmtBuff, "!! [ERR] ");
		break;
	}
	strcat_s(szFmtBuff, szFormat);
	char szBuff[1024];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf_s(szBuff, sizeof(szBuff), szFmtBuff, arg);
	va_end(arg);
#ifndef _XBOX
	OutputDebugString(szBuff);
#endif
	printf(szBuff);
}
#ifdef _XBOX
//--------------------------------------------------------------------------------------
// Name: MountDevice()
// Desc: Mounts the specified device to the chosen path
//--------------------------------------------------------------------------------------
HRESULT MountDevice( CHAR* szDevice, CHAR* szDrive)
{
	CHAR szDestinationDrive[16];
    sprintf_s( szDestinationDrive,"\\??\\%s", szDrive );

    ANSI_STRING DeviceName;
	RtlInitAnsiString(&DeviceName, szDevice);
	ANSI_STRING LinkName;
	RtlInitAnsiString(&LinkName, szDestinationDrive);

    return ( HRESULT )ObCreateSymbolicLink( &LinkName, &DeviceName );
}

//--------------------------------------------------------------------------------------
// Name: UnmountDevice()
// Desc: Unmounts the specified drive
//--------------------------------------------------------------------------------------
HRESULT UnmountDevice( CHAR* szDrive )
{
    CHAR szDestinationDrive[16];
    sprintf_s( szDestinationDrive,"\\??\\%s", szDrive );

    ANSI_STRING LinkName;
	RtlInitAnsiString(&LinkName, szDestinationDrive);

    return ( HRESULT )ObDeleteSymbolicLink( &LinkName );
}

X360_DEVICE xDevices[] = {
	{ "\\Device\\Harddisk0\\Partition1", "game:", TRUE },
	{ "\\Device\\Harddisk0\\Partition1", "hdd:", FALSE },
	{ "\\Device\\Cdrom0", "dvd:", FALSE },
	{ "\\Device\\Harddisk0\\Partition1\\DEVKIT", "e:", FALSE },
	{ "\\Device\\Harddisk0\\Partition1\\DEVKIT", "devkit:", FALSE },
	{ "\\Network", "u:", FALSE },
	{ "\\Network", "net:", FALSE },
	{ "\\Device\\Mu0", "mu0:", FALSE },
	{ "\\Device\\Mu1", "mu1:", FALSE },
	{ "\\Device\\BuiltInMuSfc", "muint:", FALSE },
	{ "\\Device\\Mass0", "usbmass0:", FALSE },
	{ "\\Device\\Mass1", "usbmass1:", FALSE },
	{ "\\Device\\Mass2", "usbmass2:", FALSE },
	{ "\\Device\\Mass0PartitionFile\\Storage", "usbmass0mu:", FALSE },
	{ "\\Device\\Mass1PartitionFile\\Storage", "usbmass1mu:", FALSE },
	{ "\\Device\\Mass2PartitionFile\\Storage", "usbmass2mu:", FALSE },
	{ "\\Device\\BuiltInMuUsb\\Storage", "intusb:", FALSE },
};
#endif
int fuck()
{
	HCRYPTPROV hCryptProv = NULL;
	LPCSTR UserName = "RGBuild++Container";

	if(CryptAcquireContext(
	   &hCryptProv,               // Handle to the CSP.
	   UserName,                  // Container name.
	   NULL,                      // Use the default provider.
	   PROV_RSA_FULL,             // Provider type.
	   0))                        // Flag values.
	{
		Log(0, "A crypto context with the %s key container \n", UserName);
		Log(0, "has been acquired.\n\n");
	}
	else
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
		   if(CryptAcquireContext(
			  &hCryptProv, 
			  UserName, 
			  NULL, 
			  PROV_RSA_FULL, 
			  CRYPT_NEWKEYSET)) 
			{
				Log(0, "A new key container has been created.\n");
			}
			else
			{
				Log(1, "Could not create a new key container.\n");
				return 1;
			}
		}
		else
		{
			Log(1, "A cryptographic service handle could not be acquired.\n");
			return 1;
		}
	}
	HCRYPTKEY key;
	bool keygenned = CryptGenKey(hCryptProv, CALG_RSA_SIGN, (0x400 << 16) | CRYPT_EXPORTABLE, &key);
	DWORD len = 0;
	DWORD len2 = 0;
	CryptExportKey(key, NULL, PRIVATEKEYBLOB, 0, NULL, &len);
	BYTE* privkey = (BYTE*)malloc(len);
	CryptExportKey(key, NULL, PRIVATEKEYBLOB, 0, privkey, &len);
	CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, NULL, &len2);
	BYTE* pubkey = (BYTE*)malloc(len2);
	CryptExportKey(key, NULL, PUBLICKEYBLOB, 0, pubkey, &len2);

	FILE* file = fopen("C:\\priv.bin", "wb+");
	fwrite(privkey, 1, len, file);
	fclose(file);
	file = fopen("C:\\pub.bin", "wb+");
	fwrite(pubkey, 1, len2, file);
	fclose(file);

	if (CryptReleaseContext(hCryptProv,0))
	{
	  printf("The handle has been freed.\n");
	}
	else
	{
	  printf("The handle could not be freed.\n");
	}
}
//-------------------------------------------------------------------------------------
// Name: main()
// Desc: The application's entry point
//-------------------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{

#if 0
	// kv gen sample code
	CXeKeyVault kv;
	FILE* filee = fopen("C:\\kv.bin", "rb+");
	fread((void*)&kv.xeData, 0x4000, 1, filee);
	fclose(filee);
	kv.RandomizeKeys();
	filee = fopen("C:\\kv_new.bin", "wb+");
	//fread((void*)&kv.xeData, 0x4000, 1, filee);
	fwrite((void*)&kv.xeData, 0x4000, 1, filee);
	fclose(filee);
#endif

	DbgPrint("\n**************************************************************\n");
	DbgPrint("*              RGBuild++ - the next generation!              *\n", RGB_VER);
	DbgPrint("*    version %d by stoker25, tydye81 and #RGLoader@EFnet    *\n", RGB_BLD);
	DbgPrint("*                                                            *\n");
	DbgPrint("*                                                %s BUILD *\n", RGB_PLT);
	DbgPrint("**************************************************************\n"); // \n
	DbgPrint("now with 150 percent more malloc-caused memory leaks!\n(which for some reason, we can't free...)\n\n");
#ifdef _DEBUG
	DbgPrint("TODO: %s\n\n", RGB_TODO);
#endif


	CXeFlashImage img;
	byte b1blkey[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	byte bcpukey[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	char* imagepath = 0;
	bool extractall = FALSE;
	char* extractpath = 0;

#ifndef _XBOX
	loglvl = 1; // no debug info
	if(argc <= 1 || (argc == 2 && (!_stricmp(argv[argc-1], "/?") || !_stricmp(argv[argc-1], "-?"))))
	{
		// usage
		DbgPrint("RGBuildPP [/V] [/CPU key] [/1BL key] [/E PATH] imagepath\n", RGB_VER);
		DbgPrint("/V - verbose mode\n");
		DbgPrint("/CPU - set cpukey\n");
		DbgPrint("/1BL - set 1blkey\n");
		DbgPrint("/E - extract all\n");
		return 0;
	}

	for(int i = 1; i < argc; i++)
	{
		if(!_stricmp(argv[i], "/v") || !_stricmp(argv[i], "-v")) // verbose
		{
			Log(2, "verbose mode\n");
			loglvl = -1;
			continue;
		}
		if(!_stricmp(argv[i], "/e") || !_stricmp(argv[i], "-e"))
		{
			Log(2, "extract mode\n");
			extractall = TRUE;
			extractpath = argv[i+1];
			Log(2, "extract path: %s\n", extractpath);
			i++;
			continue;
		}
		if(!_stricmp(argv[i], "/cpu") || !_stricmp(argv[i], "-cpu"))
		{
			strToBytes(argv[i+1], (BYTE*)&bcpukey, 0x10);
			Log(2, "set cpukey to %s\n", argv[i+1]);
			i++;
			continue;
		}
		if(!_stricmp(argv[i], "/1bl") || !_stricmp(argv[i], "-1bl"))
		{
			strToBytes(argv[i+1], (BYTE*)&b1blkey, 0x10);
			Log(2, "set 1blkey to %s\n", argv[i+1]);
			i++;
			continue;
		}
		imagepath = argv[i];
	}

#else
	// mount all our devices
	for(int i = 0;i < sizeof(xDevices)/sizeof(X360_DEVICE); i++)
	{
		if(strcmp(xDevices[i].Drive, "game:") == 0)
		{
			xDevices[i].Success = TRUE;
			continue;
		}
		MountDevice(xDevices[i].Device, xDevices[i].Drive);
		CHAR szSourceDevice[64];
		sprintf_s( szSourceDevice,"%s\\", xDevices[i].Drive );
		if(GetFileAttributes(szSourceDevice) != 0xFFFFFFFF)
		{
			Log(0, "mounted device %s\n", xDevices[i].Drive);
			xDevices[i].Success = TRUE;
		}
		else
		{
			Log(0, "failed to mount device %s\n", xDevices[i].Drive);
		}
	}
#endif

	DbgPrint("\n");
	img.pb1BLKey = (BYTE*)&b1blkey;
	img.pbCPUKey = (BYTE*)&bcpukey;
	errno_t ret = 0;
	if(imagepath == 0 || strlen(imagepath) == 0)
	{
#ifndef _XBOX
		Log(3, "no image specified\n");
		return 0;
#else
		Log(2, "Loading image from system NAND\n");
		ret = img.LoadFlashDevice();
#endif
	}
	else
	{
		// LOAD THE IMAGE
		Log(2, "loading image %s\n", imagepath);
		ret = img.LoadImageFile(imagepath);
	}

	if(ret != 0)
	{
		Log(3, "error loading image: %d\n", ret);
		return 0;
	}

	// test
	//img.blFlash.bCopyrightSign = 0x13;
	//img.blFlash.dwKeyVaultLength = 0x0730;
	//img.bl2BL[0].ulPostOutputAddr = 0x1337BEEFCAFE1337;
	//img.xeKeyVault.RepairDesKeys();
	//img.xeKeyVault.RandomizeKeys();
	//img.SaveImageFile("devkit:\\RGBuildPP\\test.bin");
	FILE* file = fopen("devkit:\\RGBuildPP\\kv_dec.bin", "rb+");
	fread((BYTE*)&img.xeKeyVault.xeData, 1, 0x4000, file);
	fclose(file);
	//FILE * file = fopen("devkit:\\RGBuildPP\\savetest.bin", "wb+");
	//fwrite(img.xeBlkDriver.pbImageData, 1, img.xeBlkDriver.dwImageLengthReal, file);
	//fclose(file);
	img.SaveFlashDevice();
	file = fopen("devkit:\\RGBuildPP\\savetest2.bin", "wb+");
	fwrite(img.xeBlkDriver.pbImageData, 1, img.xeBlkDriver.dwPageCount * 528, file);
	fclose(file);

	img.DumpKeyVaults("devkit:\\RGBuildPP");
	// /test
#ifdef _XBOX	
	HalReturnToFirmware(1);
#endif
	if(extractall)
	{
		if(extractpath == 0 || strlen(extractpath) == 0)
		{
			Log(3, "no extract path specified\n");
			return 0;
		}
		Log(1, "dumping all sections from nand...\n");
		img.DumpSMC(extractpath);
		img.DumpKeyVaults(extractpath);
		img.DumpBootloaders(extractpath);
		img.DumpFiles(extractpath);
		Log(1, "dump complete\n");
	}
#ifndef _XBOX
	if(loglvl <= 0)
	{
		Log(-1, "enter a number to exit\n");
		int i;
		cin >> i;
	}
#endif
    return 1;
}

