// RGBuild++

#include "stdafx.h"
#include "CXeFlashImage.h"
#include "util.h"
#include "version.h"
#include "dirent.h"
#include "INIReader.h"

using namespace std;
static int loglvl = -1;
char dbgLog[131072];
char* lastLog;
void DbgPrint(const char* szFormat, ...)
{
	char szBuff[4096];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf_s(szBuff, sizeof(szBuff), szFormat, arg);
	va_end(arg);
	if(lastLog == 0)
		lastLog = (char*)&dbgLog;
	strcpy_s(lastLog, 4096, (char*)&szBuff);
	lastLog = lastLog + strlen(szBuff);
#ifndef _XBOX
	OutputDebugString(szBuff);
#endif
	printf(szBuff);
}
void Log(int priority, const char* szFormat, ...)
{
	if(priority < loglvl)
		return;
	char szFmtBuff[4096];
	memset(szFmtBuff, 0, 4096);
	switch(priority)
	{
	case -1:
		strcat_s(szFmtBuff, "\n** [DBG] ");
		break;
	case 0:
		strcat_s(szFmtBuff, "** [DBG] ");
		break;
	case 1:
		strcat_s(szFmtBuff, "*  [INF] ");
		break;
	case 2:
		strcat_s(szFmtBuff, "## [WRN] ");
		break;
	case 3:
		strcat_s(szFmtBuff, "!! [ERR] ");
		break;
	}
	strcat_s(szFmtBuff, szFormat);
	szFmtBuff[4095] = 0;
	char szBuff[4096];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf_s(szBuff, sizeof(szBuff), szFmtBuff, arg);
	va_end(arg);
#ifndef _XBOX
	OutputDebugString(szBuff);
#endif
	printf(szBuff);
}
__checkReturn errno_t saveDataf(const char* szPathFormat, BYTE * data, DWORD dwLength, ...)
{
	char szBuff[4096];
	va_list arg;
	va_start(arg, dwLength);
	_vsnprintf_s(szBuff, sizeof(szBuff), szPathFormat, arg);
	va_end(arg);
	return saveData(szBuff, data, dwLength);
}
#ifdef _XBOX
//--------------------------------------------------------------------------------------
// Name: MountDevice()
// Desc: Mounts the specified device to the chosen path
//--------------------------------------------------------------------------------------
HRESULT mountDevice( CHAR* szDevice, CHAR* szDrive)
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
HRESULT unmountDevice( CHAR* szDrive )
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

#if 0
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
	BOOL keygenned = CryptGenKey(hCryptProv, CALG_RSA_SIGN, (0x400 << 16) | CRYPT_EXPORTABLE, &key);
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
#endif


CXeFlashImage img;
#ifndef _DEBUG // release builds shouldn't have these keys
	byte b1blkey[0x10] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	byte bcpukey[0x10] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
#else
	byte b1blkey[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	byte bcpukey[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif


#ifndef _XBOX
/* WINDOWS RGBUILD */
int MainContinue(int argc, char* argv[])
{
	errno_t ret = 0;
	char* imagepath = 0;
	BOOL extractall = FALSE;
	BOOL createimg = FALSE;
	char* createini = 0;
	char* extractpath = 0;
	BOOL stripmode = FALSE;
	BOOL rc4mode = FALSE;
	byte rc4key[0x10];
	DWORD rc4offset = 0;

#ifndef _DEBUG
	loglvl = 1; // default log level to 1
#endif

	if(argc <= 1 || (argc == 2 && (!_stricmp(argv[argc-1], "/?") || !_stricmp(argv[argc-1], "-?"))))
	{
		// usage
		//TODO: map command
		DbgPrint("usage:\n");
		DbgPrint("RGBuildPP [/V] [/MAP] [/CPU KEY] [/1BL KEY] [/E PATH] [/C INIPATH] imagepath\n");
		DbgPrint("\n=RGBuild options\n");
		DbgPrint("Option\tParameters\tExplanation\n");
		DbgPrint("/?\t\t\tshow this usage\n");
		DbgPrint("/V\t\t\tverbose logging mode\n");
		DbgPrint("/CPU\tcpu_key\t\tset cpu key\n");
		DbgPrint("/1BL\t1bl_key\t\tset 1bl key\n");
		DbgPrint("/RC4\tkey offset\tdoes rc4 crypto on file starting from offset, using\n\t\t\t16-byte key (used for testing)\n\n");
		DbgPrint("/S\t\t\tstrips file of edc/spare data\n");
		DbgPrint("\n=Image options\n");
		DbgPrint("Option\tParameters\tExplanation\n");
		DbgPrint("/E\tpath\t\textract image\n");
		DbgPrint("/C\tini_path\tcreate image from ini\n");
		DbgPrint("/MAP\t\t\tprint map of image(!)\n");
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
		if(!_stricmp(argv[i], "/rc4") || !_stricmp(argv[i], "-rc4")) // rc4
		{
			Log(2, "rc4 crypt mode\n");
			rc4mode = TRUE;
			strToBytes(argv[i+1], (BYTE*)&rc4key, 0x10);
			char* end;
			rc4offset = strtol(argv[i+2], &end, 0);
			i+=2;
			continue;
		}
		if(!_stricmp(argv[i], "/s") || !_stricmp(argv[i], "-s")) // verbose
		{
			Log(2, "strip mode\n");
			stripmode = TRUE;
			continue;
		}
		if(!_stricmp(argv[i], "/c") || !_stricmp(argv[i], "-c")) // verbose
		{
			Log(2, "image creation mode\n");
			createimg = TRUE;
			createini = argv[i+1];
			extractall = FALSE;
			continue;
		}

		if(!createimg && (!_stricmp(argv[i], "/e") || !_stricmp(argv[i], "-e")))
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

	DbgPrint("\n");
	if(imagepath == 0 || strlen(imagepath) == 0)
	{
		Log(3, "no image specified\n");
		return 0;
	}
	else
		if(rc4mode)
		{
			BYTE* data;
			DWORD len;
			readData(imagepath, &data, &len);
			XeCryptRc4(rc4key, 0x10, data + rc4offset, len - rc4offset);
			//saveData("C:\\dec.bin", data, len);
			saveDataf("%s.dec", data, len, imagepath);
		}
		if(stripmode)
		{
			BYTE* data;
			DWORD len;
			readData(imagepath, &data, &len);
			char pathBuff[4096];
			sprintf_s(pathBuff, 4096, "%s.stripped.bin", imagepath);
//			FILE* file = fopen(pathBuff, "wb+");
			FILE* file;
			errno_t err = fopen_s(&file, pathBuff, "wb+");
			if(err != 0)
			{
				Log(3, "can't open %s for writing!\n", pathBuff);
				return 0;
			}
			for(DWORD i = 0; i < (len / 0x210); i++)
			{
				fwrite(data + (i*0x210), 0x1, 0x200, file);
			}
			fclose(file);
			return 0;
		}
		if(!createimg)
		{
			Log(2, "loading image %s\n", imagepath);
			if(ret = img.LoadImageFile(imagepath) != 0)
			{
				Log(3, "error loading image: %d\n", ret);
				return 0;
			}
		}
		else
		{
			Log(2, "creating image %s using %s\n", imagepath, createini);
			if(ret = img.ReadImageIni(createini) > 0)
			{
				Log(3, "error creating image: 0x%x", ret);
				return 0;
			}
			img.SaveImageFile(imagepath);
		}

	DbgPrint("\n");
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

		char filepath[255];
		sprintf_s(filepath, 255, "%s\\image.ini", extractpath);
		img.WriteImageIni(filepath);

		memset(filepath, 0, 255);
		sprintf_s(filepath, 255, "%s\\FileSystem\\", extractpath);
		img.DumpFiles(filepath);
		Log(1, "dump complete!\n");
	}
	return 0;
}
#else
/* XBOX RGBUILD */
int XboxContinue(int argc, char* argv[])
{
	errno_t ret;
	// mount all our devices
	CHAR szSourceDevice[64];
	for(int i = 0;i < sizeof(xDevices)/sizeof(X360_DEVICE); i++)
	{
		if(strcmp(xDevices[i].Drive, "game:") == 0)
		{
			xDevices[i].Success = TRUE;
			continue;
		}
		mountDevice(xDevices[i].Device, xDevices[i].Drive);
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
		memset(szSourceDevice, 0, 64);
	}
	DbgPrint("\n");
	Log(2, "loading image from system NAND\n");
	ret = img.LoadFlashDevice();
	
	HalReturnToFirmware(1);
	return ret;
}
#endif
string tempDir;
BOOL saveToFlash;
string savePath;
string bldrPath;
string buildPath;
string buildVer;
BOOL genKV;
string hackMethod;
string jtagMethod;
//-------------------------------------------------------------------------------------
// Name: main()
// Desc: The application's entry point
//-------------------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
	BOOL saveLog = FALSE;
	char logPath[4096];
	char cwd[4096];
	errno_t ret = 0;
	INIReader* reader = new INIReader("config.ini");
	
	// hack to make full build string appear in exe
	char* buildstr = (char*)&rgbBuildStrFull;

	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	strcpy_s(logPath, 4096, "RGBuildPPLog.txt");

	DbgPrint("\n**************************************************************\n");
	DbgPrint("*              RGBuild++ - the next generation!              *\n");
	DbgPrint("*          by stoker25, tydye81 and #RGLoader@EFnet          *\n");
	DbgPrint("*                                                            *\n");
	DbgPrint("*                                                            *\n"); // 60 spaces
	DbgPrint("*");
	int numspaces = 60 - 6 - strlen(RGB_VER_STR_TAG) - 1;
	for(int i = 0; i < numspaces; i++)
		DbgPrint(" ");
	DbgPrint("build %s *\n", RGB_VER_STR_TAG);
	//if(loglvl > 0)
	//	DbgPrint("*                                       build %s *\n", rgbBuildStr);
	//else
	//	DbgPrint("* build %s\n", rgbBuildStrFull);
	DbgPrint("**************************************************************\n\n");
	
	//6.2.8102.101.x86fre.winmain_win8m3.110830-1739.92eb4451821f0730

	DbgPrint("working directory:\n%s\n\n", _getcwd(cwd, 255));

#ifdef _DEBUG
	DbgPrint("===TODO===%s\n\n", RGB_TODO);
#endif

	if(reader->ParseError() == 0)
	{
		// TODO: rest of these settings
		//RGBuild settings
		loglvl = (reader->GetBoolean("RGBuild", "Verbose", TRUE) ? -1 : 1);
		savePath = reader->Get("RGBuild", "SavePath", "HDD:\\RGBImage.bin");
		saveLog = reader->GetBoolean("RGBuild", "SaveLogFile", TRUE);
		string path = reader->Get("RGBuild", "LogPath", "RGBuildPPLog.txt");
		strcpy_s(logPath, 4096, path.c_str());
		//Image settings
		string blkey = reader->Get("Image", "1BLKey", "00000000000000000000000000000000");
		string cpukey = reader->Get("Image", "CPUKey", "00000000000000000000000000000000");
		strToBytes((PSZ)blkey.c_str(), (BYTE*)&b1blkey, 0x10);
		strToBytes((PSZ)cpukey.c_str(), (BYTE*)&bcpukey, 0x10);
		genKV = reader->GetBoolean("Image", "GenerateDummyKV", FALSE);
		bldrPath = reader->Get("Image", "BootloadersPath", "Bootloaders\\");
		buildPath = reader->Get("Image", "BuildsPath", "Builds\\");
		buildVer = reader->Get("Image", "Kernel", "15574-dev");
		hackMethod = reader->Get("Image", "HackMethod", "Stock");
		jtagMethod = reader->Get("Image", "JTAGSMC", "AUD_CLAMP");
#ifdef _XBOX
		tempDir = reader->Get("RGBuild", "TempDir", ".\\");
		saveToFlash = reader->GetBoolean("RGBuild", "SaveToFlashDevice", FALSE);
#endif

	}
	img.pb1BLKey = (BYTE*)&b1blkey;
	img.pbCPUKey = (BYTE*)&bcpukey;

#ifndef _XBOX
	ret = MainContinue(argc, argv);
#else
	ret = XboxContinue(argc, argv);
#endif

	img.Close();

	if(_chdir(cwd) != 0)
		Log(3, "failed re-opening directory %s!\n", cwd);

	if(saveLog)
		saveData(logPath, (BYTE*)dbgLog, strlen(dbgLog));

#ifndef _XBOX
	if(loglvl <= 0)
	{
		Log(-1, "press enter to exit\n");
		getchar();
	}
#endif

    return 1;
}
