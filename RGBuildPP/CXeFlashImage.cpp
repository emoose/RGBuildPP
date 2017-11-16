#include "stdafx.h"
#include "CXeFlashImage.h"
#include "util.h"
bool directoryExists(PSZ path)
{
	if(_access(path, 0) != 0)
		return false;
	struct stat status;
	stat(path, &status);
	if(!(status.st_mode & S_IFDIR))
		return false;
	return true;
}
errno_t CXeFlashImage::LoadFlashDevice()
{
	this->xeBlkDriver = *new CXeFlashBlockDriver();
	errno_t ret = this->xeBlkDriver.OpenDevice();
	if(ret != 0)
		return ret;
	return this->LoadContinue();
}
errno_t CXeFlashImage::LoadImageFile(PSZ szPath)
{
	this->xeBlkDriver = *new CXeFlashBlockDriver();
	errno_t ret = this->xeBlkDriver.OpenImage(szPath);
	if(ret != 0)
		return ret;

	return this->LoadContinue();
}
errno_t CXeFlashImage::SaveImageFile(PSZ szPath)
{
	this->SaveContinue();
	this->LoadContinue();
	Log(1, "saving image to file %s\n", szPath);
	return this->xeBlkDriver.SaveImage(szPath);
}
errno_t CXeFlashImage::SaveFlashDevice()
{
	this->SaveContinue();
	this->LoadContinue();
	Log(1, "saving image to NAND\n");
	this->xeBlkDriver.SaveDevice();
	return 0;
}
errno_t CXeFlashImage::SaveContinue()
{
	int nextldraddr = 0;
	// save header
	this->blFlash.Save(true);
	if(this->xeBlkDriver.Write(nextldraddr, this->blFlash.pbData, 0x200) == 0)
	{
		Log(3, "header write returned 0?\n");
		return 1;
	}
	// save SMC
	this->xeSMC.Save(true);
	if(this->xeBlkDriver.Write(this->blFlash.dwSmcAddr, this->xeSMC.pbData, this->blFlash.dwSmcLength) == 0)
	{
		Log(3, "SMC write returned 0?");
		return 1;
	}
	// save KeyVault
	this->SaveKeyVaults();

	// save bootloaders
	this->SaveBootloaders();

	// save filesystem/mobile data

	// save config block?
	return 0;
}
errno_t CXeFlashImage::LoadContinue()
{
	int nextldraddr = 0;
	// load header
	this->blFlash.cbData = 0x200;
	this->blFlash.pbData = (byte*)malloc(0x200);
	if(this->xeBlkDriver.Read(nextldraddr, this->blFlash.pbData, 0x200) == 0)
	{
		Log(3, "header read returned 0?\n");
		return 1;
	}
	this->blFlash.Load(false);

	// load smc
	this->xeSMC.cbData = this->blFlash.dwSmcLength;
	this->xeSMC.pbData = (byte*)malloc(this->xeSMC.cbData);
	if(this->xeBlkDriver.Read(this->blFlash.dwSmcAddr, this->xeSMC.pbData, this->xeSMC.cbData) == 0)
	{
		Log(3, "SMC read returned 0?\n");
		return 2;
	}
	this->xeSMC.Load(true);
	Log(0, "SMC board: %s\n", this->xeSMC.GetMobo());
	Log(0, "SMC version: 0x%x\n\n", bswap16(*this->xeSMC.pwVersion));
	
	this->LoadKeyVaults();
	this->LoadBootloaders();
	this->LoadFileSystems();

	// check if this is bb and we havent set fs block offset
	if(this->xeBlkDriver.dwFSOffset == 0 && this->xeBlkDriver.dwFlashConfig == 0xAA3020)
	{
		CXeFlashFileSystemRoot* fs = this->GetFS();
		if(fs == NULL)
			return 10;
		FLASHFILESYSTEM_ENTRY* pfsEntry = fs->FindEntry("bootanim.xex");
		if(pfsEntry == NULL)
			return 11;
		BYTE filebuff[4];
		filebuff[3] = 0;
		// try 256 first
		this->xeBlkDriver.ReadLilBlock(pfsEntry->wBlockNumber + (0x2E0), (BYTE*)&filebuff, 0x3);
		if(!strcmp((char*)filebuff, "XEX"))
		{
			this->xeBlkDriver.dwFSOffset = 0x2e0;
			return 0;
		}
		// now try 512
		this->xeBlkDriver.ReadLilBlock(pfsEntry->wBlockNumber + (0xAE0), (BYTE*)&filebuff, 0x3);
		if(!strcmp((char*)filebuff, "XEX"))
		{
			this->xeBlkDriver.dwFSOffset = 0xAe0;
			return 0;
		}
		return 12;
	}
	return 0;
}
BYTE* CXeFlashImage::GetConfigBlocks()
{
	BYTE * buffer = (BYTE*)malloc(4 * this->xeBlkDriver.dwBlockLength);
	for(int i = 0; i < 4; i++)
		this->xeBlkDriver.ReadBlock(i + this->xeBlkDriver.dwConfigBlockIdx, buffer + (i*this->xeBlkDriver.dwBlockLength), this->xeBlkDriver.dwBlockLength);
	return buffer;
}
CXeFlashFileSystemRoot* CXeFlashImage::GetFS()
{
	if(this->xecbFileSystems <= 0)
		return NULL;
	return &this->pxeFileSystems[xedwLatestFileSystem];
}
#pragma region section loading
int CXeFlashImage::LoadFileSystems()
{
	BYTE spare[0x10];
	this->xecbFileSystems = 0;
	this->pxeFileSystems = 0;
	DWORD fsBlks[0x77];
	for(DWORD i = 0; i < this->xeBlkDriver.dwLilBlockCount; i++)
	{
		this->xeBlkDriver.ReadLilBlockSpare(i, (BYTE*)&spare);
		BYTE blktype = this->xeBlkDriver.GetSpareBlockTypeField(spare);
		int seq = this->xeBlkDriver.GetSpareSeqField(spare);
		if(seq != 0 && ((blktype & 0x3F) == 0x2C || (blktype & 0x3F) == 0x30))
		{
			// got a filesystem?
			fsBlks[this->xecbFileSystems] = i;
			this->xecbFileSystems++;
		}
	}
	this->pxeFileSystems = (CXeFlashFileSystemRoot*)malloc(this->xecbFileSystems * sizeof(CXeFlashFileSystemRoot));
	// now loop through the found filesystems
	int latestver = -1;
	for(DWORD i = 0; i < this->xecbFileSystems; i++)
	{
		Log(-1, "found filesystem @ block 0x%x\n", fsBlks[i]);
		this->xeBlkDriver.ReadLilBlockSpare(fsBlks[i], (BYTE*)&spare);
		this->pxeFileSystems[i] = *new CXeFlashFileSystemRoot();
		this->pxeFileSystems[i].xepBlkDriver = &this->xeBlkDriver;
		this->pxeFileSystems[i].pbData = (BYTE*)malloc(this->xeBlkDriver.dwLilBlockLength);
		this->pxeFileSystems[i].cbData = this->xeBlkDriver.dwLilBlockLength;
		this->xeBlkDriver.ReadLilBlock(fsBlks[i], this->pxeFileSystems[i].pbData, this->xeBlkDriver.dwLilBlockLength);
		this->pxeFileSystems[i].Load();
		this->pxeFileSystems[i].dwVersion = this->xeBlkDriver.GetSpareSeqField(spare);
		if(this->pxeFileSystems[i].dwVersion > latestver)
		{
			this->xedwLatestFileSystem = i;
			latestver = this->pxeFileSystems[i].dwVersion;
		}
	}

	// now output latest fs info
	if(latestver > -1)
	{
		DbgPrint("\n");
		Log(1, "found latest filesystem @ block 0x%x\n", fsBlks[this->xedwLatestFileSystem]);
		CXeFlashFileSystemRoot root = this->pxeFileSystems[this->xedwLatestFileSystem];
		for(DWORD i = 0; i < root.fscbEntries; i++)
		{
			Log(0, "file: %s, blk 0x%x, length 0x%x\n", root.pfsEntries[i].cFileName, root.pfsEntries[i].wBlockNumber, root.pfsEntries[i].dwLength);
		}
		DbgPrint("\n");
	}

	Log(0, "loading mobile data pages...\n");
	// now to load mobile data
	DWORD mobilePages[256];
	BYTE mobilePageTypes[256];
	DWORD mobilePageVers[256];
	DWORD pageCount = 0;
	for(DWORD i = 0; i < this->xeBlkDriver.dwPageCount; i++)
	{
		this->xeBlkDriver.ReadPageSpare(i, (BYTE*)&spare);
		BYTE pgcount = this->xeBlkDriver.GetSparePageCountField((BYTE*)&spare);
		if(pgcount == 0)
			continue;
		BYTE type = this->xeBlkDriver.GetSpareBlockTypeField((BYTE*)&spare);
		// clear page count upper 2 bits
		type = type & 0x3F;
		if(!this->xeBlkDriver.IsMobileData(type))
			continue;
		mobilePages[pageCount] = i;
		mobilePageTypes[pageCount] = type;
		mobilePageVers[pageCount] = this->xeBlkDriver.GetSpareSeqField((BYTE*)&spare);
		pageCount++;
	}
	// now lets filter these pages into mobile "blocks"
	int realMobileCount = 0;
	DWORD realMobilePages[256];
	for(DWORD i = 0; i < pageCount; i++)
	{
		this->xeBlkDriver.ReadPageSpare(mobilePages[i], (BYTE*)&spare);
		WORD fssize = this->xeBlkDriver.GetSpareSizeField((BYTE*)&spare);
		BYTE pagcount = this->xeBlkDriver.GetSparePageCountField((BYTE*)&spare);
		int pgcount = fssize / this->xeBlkDriver.dwPageLength + (fssize%this->xeBlkDriver.dwPageLength > 0 ? 1 : 0);
		int pgcount2 = 0x20 - pagcount; // __ADAPTIVE__MIGHT NEED TO BE CHANGED
		if(pgcount != pgcount2 && pgcount == 1 && fssize >= 0x200)
			continue;

		int pagescalc = 0;
		for(DWORD y = i+1; y < pageCount; y++)
		{
			if(mobilePageTypes[y] != mobilePageTypes[i])
				continue;
			if(mobilePageVers[y] != mobilePageVers[i])
				continue;
			if(!(mobilePages[y] < (mobilePages[i] + pgcount)))
				continue;
			if(!(mobilePages[y] >= mobilePages[i]))
				continue;
			pagescalc++;
			if(pagescalc+1 == pgcount)
				break;
		}
		if(pagescalc+1 != pgcount)
			continue;

		realMobilePages[realMobileCount] = mobilePages[i];
		realMobileCount++;
		i += pagescalc;
	}

	// alloc our mobile pointer
	this->pxeMobileData = (FLASHMOBILEDATA*)malloc(realMobileCount * sizeof(FLASHMOBILEDATA));
	DWORD latestMobileData[9];
	for(int i = 0; i < realMobileCount; i++)
	{
		this->xeBlkDriver.ReadPageSpare(realMobilePages[i], (BYTE*)&spare);
		WORD fssize = this->xeBlkDriver.GetSpareSizeField((BYTE*)&spare);
		BYTE type = this->xeBlkDriver.GetSpareBlockTypeField((BYTE*)&spare) & 0x3F;
		DWORD seq = this->xeBlkDriver.GetSpareSeqField((BYTE*)&spare);
		memset((VOID*)&this->pxeMobileData[i], 0, sizeof(FLASHMOBILEDATA));

		this->pxeMobileData[i].dwDataSequence = seq;
		this->pxeMobileData[i].bDataType = type;
		this->pxeMobileData[i].dwPage = realMobilePages[i];
		this->pxeMobileData[i].cbData = fssize;
		this->pxeMobileData[i].pbData = (BYTE*)malloc(fssize);
		this->xeBlkDriver.Read(realMobilePages[i] * 0x200, this->pxeMobileData[i].pbData, fssize);
		int veridx = type & 0xF;
		veridx--;
		if(latestMobileData[veridx] < seq)
		{
			this->xedwLatestMobileData[veridx] = i;
			latestMobileData[veridx] = seq;
		}
		Log(0, "found mobile data type 0x%x @ page 0x%x, version %d, size 0x%x\n", type, realMobilePages[i], seq, fssize);
	}
	this->xecbMobileData = realMobileCount;
	return 0;
}
int CXeFlashImage::SaveKeyVaults()
{
	Log(0, "saving KeyVault...\n");
	bool altkv = this->xeKeyVault.xeData.b1AlternativeKeyVault != 0;
	this->xeKeyVault.Save(true);
	if(this->xeBlkDriver.Write(0x4000, (BYTE*)&this->xeKeyVault.xeData, 0x4000) == 0)
	{
		Log(3, "KV write returned 0?\n");
		return 1;
	}
	if(!altkv)
		return 0;
	Log(0, "saving alt KeyVault...\n");
	this->xeAltKeyVault.Save(true);
	if(this->xeBlkDriver.Write(0x8000, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000) == 0)
	{
		Log(3, "alt KV write returned 0?\n");
		return 1;
	}
	return 0;
}
int CXeFlashImage::LoadKeyVaults()
{
	Log(0, "loading KeyVault...\n");
	this->xeKeyVault.pwKeyVaultVersion = &this->blFlash.wKeyVaultVersion;
	this->xeKeyVault.pbCPUKey = this->pbCPUKey;
	if(this->xeBlkDriver.Read(0x4000, (BYTE*)&this->xeKeyVault.xeData, 0x4000) == 0)
	{
		Log(3, "KV read returned 0?\n");
		return 1;
	}
	errno_t ret = this->xeKeyVault.Load(true);
	if(ret != 0)
	{
		Log(3, "can't decrypt KeyVault?\n");
		return 1;
	}
	Log(0, "serial no: %.12s\n", this->xeKeyVault.xeData.sz14ConsoleSerialNumber);
	Log(0, "part no: %s\n", this->xeKeyVault.xeData.b36ConsoleCertificate.ConsolePartNumber);
	Log(0, "mfg date: %s\n", this->xeKeyVault.xeData.b36ConsoleCertificate.ManufacturingDate);
	Log(0, "privileges: 0x%x\n", this->xeKeyVault.xeData.b36ConsoleCertificate.Privileges);
	if(this->xeKeyVault.xeData.b1AlternativeKeyVault != 0)
	{
		Log(1, "loading alt KeyVault...\n");
		this->xeAltKeyVault.pwKeyVaultVersion = &this->blFlash.wKeyVaultVersion;
		this->xeAltKeyVault.pbCPUKey = this->pbCPUKey;
		if(this->xeBlkDriver.Read(0x8000, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000) == 0)
		{
			Log(3, "alt KV read returned 0?\n");
			return 2;
		}
		errno_t ret = this->xeAltKeyVault.Load(true);
		if(ret != 0)
		{
			Log(3, "can't decrypt alt KeyVault?\n");
			return 2;
		}
		Log(0, "serial no: %.12s\n", this->xeAltKeyVault.xeData.sz14ConsoleSerialNumber);
		Log(0, "part no: %s\n", this->xeAltKeyVault.xeData.b36ConsoleCertificate.ConsolePartNumber);
		Log(0, "mfg date: %s\n", this->xeAltKeyVault.xeData.b36ConsoleCertificate.ManufacturingDate);
		Log(0, "privileges: 0x%x\n", this->xeAltKeyVault.xeData.b36ConsoleCertificate.Privileges);
	}
	return 0;
}
errno_t CXeFlashImage::DumpKeyVaults(PSZ path)
{
	CHAR pathBuff[1024];
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	bool altkv = !this->xeKeyVault.bIsDecrypted ? FALSE : (this->xeKeyVault.xeData.b1AlternativeKeyVault != 0);
	Log(1, "dumping KeyVault (%s)...\n", this->xeKeyVault.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\KV%s.bin", path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));
	saveData((CHAR*)&pathBuff, (BYTE*)&this->xeKeyVault.xeData, 0x4000);

	this->xeKeyVault.Crypt(!this->xeKeyVault.bIsDecrypted);

	Log(1, "dumping KeyVault (%s)...\n", this->xeKeyVault.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\KV%s.bin", path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));
	saveData((CHAR*)&pathBuff, (BYTE*)&this->xeKeyVault.xeData, 0x4000);

	this->xeKeyVault.Crypt(!this->xeKeyVault.bIsDecrypted);

	if(!altkv)
		return 0;
	Log(1, "dumping alt KeyVault (%s)...\n", this->xeAltKeyVault.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\AltKV%s.bin", path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));
	saveData((CHAR*)&pathBuff, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000);

	this->xeAltKeyVault.Crypt(!this->xeAltKeyVault.bIsDecrypted);

	Log(1, "dumping alt KeyVault (%s)...\n", this->xeAltKeyVault.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\AltKV%s.bin", path, (this->xeAltKeyVault.bIsDecrypted ? "_dec" : ""));
	saveData((CHAR*)&pathBuff, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000);

	this->xeAltKeyVault.Crypt(!this->xeAltKeyVault.bIsDecrypted);
	return 0;
}

errno_t CXeFlashImage::DumpSMC(PSZ path)
{
	CHAR pathBuff[1024];
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	Log(1, "dumping SMC (%s)...\n", this->xeSMC.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\SMC%s.bin", path, (this->xeSMC.bIsDecrypted ? "_dec" : ""));
	saveData((CHAR*)&pathBuff, this->xeSMC.pbData, this->xeSMC.cbData);

	if(!this->xeSMC.bIsDecrypted)
		this->xeSMC.UnMunge();
	else
		this->xeSMC.Munge();

	Log(1, "dumping SMC (%s)...\n", this->xeSMC.bIsDecrypted ? "dec" : "enc");
	sprintf_s((char*)&pathBuff, 1024, "%s\\SMC%s.bin", path, this->xeSMC.bIsDecrypted ? "_dec" : "");
	saveData((char*)&pathBuff, this->xeSMC.pbData, this->xeSMC.cbData);

	if(!this->xeSMC.bIsDecrypted)
		this->xeSMC.UnMunge();
	else
		this->xeSMC.Munge();
	return 0;
}
CHAR* CXeFlashImage::GetMobileName(BYTE bType)
{
	CHAR name[0x8] = "MobileA";
	CHAR* name2 = (CHAR*)malloc(0x9);
	name[6] = bType + 0x11;
	memset((void*)name2, 0, 0x9);
	memcpy((void*)name2, (void*)&name, 0x8);
	return name2;
}
errno_t CXeFlashImage::DumpFiles(PSZ path)
{
	CHAR pathBuff[1024];
	CXeFlashFileSystemRoot* fs = this->GetFS();
	if(fs == NULL)
		return 1;
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	for(DWORD i = 0; i < fs->fscbEntries; i++)
	{
		if(fs->pfsEntries[i].cFileName[0] == '_')
			continue;
		Log(1, "dumping file %s (size:0x%x)\n", fs->pfsEntries[i].cFileName, fs->pfsEntries[i].dwLength);
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s", path, fs->pfsEntries[i].cFileName);
		BYTE* buffer = fs->FileGetData(&fs->pfsEntries[i]);
		if(buffer == NULL)
			return 1;
		saveData(pathBuff, buffer, fs->pfsEntries[i].dwLength);
		free(buffer);
		memset(&pathBuff, 0, 1024);
	}
	// now lets try dumping the mobile data
	for(DWORD i = 0; i < this->xecbMobileData; i++)
	{
		char* name = this->GetMobileName(pxeMobileData[i].bDataType);
		Log(1, "dumping mobile file %s (size:0x%x, pg:0x%x, seq:0x%x)\n", name, pxeMobileData[i].cbData, pxeMobileData[i].dwPage, pxeMobileData[i].dwDataSequence);
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%x.%x.bin", path, name, pxeMobileData[i].dwPage, pxeMobileData[i].dwDataSequence);
		saveData(pathBuff, pxeMobileData[i].pbData, pxeMobileData[i].cbData);
		memset(&pathBuff, 0, 1024);
		free((void*)name);
	}
	return 0;
}
errno_t CXeFlashImage::DumpBootloaders(PSZ path)
{
	CHAR pathBuff[1024];
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	if(this->bl2BL[0].isValid())
	{
		Log(1, "dumping 2BL_A...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s_A.%d.bin", path, (this->bl2BL[0].blHdr.wMagic & 0x1000) == 0x1000 ? "SB" : "CB", this->bl2BL[0].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl2BL[0].pbData, this->bl2BL[0].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl2BL[1].isValid())
	{
		Log(1, "dumping 2BL_B...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s_B.%d.bin", path, (this->bl2BL[1].blHdr.wMagic & 0x1000) == 0x1000 ? "SB" : "CB", this->bl2BL[1].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl2BL[1].pbData, this->bl2BL[1].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl3BL.isValid())
	{
		Log(1, "dumping 3BL...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl3BL.blHdr.wMagic & 0x1000) == 0x1000 ? "SC" : "CC", this->bl3BL.blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl3BL.pbData, this->bl3BL.blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl4BL.isValid())
	{
		Log(1, "dumping 4BL...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl4BL.blHdr.wMagic & 0x1000) == 0x1000 ? "SD" : "CD", this->bl4BL.blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl4BL.pbData, this->bl4BL.blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl5BL.isValid())
	{
		Log(1, "dumping 5BL...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl5BL.blHdr.wMagic & 0x1000) == 0x1000 ? "SE" : "CE", this->bl5BL.blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl5BL.pbData, this->bl5BL.blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl6BL[0].isValid())
	{
		Log(1, "dumping 6BL (slot0)...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl6BL[0].blHdr.wMagic & 0x1000) == 0x1000 ? "SF" : "CF", this->bl6BL[0].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl6BL[0].pbData, this->bl6BL[0].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl7BL[0].isValid())
	{
		Log(1, "dumping 7BL (slot0)...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl7BL[0].blHdr.wMagic & 0x1000) == 0x1000 ? "SG" : "CG", this->bl7BL[0].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl7BL[0].pbData, this->bl7BL[0].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl6BL[1].isValid())
	{
		Log(1, "dumping 6BL (slot1)...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl6BL[1].blHdr.wMagic & 0x1000) == 0x1000 ? "SF" : "CF", this->bl6BL[1].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl6BL[1].pbData, this->bl6BL[1].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	if(this->bl7BL[1].isValid())
	{
		Log(1, "dumping 7BL (slot1)...\n");
		sprintf_s((char*)&pathBuff, 1024, "%s\\%s.%d.bin", path, (this->bl7BL[1].blHdr.wMagic & 0x1000) == 0x1000 ? "SG" : "CG", this->bl7BL[1].blHdr.wBuild);
		saveData((char*)&pathBuff, this->bl7BL[1].pbData, this->bl7BL[1].blHdr.dwLength);
	}
	memset(pathBuff, 0, 1024);
	Log(1, "dumping config blocks...\n");
	sprintf_s((char*)&pathBuff, 1024, "%s\\smc_config.bin", path);
	BYTE* blkBuff = this->GetConfigBlocks();
	saveData((char*)&pathBuff, blkBuff, 4 * this->xeBlkDriver.dwBlockLength);
	free(blkBuff);
	return 0;
}
errno_t CXeFlashImage::SaveBootloaders()
{
	BYTE * prevkey = 0;
	int nextldraddr = this->blFlash.blHdr.dwEntrypoint;

	// init 2bl
	this->bl2BL[0].pbPrevBldrKey = this->pb1BLKey;
	this->bl2BL[0].Save(true);

	// write 2bl
	if(this->xeBlkDriver.Write(nextldraddr, this->bl2BL[0].pbData, this->bl2BL[0].blHdr.dwLength) == 0)
	{
		Log(3, "2BL write returned 0?\n");
		return 1;
	}

	prevkey = (BYTE*)&this->bl2BL[0].pbRc4Key;
	nextldraddr += this->bl2BL[0].blHdr.dwLength;
	// init 2bl_b if this is dual2bl nand
	if((this->bl2BL[0].blHdr.wFlags & XBOX_BLDR_FLAG_2BL_DUALLDR) == XBOX_BLDR_FLAG_2BL_DUALLDR)
	{
		// init 2bl_b
		this->bl2BL[1].pbPrevBldrKey = prevkey;
		this->bl2BL[1].pbCPUKey = this->pbCPUKey;
		this->bl2BL[1].Save(true);

		// write 2bl
		if(this->xeBlkDriver.Write(nextldraddr, this->bl2BL[1].pbData, this->bl2BL[1].blHdr.dwLength) == 0)
		{
			Log(3, "2BL_B write returned 0?\n");
			return 1;
		}
		prevkey = (BYTE*)&this->bl2BL[1].pbRc4Key;
		nextldraddr += this->bl2BL[1].blHdr.dwLength;
	}
	// init 3bl if this is a devkit nand
	if((this->bl2BL[0].blHdr.wMagic & XBOX_BLDR_MAGIC_DEVKIT) == XBOX_BLDR_MAGIC_DEVKIT)
	{
		// init 3bl
		this->bl3BL.pbPrevBldrKey = prevkey;
		this->bl3BL.pbCPUKey = this->pbCPUKey;
		this->bl3BL.Save(true);

		// write 3bl
		if(this->xeBlkDriver.Write(nextldraddr, this->bl3BL.pbData, this->bl3BL.blHdr.dwLength) == 0)
		{
			Log(3, "3BL write returned 0?\n");
			return 1;
		}
		prevkey = (BYTE*)&this->bl3BL.pbRc4Key;
		nextldraddr += this->bl3BL.blHdr.dwLength;
	}

	// init 4bl
	this->bl4BL.pbPrevBldrKey = prevkey;
	this->bl4BL.pbCPUKey = this->pbCPUKey;
	this->bl4BL.Save(true);

	// write 4bl
	if(this->xeBlkDriver.Write(nextldraddr, this->bl4BL.pbData, this->bl4BL.blHdr.dwLength) == 0)
	{
		Log(3, "4BL write returned 0?\n");
		return 1;
	}
	prevkey = (BYTE*)&this->bl4BL.pbRc4Key;
	nextldraddr += this->bl4BL.blHdr.dwLength;

	// init 5bl
	this->bl5BL.pbPrevBldrKey = prevkey;
	this->bl5BL.pbCPUKey = this->pbCPUKey;
	this->bl5BL.Save(true);

	// write 5bl
	if(this->xeBlkDriver.Write(nextldraddr, this->bl5BL.pbData, this->bl5BL.blHdr.dwLength) == 0)
	{
		Log(3, "5BL write returned 0?\n");
		return 1;
	}
	prevkey = (BYTE*)&this->bl5BL.pbRc4Key;
	nextldraddr += this->bl5BL.blHdr.dwLength;

	// FUCK THE 6BL AND 7BL FOR NOW, SERIAL GUYS, FUCK THAT SHIT

}
errno_t CXeFlashImage::LoadBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey)
{
	bldr->pbCPUKey = this->pbCPUKey;
	bldr->pbPrevBldrKey = *pbPrevKey;
	bldr->pbData = (byte*)malloc(0x10);
	if(this->xeBlkDriver.Read(*dwBlAddr, bldr->pbData, 0x10) != 0x10)
		return 1;
	
	bldr->LoadBldrHdr();
	free(bldr->pbData);
	if(!bldr->isValid())
		return 2;
	bldr->pbData = (byte*)malloc(bldr->blHdr.dwLength);
	if(this->xeBlkDriver.Read(*dwBlAddr, bldr->pbData, bldr->blHdr.dwLength) != bldr->blHdr.dwLength)
		return 3;
	bldr->Load(true);
	*dwBlAddr += bldr->blHdr.dwLength;
	*pbPrevKey = bldr->pbRc4Key;
	return 0;
}
errno_t CXeFlashImage::LoadBootloaders()
{
	BYTE * rc4Key = this->pb1BLKey;
	int ldrAddr = this->blFlash.blHdr.dwEntrypoint;
	// load 2BL_A
	this->LoadBootloader(&this->bl2BL[0], &ldrAddr, &rc4Key);
	if((this->bl2BL[0].blHdr.wFlags & XBOX_BLDR_FLAG_2BL_DUALLDR) == XBOX_BLDR_FLAG_2BL_DUALLDR)
	{
		// load 2BL_B if its dual 2bl
		this->LoadBootloader(&this->bl2BL[1], &ldrAddr, &rc4Key);
	}
	if((this->bl2BL[0].blHdr.wMagic & XBOX_BLDR_MAGIC_DEVKIT) == XBOX_BLDR_MAGIC_DEVKIT)
	{
		// load 3BL if devkit
		this->LoadBootloader(&this->bl3BL, &ldrAddr, &rc4Key);
	}
	// load 4BL
	this->LoadBootloader(&this->bl4BL, &ldrAddr, &rc4Key);
	// load 5BL/kernel
	this->LoadBootloader(&this->bl5BL, &ldrAddr, &rc4Key);
	ldrAddr = this->blFlash.dwSysUpdateAddr;
	for(int i = 0; i < 2; i++)
	{
		// load 6BL
		rc4Key = this->pb1BLKey;
		this->LoadBootloader(&this->bl6BL[i], &ldrAddr, &rc4Key);
		ldrAddr = this->blFlash.dwSysUpdateAddr + this->xeBlkDriver.dwPatchSlotLength;
	}
	ldrAddr = this->blFlash.dwSysUpdateAddr;
	for(int i = 0; i < 2; i++)
	{
		if(this->bl6BL[i].isValid())
		{
			ldrAddr += this->bl6BL[i].blHdr.dwLength;
			// check for 7bl
			this->bl7BL[i].pbPrevBldrKey = this->bl6BL[i].b7BLNonce;
			this->bl7BL[i].pbData = (byte*)malloc(0x10);
			if(this->xeBlkDriver.Read(ldrAddr, this->bl7BL[i].pbData, 0x10) == 0)
			{
				Log(3, "7BL hdr read returned 0?\n");
				break;
			}
			this->bl7BL[i].LoadBldrHdr();
			free(this->bl7BL[i].pbData);
			// load 7bl
			if(this->bl7BL[i].isValid())
			{
				this->bl7BL[i].pbData = (byte*)malloc(this->bl7BL[i].blHdr.dwLength);
				int hdrlen = (this->xeBlkDriver.dwPatchSlotLength - this->bl6BL[i].blHdr.dwLength);
				this->xeBlkDriver.Read(ldrAddr, this->bl7BL[i].pbData, hdrlen);
				int todo = this->bl7BL[i].blHdr.dwLength - hdrlen; 
				for(int y = 0; y < this->bl6BL[i].bl7BLPerBoxData.wUsedBlocksCount; y++)
				{
					int read = (y + 1 == this->bl6BL[i].bl7BLPerBoxData.wUsedBlocksCount ? todo % this->xeBlkDriver.dwBlockLength : this->xeBlkDriver.dwBlockLength);
					if(read==0)
						read = this->xeBlkDriver.dwBlockLength;
					WORD blkNum = this->bl6BL[i].bl7BLPerBoxData.wBlockNumbers[y];
					BYTE* copydest = (BYTE*)(this->bl7BL[i].pbData + hdrlen + (y * this->xeBlkDriver.dwBlockLength));
					this->xeBlkDriver.ReadLilBlock(blkNum, copydest, read);
				}
				this->bl7BL[i].Load(true);
			}
		}
		ldrAddr += this->xeBlkDriver.dwPatchSlotLength - this->bl6BL[i].blHdr.dwLength;
	}
	return 0;
}
#pragma endregion