#include "stdafx.h"
#include "CXeFlashImage.h"
#include "util.h"
#include "INIReader.h"
#include "version.h"

// shhh... lets not leak our hard work
#if !RGB_VER_MIN
#include "rgbp.h"
#endif

using namespace std;
__checkReturn errno_t CXeFlashImage::CreateDefaults(DWORD imgLen, DWORD pageLen, DWORD spareType, DWORD flashConfig, DWORD fsOffset)
{
	this->xeBlkDriver = *new CXeFlashBlockDriver();
	this->xeBlkDriver.CreateDefaults(imgLen, pageLen, spareType, flashConfig, fsOffset);
	int ret = this->blFlash.CreateDefaults();
	if(ret != 0)
	{
		Log(3, "CXeFlashImage::CreateDefaults: flash header creation failed: 0x%x\n", ret);
		return ret;
	}
	ret = this->CreateFileSystem();
	// fix up 2bl
	this->bl2BL[0].pxeSMC = &this->xeSMC;
	this->bl2BL[1].pxeSMC = &this->xeSMC;
	return ret;
}
__checkReturn errno_t CXeFlashImage::LoadFlashDevice()
{
	this->xeBlkDriver = *new CXeFlashBlockDriver();
	errno_t ret = this->xeBlkDriver.OpenDevice();
	if(ret != 0)
		return ret;
	return this->LoadContinue();
}
__checkReturn errno_t CXeFlashImage::LoadImageFile(PSZ szPath)
{
	//this->xeBlkDriver = *new CXeFlashBlockDriver();
	errno_t ret = this->xeBlkDriver.OpenImage(szPath);
	if(ret != 0)
		return ret;

	return this->LoadContinue();
}
__checkReturn errno_t CXeFlashImage::SaveImageFile(PSZ szPath)
{
	this->SaveContinue();
	this->LoadContinue();
	Log(1, "CXeFlashImage::SaveImageFile: saving image to file %s\n", szPath);
	return this->xeBlkDriver.SaveImage(szPath);
}
__checkReturn errno_t CXeFlashImage::SaveFlashDevice()
{
	this->SaveContinue();
	this->LoadContinue();
	Log(1, "CXeFlashImage::SaveFlashDevice: saving image to NAND\n");
	this->xeBlkDriver.SaveDevice();
	return 0;
}
__checkReturn errno_t CXeFlashImage::SaveContinue()
{
	int nextldraddr = 0;
	int ret = 0;
	if(this->cbPayloads > 0)
	{
		// we have payloads to save!
		this->blFlash.wRgbpIndicator = 0x1337;
		BYTE* payloads = (BYTE*)this->blFlash.pbData + 0x84;
		for(DWORD i = 0; i < this->cbPayloads; i++)
		{
			this->xeBlkDriver.Write(this->xePayloads[i].dwOffset, this->xePayloads[i].pbData, this->xePayloads[i].cbData);
			this->xePayloads[i].dwOffset = bswap32(this->xePayloads[i].dwOffset);
			this->xePayloads[i].cbData = bswap32(this->xePayloads[i].cbData);
			this->xePayloads[i].bDescLength = strlen(this->xePayloads[i].szDescription);
			memcpy(payloads, &this->xePayloads[i], 0x9);
			this->xePayloads[i].dwOffset = bswap32(this->xePayloads[i].dwOffset);
			this->xePayloads[i].cbData = bswap32(this->xePayloads[i].cbData);
			memcpy(payloads + 0x9, this->xePayloads[i].szDescription, this->xePayloads[i].bDescLength);
			payloads += 9 + this->xePayloads[i].bDescLength;
			if(this->GetFS() != NULL)
			{
				int blkcount = this->xePayloads[i].cbData / this->xeBlkDriver.dwLilBlockLength + (this->xePayloads[i].cbData % this->xeBlkDriver.dwLilBlockLength > 0 ? 1 : 0);
				int startblk = this->xePayloads[i].dwOffset / this->xeBlkDriver.dwLilBlockLength;
				for(int y = 0; y < blkcount; y++)
					this->GetFS()->pwBlockMap[y + startblk] = 0x1ffb;
			}
		}
		*(DWORD*)payloads = 0xFFFFFFFF;
	}

	// save header
	this->blFlash.dwSmcLength = this->xeSMC.cbData;
	this->blFlash.Save(true);
	if(ret = this->xeBlkDriver.Write(nextldraddr, this->blFlash.pbData, 0x200) != 0x200)
	{
		Log(3, "CXeFlashImage::SaveContinue: header write: Write returned 0x%x, expected 0x200!\n", ret);
		return 1;
	}

	// save SMC
	this->xeSMC.Save(true);
	if(ret = this->xeBlkDriver.Write(this->blFlash.dwSmcAddr, this->xeSMC.pbData, this->xeSMC.cbData) != this->xeSMC.cbData)
	{
		Log(3, "CXeFlashImage::SaveContinue: SMC write: Write returned 0x%x, expected 0x200!\n", ret);
		return 1;
	}

	// save KeyVault
	if(this->blFlash.blHdr.wBuild >= 1838)
		this->SaveKeyVaults();
	else
	{
		this->pb1BLKey = 0;
		this->pbCPUKey = 0;
	}
	// save bootloaders
	this->SaveBootloaders();

	// save filesystem/mobile data
	if(this->blFlash.blHdr.wBuild >= 1838) // TODO: FATX for pre-1838
		this->SaveFileSystems();
	else
		Log(2, "image uses FATX filesystem, unsupported! (for now...)\n");

	return 0;
}
__checkReturn errno_t CXeFlashImage::LoadContinue()
{
	int nextldraddr = 0;
	this->cbPayloads = 0;
	// load header
	this->blFlash.cbData = 0x200;
	this->blFlash.pbData = (byte*)malloc(0x200);
	DWORD ret = 0;
	if(ret = this->xeBlkDriver.Read(nextldraddr, this->blFlash.pbData, 0x200) != 0x200)
	{
		Log(3, "CXeFlashImage::LoadContinue: header read: Read returned 0x%x, expected 0x200!\n", ret);
		return 1;
	}
	this->blFlash.Load(false);
	// null out the keys for < 1839
	if(this->blFlash.blHdr.wBuild < 1839)
	{
		memset(this->pb1BLKey, 0, 0x10);
		memset(this->pbCPUKey, 0, 0x10);
	}
	if(this->blFlash.wRgbpIndicator == 0x1337) // do we have any payloads?
	{
		BYTE* payloads = this->blFlash.pbData + 0x84;
		for(int i = 0; i < MAX_RGB_PAYLOADS; i++)
		{
			memcpy(&this->xePayloads[i], payloads, 0x9);
			if(this->xePayloads[i].dwOffset == 0xFFFFFFFF)
				break;
			this->xePayloads[i].dwOffset = bswap32(this->xePayloads[i].dwOffset);
			this->xePayloads[i].cbData = bswap32(this->xePayloads[i].cbData);
			this->xePayloads[i].szDescription = (char*)payloads + 0x9;
			payloads += (9 + this->xePayloads[i].bDescLength);
			// read its data
			this->xePayloads[i].pbData = (BYTE*)malloc(this->xePayloads[i].cbData);
			if(this->xeBlkDriver.Read(this->xePayloads[i].dwOffset, this->xePayloads[i].pbData, this->xePayloads[i].cbData) != this->xePayloads[i].cbData)
			{
				Log(3, "CXeFlashImage::LoadContinue: reading payload data for %s failed!\n", this->xePayloads[i].szDescription);
				break;
			}
			cbPayloads++;
		}
	}

	// load smc
	this->xeSMC.cbData = this->blFlash.dwSmcLength;
	this->xeSMC.pbData = (byte*)malloc(this->xeSMC.cbData);
	if(ret = this->xeBlkDriver.Read(this->blFlash.dwSmcAddr, this->xeSMC.pbData, this->xeSMC.cbData) != this->xeSMC.cbData)
	{
		Log(3, "CXeFlashImage::LoadContinue: SMC read: Read returned 0x%x, expected 0x%x!\n", ret, this->xeSMC.cbData);
		return 2;
	}
	this->xeSMC.Load(true);
	Log(0, "CXeFlashImage::LoadContinue: SMC board: %s\n", this->xeSMC.GetMobo());
	Log(0, "CXeFlashImage::LoadContinue: SMC version: 0x%x\n", bswap16(*this->xeSMC.pwVersion));
	
	if(this->blFlash.blHdr.wBuild >= 1838)
		this->LoadKeyVaults();
	else // this is a betakit nand, disable the keys
	{
		this->pb1BLKey = 0;
		this->pbCPUKey = 0;
	}
	this->LoadBootloaders();

	if(this->blFlash.blHdr.wBuild >= 1838) // TODO: FATX for pre-1838
		this->LoadFileSystems();
	else
		Log(2, "image uses FATX filesystem, unsupported! (for now...)\n");
	
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
__checkReturn errno_t CXeFlashImage::SetConfigBlocks(BYTE* pbData, DWORD cbData)
{
	for(int i = 0; i < 4; i++)
		this->xeBlkDriver.WriteBlock(i + this->xeBlkDriver.dwConfigBlockIdx, pbData + (i*this->xeBlkDriver.dwBlockLength), this->xeBlkDriver.dwBlockLength);
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
	return &this->pxeFileSystems[this->xedwLatestFileSystem];
}
#pragma region section loading
__checkReturn errno_t CXeFlashImage::SaveFileSystems()
{
	BYTE spare[0x10];
	if(this->GetFS() == NULL)
		return 1;
	for(DWORD i = 0; i < this->xecbMobileData; i++)
	{
		if(this->pxeMobileData[i].dwPage == 0)
		{
			WORD blkIdx = 0;
			if(!this->GetFS()->ChainAllocBlock(&blkIdx))
			{
				Log(3, "CXeFlashImage::SaveFileSystems: mobile write: couldn't allocate new block!\n");
				return 1;
			}
			this->GetFS()->pwBlockMap[blkIdx] = 0x1ffb;
			this->pxeMobileData[i].dwPage = blkIdx * (this->xeBlkDriver.dwLilBlockLengthReal / this->xeBlkDriver.dwPageLength);
		}
		if(this->xeBlkDriver.Write(this->pxeMobileData[i].dwPage * 0x200, this->pxeMobileData[i].pbData, this->pxeMobileData[i].cbData) != this->pxeMobileData[i].cbData)
		{
			Log(3, "CXeFlashImage::SaveFileSystems: mobile write: couldn't write 0x%x bytes\n", this->pxeMobileData[i].cbData);
			return 2;
		}
		// update spare data
		// this should use fs size / page count fields proper, as in check existing ones for space etc but we're lazy
		int pagecount = this->pxeMobileData[i].cbData / 0x200;
		if(pagecount < 1)
			pagecount = 1;
		for(int z = 0; z < pagecount; z++)
		{
			this->xeBlkDriver.ReadPageSpare(this->pxeMobileData[i].dwPage + z, spare);
			this->xeBlkDriver.SetSpareBlockTypeField(spare, this->pxeMobileData[i].bDataType);
			this->xeBlkDriver.SetSpareSeqField(spare, this->pxeMobileData[i].dwDataSequence);

			this->xeBlkDriver.SetSparePageCountField(spare, 0x20 - pagecount);
			this->xeBlkDriver.SetSpareSizeField(spare, pagecount * 0x200);
			// TODO: fix more spare fields?
			this->xeBlkDriver.WritePageSpare(this->pxeMobileData[i].dwPage + z, spare); 
		}
	}
	DbgPrint("\n");
	for(DWORD i = 0; i < this->xecbFileSystems; i++)
	{
		Log(1, "CXeFlashImage::SaveFileSystems: saving filesystem to block 0x%x\n", this->pxeFileSystems[i].wBlkIdx);
		this->pxeFileSystems[i].Save(this->pxeFileSystems[i].wBlkIdx);
	}

	if(!strcmp(this->xeSMC.GetMobo(), "Corona") && this->xeBlkDriver.dwSpareType == 3)
		for(DWORD i = 0; i < 2; i++)
			if(this->xeCoronaData[i].dwFSVersion > 0)
			{
				this->xeCoronaData[i].wFSBlockIdx = bswap16(this->xeCoronaData[i].wFSBlockIdx);
				this->xeCoronaData[i].dwFSVersion = bswap32(this->xeCoronaData[i].dwFSVersion);
			
				XeCryptSha((u8*)&this->xeCoronaData[i].dwUnknown, sizeof(XE_CORONA_FS_DATA) - 0x14, 0, 0, 0, 0, (u8*)&this->xeCoronaData[i].bSectionDigest, 0x14);
				this->xeBlkDriver.Write(0x2FE8000 + (0x4000 * i), (BYTE*)&this->xeCoronaData[i], sizeof(XE_CORONA_FS_DATA));
			}
	
	return 0;
}
__checkReturn errno_t CXeFlashImage::Close()
{
	if(this->blFlash.pbData != 0)
		free(this->blFlash.pbData);
	if(this->xeSMC.pbData != 0)
		free(this->xeSMC.pbData);
	if(this->bl2BL[0].pbData != 0)
		free(this->bl2BL[0].pbData);
	if(this->bl2BL[1].pbData != 0)
		free(this->bl2BL[1].pbData);
	if(this->bl3BL.pbData != 0)
		free(this->bl3BL.pbData);
	if(this->bl4BL.pbData != 0)
		free(this->bl4BL.pbData);
	if(this->bl5BL.pbData != 0)
		free(this->bl5BL.pbData);
	if(this->bl6BL[0].pbData != 0)
		free(this->bl6BL[0].pbData);
	if(this->bl6BL[1].pbData != 0)
		free(this->bl6BL[1].pbData);
	if(this->bl7BL[0].pbData != 0)
		free(this->bl7BL[0].pbData);
	if(this->bl7BL[1].pbData != 0)
		free(this->bl7BL[1].pbData);
	if(this->pxeFileSystems != 0)
	{
		for(DWORD i = 0; i < this->xecbFileSystems; i++)
		{
			if(this->pxeFileSystems[i].pfsEntries != 0)
				free(this->pxeFileSystems[i].pfsEntries);
			if(this->pxeFileSystems[i].pwBlockMap != 0)
				free(this->pxeFileSystems[i].pwBlockMap);
		}
		free(this->pxeFileSystems);
	}
	if(this->xecbMobileData != 0)
	{
		for(DWORD i = 0; i < this->xecbMobileData; i++)
			if(this->pxeMobileData[i].pbData != 0)
				free(this->pxeMobileData[i].pbData);
		free(this->pxeMobileData);
	}
	if(this->xeBlkDriver.pbImageData != 0)
		free(this->xeBlkDriver.pbImageData);
	return 0;
}
__checkReturn errno_t CXeFlashImage::CreateFileSystem()
{
	// allocate it a block if we have a latest fs
	CXeFlashFileSystemRoot newroot;
	newroot.xepBlkDriver = &this->xeBlkDriver;
	newroot.dwVersion = 3;
	newroot.wBlkIdx = 0x110;
	newroot.cbBlockMap = 0x1000;
	newroot.cfsEntries = 0;
	// allocate our filename/blockmap buffers
	newroot.pwBlockMap = (WORD*)malloc(this->xeBlkDriver.dwLilBlockLength / 2);
	newroot.pfsEntries = (FLASHFILESYSTEM_ENTRY*)malloc(this->xeBlkDriver.dwLilBlockLength / 2);
	memset(newroot.pwBlockMap, 0, this->xeBlkDriver.dwLilBlockLength / 2);
	memset(newroot.pfsEntries, 0, this->xeBlkDriver.dwLilBlockLength / 2);
	for(DWORD i = 0; i < this->xeBlkDriver.dwLilBlockCount; i++)
		newroot.pwBlockMap[i] = 0x1ffe;
	newroot.pwBlockMap[newroot.wBlkIdx] = 0x1fff;
	DWORD fsStart = (this->blFlash.dwSysUpdateAddr + (this->blFlash.dwFileSystemAddr * 2)) / this->xeBlkDriver.dwLilBlockLength;
	if((this->blFlash.dwSysUpdateAddr + (this->blFlash.dwFileSystemAddr * 2)) % this->xeBlkDriver.dwLilBlockLength > 0)
		fsStart++;

	for(DWORD i = 0; i < this->cbPayloads; i++)
	{
		int blkcount = this->xePayloads[i].cbData / this->xeBlkDriver.dwLilBlockLength + (this->xePayloads[i].cbData % this->xeBlkDriver.dwLilBlockLength > 0 ? 1 : 0);
		int startblk = this->xePayloads[i].dwOffset / this->xeBlkDriver.dwLilBlockLength;
		for(int y = 0; y < blkcount; y++)
			newroot.pwBlockMap[y + startblk] = 0x1ffb;
	}

	for(WORD i = 0; i < (WORD)fsStart; i++)
		newroot.pwBlockMap[i] = 0x1ffb;

	for(DWORD i = 0; i < this->xecbMobileData; i++)
		newroot.pwBlockMap[this->pxeMobileData[i].dwPage / 0x20] = 0x1ffb;

	for(int i = 0; i < 4; i++)
		newroot.pwBlockMap[i + this->xeBlkDriver.dwConfigBlockIdx] = 0x1ffb;

	CXeFlashFileSystemRoot* newroots = (CXeFlashFileSystemRoot*)malloc((this->xecbFileSystems + 1) * sizeof(CXeFlashFileSystemRoot));
	if(this->pxeFileSystems != 0 && this->GetFS() != NULL)
	{
		CXeFlashFileSystemRoot* latest = this->GetFS();
		if(!latest->ChainAllocBlock(&newroot.wBlkIdx))
		{
			Log(3, "CXeFlashImage::CreateFileSystem: failed to allocate a new block!\n");
			return -1;
		}
		newroot.dwVersion = latest->dwVersion + 1;
		// copy over old stuff
		memcpy(newroot.pwBlockMap, latest->pwBlockMap, this->xeBlkDriver.dwLilBlockLength / 2);
		memcpy(newroot.pfsEntries, latest->pfsEntries, this->xeBlkDriver.dwLilBlockLength / 2);
		newroot.cbBlockMap = latest->cbBlockMap;
		newroot.cfsEntries = latest->cfsEntries;
		memcpy(newroots, this->pxeFileSystems, (this->xecbFileSystems) * sizeof(CXeFlashFileSystemRoot));
	}
	memcpy(&newroots[this->xecbFileSystems], &newroot, sizeof(CXeFlashFileSystemRoot));
	this->xedwLatestFileSystem = this->xecbFileSystems;
	this->xecbFileSystems++;
	free(this->pxeFileSystems);
	this->pxeFileSystems = newroots;
	return this->xedwLatestFileSystem;
}
__checkReturn errno_t CXeFlashImage::LoadFileSystems()
{
	BYTE spare[0x10];
	this->xecbFileSystems = 0;
	this->pxeFileSystems = 0;
	WORD* fsBlks = (WORD*)malloc(0x77 * sizeof(WORD));
	if(!strcmp(this->xeSMC.GetMobo(), "Corona") && this->xeBlkDriver.dwSpareType == 3)
	{
		// TODO: fix this up in case only one of the slots are filled
		for(DWORD i = 0; i < 2; i++)
		{
			this->xeBlkDriver.Read(0x2FE8000 + (i*0x4000), (BYTE*)&this->xeCoronaData[i], sizeof(XE_CORONA_FS_DATA));
			this->xeCoronaData[i].wFSBlockIdx = bswap16(this->xeCoronaData[i].wFSBlockIdx);
			this->xeCoronaData[i].dwFSVersion = bswap32(this->xeCoronaData[i].dwFSVersion);
			fsBlks[i] = this->xeCoronaData[i].wFSBlockIdx;
		}
		this->xecbFileSystems = 2;
	}
	else
	{
		for(WORD i = 0; i < this->xeBlkDriver.dwLilBlockCount; i++)
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
	}
	this->pxeFileSystems = (CXeFlashFileSystemRoot*)malloc(this->xecbFileSystems * sizeof(CXeFlashFileSystemRoot));
	// now loop through the found filesystems
	int latestver = -1;
	for(DWORD i = 0; i < this->xecbFileSystems; i++)
	{
		Log(-1, "CXeFlashImage::LoadFileSystems: found filesystem at block 0x%x\n", fsBlks[i]);
		
		CXeFlashFileSystemRoot root;
		root.xepBlkDriver = &this->xeBlkDriver;
		root.wBlkIdx = fsBlks[i];
		if(i < 2 && this->xeCoronaData[i].dwFSVersion > 0)
			root.xepCoronaData = &this->xeCoronaData[i];
		root.Load(fsBlks[i]);
		if(root.dwVersion > latestver)
		{
			this->xedwLatestFileSystem = i;
			latestver = root.dwVersion;
		}
		memcpy(&this->pxeFileSystems[i], &root, sizeof(CXeFlashFileSystemRoot));
	}

	// now output latest fs info
	if(latestver > -1)
	{
		DbgPrint("\n");
		Log(1, "CXeFlashImage::LoadFileSystems: found latest filesystem at block 0x%x\n", fsBlks[this->xedwLatestFileSystem]);
		CXeFlashFileSystemRoot* root = this->GetFS();
		for(DWORD i = 0; i < root->cfsEntries; i++)
		{
			Log(0, "CXeFlashImage::LoadFileSystems: file: %s, blk 0x%x, length 0x%x\n", root->pfsEntries[i].cFileName, root->pfsEntries[i].wBlockNumber, root->pfsEntries[i].dwLength);
		}
		DbgPrint("\n");
	}
	free(fsBlks);
	Log(0, "CXeFlashImage::LoadFileSystems: loading mobile data pages...\n");
	// now to load mobile data
	// TODO: corona 4gb
	DWORD* mobilePages = (DWORD*)malloc(4096 * sizeof(DWORD));
	BYTE* mobilePageTypes = (BYTE*)malloc(4096);
	DWORD* mobilePageVers = (DWORD*)malloc(4096 * sizeof(DWORD));
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
	DWORD* realMobilePages = (DWORD*)malloc(4096 * sizeof(DWORD));
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
	free(mobilePageVers);
	free(mobilePageTypes);
	free(mobilePages);
	// alloc our mobile pointer
	this->pxeMobileData = (FLASHMOBILEDATA*)malloc(realMobileCount * sizeof(FLASHMOBILEDATA));
	DWORD latestMobileData[9];
	memset((void*)&latestMobileData, 0, sizeof(DWORD) * 9);
	for(int i = 0; i < realMobileCount; i++)
	{
		this->xeBlkDriver.ReadPageSpare(realMobilePages[i], (BYTE*)&spare);
		WORD fssize = this->xeBlkDriver.GetSpareSizeField((BYTE*)&spare);
		BYTE type = this->xeBlkDriver.GetSpareBlockTypeField((BYTE*)&spare) & 0x3F;
		DWORD seq = this->xeBlkDriver.GetSpareSeqField((BYTE*)&spare);
		WORD idx = this->xeBlkDriver.GetSpareIndexField((BYTE*)&spare);
		BYTE pgcount = this->xeBlkDriver.GetSparePageCountField((BYTE*)&spare);
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
		Log(0, "CXeFlashImage::LoadFileSystems: found mobile data type 0x%x @ page 0x%x, version %d, size 0x%x\n", type, realMobilePages[i], seq, fssize);
	}
	free(realMobilePages);
	this->xecbMobileData = realMobileCount;
	return 0;
}
__checkReturn errno_t CXeFlashImage::SaveKeyVaults()
{
	Log(0, "CXeFlashImage::SaveKeyVaults: saving KeyVault...\n");
	int ret = 0;
	BOOL altkv = this->xeKeyVault.xeData.b1AlternativeKeyVault != 0;
	this->xeKeyVault.Save(true);
	if(ret = this->xeBlkDriver.Write(0x4000, (BYTE*)&this->xeKeyVault.xeData, 0x4000) != 0x4000)
	{
		Log(3, "CXeFlashImage::SaveKeyVaults: KV write: Write returned 0x%x, expected 0x4000!\n", ret);
		return 1;
	}
	if(!altkv)
		return 0;
	Log(0, "CXeFlashImage::SaveKeyVaults: (alt) saving KeyVault...\n");
	this->xeAltKeyVault.Save(true);
	if(ret = this->xeBlkDriver.Write(0x8000, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000) == 0)
	{
		Log(3, "CXeFlashImage::SaveKeyVaults: (alt) KV write: Write returned 0x%x, expected 0x4000!\n", ret);
		return 1;
	}
	return 0;
}
__checkReturn errno_t CXeFlashImage::LoadKeyVaults()
{
	int ret = 0;
	Log(0, "CXeFlashImage::LoadKeyVaults: loading keyvault...\n");
	this->xeKeyVault.pwKeyVaultVersion = &this->blFlash.wKeyVaultVersion;
	this->xeKeyVault.pbCPUKey = this->pbCPUKey;
	if(ret = this->xeBlkDriver.Read(0x4000, (BYTE*)&this->xeKeyVault.xeData, 0x4000) != 0x4000)
	{
		Log(3, "CXeFlashImage::LoadKeyVaults: KV read: Read returned 0x%x, expected 0x4000!\n", ret);
		return 1;
	}
	if(ret = this->xeKeyVault.Load(true) != 0)
	{
		Log(3, "CXeFlashImage::LoadKeyVaults: KV read: can't decrypt keyvault: 0x%x, wrong cpukey?\n", ret);
		return 1;
	}
	Log(0, "CXeFlashImage::LoadKeyVaults: serial no: %.12s\n", this->xeKeyVault.xeData.sz14ConsoleSerialNumber);
	Log(0, "CXeFlashImage::LoadKeyVaults: part no: %s\n", this->xeKeyVault.xeData.b36ConsoleCertificate.ConsolePartNumber);
	Log(0, "CXeFlashImage::LoadKeyVaults: mfg date: %s\n", this->xeKeyVault.xeData.b36ConsoleCertificate.ManufacturingDate);
	Log(0, "CXeFlashImage::LoadKeyVaults: privileges: 0x%x\n", bswap16(this->xeKeyVault.xeData.b36ConsoleCertificate.Privileges));
	if(this->xeKeyVault.xeData.b1AlternativeKeyVault != 0)
	{
		Log(1, "CXeFlashImage::LoadKeyVaults: (alt) loading keyvault...\n");
		this->xeAltKeyVault.pwKeyVaultVersion = &this->blFlash.wKeyVaultVersion;
		this->xeAltKeyVault.pbCPUKey = this->pbCPUKey;
		if(ret = this->xeBlkDriver.Read(0x8000, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000) != 0x4000)
		{
			Log(3, "CXeFlashImage::LoadKeyVaults: (alt) KV read: Read returned 0x%x, expected 0x4000!\n", ret);
			return 2;
		}
		errno_t ret = this->xeAltKeyVault.Load(true);
		if(ret != 0)
		{
			Log(3, "CXeFlashImage::LoadKeyVaults: (alt) KV read: can't decrypt keyvault: 0x%x, wrong cpukey?\n", ret);
			return 2;
		}
		Log(0, "CXeFlashImage::LoadKeyVaults: (alt) serial no: %.12s\n", this->xeAltKeyVault.xeData.sz14ConsoleSerialNumber);
		Log(0, "CXeFlashImage::LoadKeyVaults: (alt) part no: %s\n", this->xeAltKeyVault.xeData.b36ConsoleCertificate.ConsolePartNumber);
		Log(0, "CXeFlashImage::LoadKeyVaults: (alt) mfg date: %s\n", this->xeAltKeyVault.xeData.b36ConsoleCertificate.ManufacturingDate);
		Log(0, "CXeFlashImage::LoadKeyVaults: (alt) privileges: 0x%x\n", bswap16(this->xeAltKeyVault.xeData.b36ConsoleCertificate.Privileges));
	}
	return 0;
}
__checkReturn errno_t CXeFlashImage::DumpKeyVaults(PSZ path)
{
	if(this->blFlash.blHdr.wBuild >= 1838)
	{
		//CHAR pathBuff[4096];
		if(!directoryExists(path))
			CreateDirectory(path, NULL);
		BOOL altkv = !this->xeKeyVault.bIsDecrypted ? false : (this->xeKeyVault.xeData.b1AlternativeKeyVault != 0);

		this->xeKeyVault.Save(!this->xeKeyVault.bIsDecrypted);
		Log(1, "CXeFlashImage::DumpKeyVaults: dumping KeyVault (%s)...\n", this->xeKeyVault.bIsDecrypted ? "dec" : "enc");
		//sprintf_s(pathBuff, 4096, "%s\\KV%s.bin", path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));
		//saveData(pathBuff, (BYTE*)&this->xeKeyVault.xeData, 0x4000);
		saveDataf("%s\\KV%s.bin", (BYTE*)&this->xeKeyVault.xeData, 0x4000, path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));

		this->xeKeyVault.Save(this->xeKeyVault.bIsDecrypted);

		Log(1, "CXeFlashImage::DumpKeyVaults: dumping KeyVault (%s)...\n", this->xeKeyVault.bIsDecrypted ? "dec" : "enc");
		//sprintf_s(pathBuff, 4096, "%s\\KV%s.bin", path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));
		//saveData(pathBuff, (BYTE*)&this->xeKeyVault.xeData, 0x4000);
		saveDataf("%s\\KV%s.bin", (BYTE*)&this->xeKeyVault.xeData, 0x4000, path, (this->xeKeyVault.bIsDecrypted ? "_dec" : ""));

		this->xeKeyVault.Save(!this->xeKeyVault.bIsDecrypted);

		if(!altkv)
			return 0;

		Log(1, "CXeFlashImage::DumpKeyVaults: (alt) dumping KeyVault (%s)...\n", this->xeAltKeyVault.bIsDecrypted ? "dec" : "enc");
		//sprintf_s(pathBuff, 4096, "%s\\AltKV%s.bin", path, (this->xeAltKeyVault.bIsDecrypted ? "_dec" : ""));
		//saveData(pathBuff, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000);
		saveDataf("%s\\AltKV%s.bin", (BYTE*)&this->xeAltKeyVault.xeData, 0x4000, path, (this->xeAltKeyVault.bIsDecrypted ? "_dec" : ""));

		this->xeAltKeyVault.Save(!this->xeAltKeyVault.bIsDecrypted);

		Log(1, "CXeFlashImage::DumpKeyVaults: (alt) dumping KeyVault (%s)...\n", this->xeAltKeyVault.bIsDecrypted ? "dec" : "enc");
		//sprintf_s(pathBuff, 4096, "%s\\AltKV%s.bin", path, (this->xeAltKeyVault.bIsDecrypted ? "_dec" : ""));
		//saveData(pathBuff, (BYTE*)&this->xeAltKeyVault.xeData, 0x4000);
		saveDataf("%s\\AltKV%s.bin", (BYTE*)&this->xeAltKeyVault.xeData, 0x4000, path, (this->xeAltKeyVault.bIsDecrypted ? "_dec" : ""));

		this->xeAltKeyVault.Save(!this->xeAltKeyVault.bIsDecrypted);
	}
	return 0;
}

__checkReturn errno_t CXeFlashImage::DumpSMC(PSZ path)
{
	//CHAR pathBuff[4096];
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	Log(1, "CXeFlashImage::DumpSMC: dumping SMC (%s)...\n", this->xeSMC.bIsDecrypted ? "dec" : "enc");
	//sprintf_s(pathBuff, 4096, "%s\\SMC%s.bin", path, (this->xeSMC.bIsDecrypted ? "_dec" : ""));
	//saveData(pathBuff, this->xeSMC.pbData, this->xeSMC.cbData);
	saveDataf("%s\\SMC%s.bin", this->xeSMC.pbData, this->xeSMC.cbData, path, (this->xeSMC.bIsDecrypted ? "_dec" : ""));

	if(!this->xeSMC.bIsDecrypted)
		this->xeSMC.UnMunge();
	else
		this->xeSMC.Munge();

	Log(1, "CXeFlashImage::DumpSMC: dumping SMC (%s)...\n", this->xeSMC.bIsDecrypted ? "dec" : "enc");
	//sprintf_s(pathBuff, 4096, "%s\\SMC%s.bin", path, this->xeSMC.bIsDecrypted ? "_dec" : "");
	//saveData(pathBuff, this->xeSMC.pbData, this->xeSMC.cbData);
	saveDataf("%s\\SMC%s.bin", this->xeSMC.pbData, this->xeSMC.cbData, path, (this->xeSMC.bIsDecrypted ? "_dec" : ""));

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
__checkReturn errno_t CXeFlashImage::DumpFiles(PSZ path)
{
	// TODO: dump payloads
	//CHAR pathBuff[4096];
	CXeFlashFileSystemRoot* fs = this->GetFS();
	if(fs == NULL)
		return 1;
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	for(DWORD i = 0; i < fs->cfsEntries; i++)
	{
		if(fs->pfsEntries[i].cFileName[0] == '_')
			continue;
		Log(1, "CXeFlashImage::DumpFiles: dumping file %s (size:0x%x)\n", fs->pfsEntries[i].cFileName, fs->pfsEntries[i].dwLength);
		BYTE* buffer = fs->FileGetData(&fs->pfsEntries[i]);
		if(buffer == NULL)
			return 1;
		//sprintf_s(pathBuff, 4096, "%s\\%s", path, fs->pfsEntries[i].cFileName);
		//saveData(pathBuff, buffer, fs->pfsEntries[i].dwLength);
		saveDataf("%s\\%s", buffer, fs->pfsEntries[i].dwLength, path, fs->pfsEntries[i].cFileName);
		free(buffer);
		// save meta data
		//sprintf_s(pathBuff, 4096, "%s\\%s.meta", path, fs->pfsEntries[i].cFileName);
		//saveData(pathBuff, (BYTE*)&fs->pfsEntries[i].dwTimeStamp, 4);
		saveDataf("%s\\%s.meta", (BYTE*)&fs->pfsEntries[i].dwTimeStamp, 4, path, fs->pfsEntries[i].cFileName);
	}
	// now lets try dumping the mobile data
	for(DWORD i = 0; i < 9; i++)
	{
		if(this->xedwLatestMobileData[i] <= -1)
			continue;
		DWORD idx = this->xedwLatestMobileData[i];
		FLASHMOBILEDATA* data = &pxeMobileData[idx];
		char* name = this->GetMobileName(pxeMobileData[idx].bDataType);
		Log(1, "CXeFlashImage::DumpFiles: dumping mobile file %s (size: 0x%x, pg: 0x%x, seq: 0x%x)\n", name, data->cbData, data->dwPage, data->dwDataSequence);
		//sprintf_s(pathBuff, 4096, "%s\\%s.bin", path, name);
		//saveData(pathBuff, pxeMobileData[idx].pbData, pxeMobileData[idx].cbData);
		saveDataf("%s\\%s.bin", data->pbData, data->cbData, path, name);
		free((void*)name); 
	}
	return 0;
}
__checkReturn errno_t CXeFlashImage::PayloadAddFile(PSZ szName, DWORD dwOffset, BYTE* pbData, DWORD cbData)
{
	if(this->cbPayloads >= 5)
	{
		Log(3, "CXeFlashImage::PayloadAddFile: can't add payload %s, already reached max %d entries!\n", szName, MAX_RGB_PAYLOADS);
		return 1;
	}
	this->xePayloads[this->cbPayloads].dwOffset = dwOffset;
	this->xePayloads[this->cbPayloads].cbData = cbData;
	this->xePayloads[this->cbPayloads].pbData = pbData;
	this->xePayloads[this->cbPayloads].szDescription = (char*)malloc(strlen(szName) + 1);
	memcpy(this->xePayloads[this->cbPayloads].szDescription, szName, strlen(szName) + 1);
	this->xePayloads[this->cbPayloads].bDescLength = strlen(szName);
	if(this->GetFS() != NULL)
	{ // make sure to allocate some blocks so nothing overwrites it
		// TODO: make sure this doesn't overwrite something else (ideally you should add payloads before any files)
		int blkcount = this->xePayloads[this->cbPayloads].cbData / this->xeBlkDriver.dwLilBlockLength + (this->xePayloads[this->cbPayloads].cbData % this->xeBlkDriver.dwLilBlockLength > 0 ? 1 : 0);
		int startblk = this->xePayloads[this->cbPayloads].dwOffset / this->xeBlkDriver.dwLilBlockLength;
		for(int y = 0; y < blkcount; y++)
			this->GetFS()->pwBlockMap[y + startblk] = 0x1ffb;
	}
	this->cbPayloads++;
	return 0;
}
__checkReturn errno_t CXeFlashImage::MobileAddFile(BYTE bType, FLASHMOBILEDATA** pfsEntry)
{
	FLASHMOBILEDATA* mob = (FLASHMOBILEDATA*)malloc((this->xecbMobileData + 1) * sizeof(FLASHMOBILEDATA));
	if(this->pxeMobileData != 0)
	{
		memcpy(mob, this->pxeMobileData, (this->xecbMobileData) * sizeof(FLASHMOBILEDATA));
		free(this->pxeMobileData);
	}
	this->pxeMobileData = mob;
	FLASHMOBILEDATA* newmob = &mob[this->xecbMobileData];
	*pfsEntry = newmob;
	newmob->bDataType = bType;
	newmob->dwDataSequence = this->xedwLatestMobileData[bType - 0x31] + 1;
	newmob->dwPage = 0;
	newmob->cbData = 0;
	newmob->pbData = 0;
	this->xecbMobileData++;
	return 0;
}
__checkReturn errno_t CXeFlashImage::ReadImageIni(PSZ inipath)
{
	// TODO: integrate the functions this uses from util.h into the class
	INIReader* reader = new INIReader(inipath);
	if(reader->ParseError() == 0)
	{
		errno_t ret = 0;
		// change our path to be relative to the files
		char* last = strrchr(inipath, '\\');
		last = last ? last+1 : inipath;
		char dir[255];
		memset(dir, 0, 255);
		memcpy(dir, inipath, (last - inipath));
		dir[254] = 0;
		if(_chdir(dir) != 0)
		{
			Log(3, "failed opening directory %s!\n", dir);
			return 22;
		}
		DWORD imgLength = reader->GetInteger("ImageConfig", "ImageLength", 0x1080000);
		DWORD pageLength = reader->GetInteger("ImageConfig", "PageLength", 0x210);
		DWORD spareType = reader->GetInteger("ImageConfig", "SpareType", 1);
		DWORD flashConfig = reader->GetInteger("ImageConfig", "FlashConfig", 0x23010);
		DWORD fsOffset = reader->GetInteger("ImageConfig", "FSOffset", 0);
		DWORD bl2Offset = reader->GetInteger("ImageConfig", "2BLOffset", 0x8000);
		DWORD bl7Offset = reader->GetInteger("ImageConfig", "6BLOffset", 0x70000);
		string copyright = reader->Get("ImageConfig", "CopyrightString", "2004-2010 Microsoft Corporation. All rights reserved.").c_str();
		string smcPath = reader->Get("ImageConfig", "SMC", "");
		string kvPath = reader->Get("ImageConfig", "KV", "");
		string altKvPath = reader->Get("ImageConfig", "AltKV", "");
		string mobileBPath = reader->Get("ImageConfig", "MobileB", "");
		string mobileCPath = reader->Get("ImageConfig", "MobileC", "");
		string mobileDPath = reader->Get("ImageConfig", "MobileD", "");
		string mobileEPath = reader->Get("ImageConfig", "MobileE", "");
		string mobileFPath = reader->Get("ImageConfig", "MobileF", "");
		string mobileGPath = reader->Get("ImageConfig", "MobileG", "");
		string mobileHPath = reader->Get("ImageConfig", "MobileH", "");
		string mobileIPath = reader->Get("ImageConfig", "MobileI", "");
		string mobileJPath = reader->Get("ImageConfig", "MobileJ", "");
		string configPath = reader->Get("ImageConfig", "ConfigBlock", "");
		const char* copyrightc = (const char*)copyright.c_str();
		Log(1, "formatting image...\n");
		if(ret = this->CreateDefaults(imgLength, pageLength, spareType, flashConfig, fsOffset) != 0)
		{
			Log(3, "failed to format image: 0x%x", ret);
			return 23;
		}
		this->blFlash.dwFileSystemAddr = this->xeBlkDriver.dwPatchSlotLength;
		this->blFlash.blHdr.dwEntrypoint = bl2Offset;
		this->blFlash.dwSysUpdateAddr = bl7Offset;
		// copy in the copyright string
		if(strlen(copyrightc) > 0)
			memcpy(&this->blFlash.bCopyright, copyrightc, strlen(copyrightc));

		if(strlen(smcPath.c_str()) > 0)
			if(ret = loadSMCFromFile(&this->xeSMC, (PSZ)smcPath.c_str()) != 0)
			{
				Log(3, "failed to add SMC %x: 0x%x", smcPath.c_str(), ret);
				return 24;
			}
				
		if(strlen(kvPath.c_str()) > 0)
			if(ret = loadKVFromFile(this, &this->xeKeyVault, (PSZ)kvPath.c_str()) != 0)
			{
				Log(3, "failed to add KV %x: 0x%x", kvPath.c_str(), ret);
				return 25;
			}
				
		if(strlen(altKvPath.c_str()) > 0)
			if(ret = loadKVFromFile(this, &this->xeAltKeyVault, (PSZ)altKvPath.c_str()) != 0)
			{
				Log(3, "failed to add KV %x: 0x%x", altKvPath.c_str(), ret);
				return 26;
			}

		// now lets add the fun stuff
		if(ret = loadBldrFromFile(this, &this->bl1BL, (PSZ)reader->Get("Bootloaders", "1BL_B", "").c_str()) != 0)
		{
			Log(3, "failed to add 1BL_B: 0x%x", ret);
			return 21;
		}

		if(ret = loadBldrFromFile(this, &this->bl2BL[0], (PSZ)reader->Get("Bootloaders", "2BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 2BL: 0x%x", ret);
			return 21;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl2BL[1], (PSZ)reader->Get("Bootloaders", "2BL_B", "").c_str()) != 0)
		{
			Log(3, "failed to add 2BL_B: 0x%x", ret);
			return 13;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl3BL, (PSZ)reader->Get("Bootloaders", "3BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 3BL: 0x%x", ret);
			return 14;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl4BL, (PSZ)reader->Get("Bootloaders", "4BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 4BL: 0x%x", ret);
			return 15;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl5BL, (PSZ)reader->Get("Bootloaders", "5BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 5BL: 0x%x", ret);
			return 16;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl6BL[0], (PSZ)reader->Get("Bootloaders", "6BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 6BL: 0x%x", ret);
			return 17;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl6BL[1], (PSZ)reader->Get("Bootloaders", "6BL_B", "").c_str()) != 0)
		{
			Log(3, "failed to add 6BL_B: 0x%x", ret);
			return 18;
		}
				
		if(ret = loadBldrFromFile(this, &this->bl7BL[0], (PSZ)reader->Get("Bootloaders", "7BL", "").c_str()) != 0)
		{
			Log(3, "failed to add 7BL: 0x%x", ret);
			return 19;
		}

		if(ret = loadBldrFromFile(this, &this->bl7BL[1], (PSZ)reader->Get("Bootloaders", "7BL_B", "").c_str()) != 0)
		{
			Log(3, "failed to add 7BL_B: 0x%x", ret);
			return 20;
		}
				
		Log(1, "adding mobile files...\n");
		if(strlen(mobileBPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileBPath.c_str(), 0) != 0)
			{
				Log(3, "failed to add MobileB: 0x%x", ret);
				return 2;
			}
				
		if(strlen(mobileCPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileCPath.c_str(), 1) != 0)
			{
				Log(3, "failed to add MobileC: 0x%x", ret);
				return 3;
			}
				
		if(strlen(mobileDPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileDPath.c_str(), 2) != 0)
			{
				Log(3, "failed to add MobileD: 0x%x", ret);
				return 4;
			}
				
		if(strlen(mobileEPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileEPath.c_str(), 3) != 0)
			{
				Log(3, "failed to add MobileE: 0x%x", ret);
				return 5;
			}
				
		if(strlen(mobileFPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileFPath.c_str(), 4) != 0)
			{
				Log(3, "failed to add MobileF: 0x%x", ret);
				return 6;
			}
				
		if(strlen(mobileGPath.c_str()) > 0)
			if(ret = loadMobileFromFile(this, (PSZ)mobileGPath.c_str(), 5) != 0)
			{
				Log(3, "failed to add MobileG: 0x%x", ret);
				return 7;
			}
				
		if(strlen(mobileHPath.c_str()) > 0)
			ret = loadMobileFromFile(this, (PSZ)mobileHPath.c_str(), 6);
			if(ret != 0)
			{
				Log(3, "failed to add MobileH: 0x%x", ret);
				return 8;
			}
				
		if(strlen(mobileIPath.c_str()) > 0)
			ret = loadMobileFromFile(this, (PSZ)mobileIPath.c_str(), 7);
			if(ret != 0)
			{
				Log(3, "failed to add MobileI: 0x%x", ret);
				return 9;
			}
				
		if(strlen(mobileJPath.c_str()) > 0)
			ret = loadMobileFromFile(this, (PSZ)mobileJPath.c_str(), 8);
			if(ret != 0)
			{
				Log(3, "failed to add MobileJ: 0x%x", ret);
				return 10;
			}

		// add payloads
		long plcount = reader->GetInteger("Payloads", "Count", 0);
		char buff[10];
		for(int i = 0; i < plcount; i++)
		{
			memset(buff, 0, 10);
			_itoa_s(i, (char*)&buff, 10, 10);
			string filename = reader->Get("Payloads", buff, "");
			char* name = (char*)filename.c_str();
			if(strlen(name) > 0)
			{
				char* ffname = strrchr(name, '~');
				if(!ffname)
				{
					Log(3, "invalid payload string %s, _must_ contain a ~!\n", name);
					continue;
				}
				ffname++;
				Log(1, "adding payload file %s...\n", ffname);
				BYTE* blData;
				DWORD len;
				if(ret = readData(ffname, &blData, &len) != 0)
				{
					Log(3, "cannot into filesystem file!!! %s\n", name);
					continue;
				}
				char* offset = (char*)malloc(ffname - name);
				memcpy(offset, name, ffname - name);
				char* fname = strrchr(ffname, '\\');
				fname = fname ? fname + 1 : ffname;
				char* end;
				// This parses "1234" (decimal) and also "0x4D2" (hex)
				DWORD offsset = strtol(offset, &end, 0);
				free(offset);
						
				char* ext = strrchr(fname, '.');
				if(ext)
					if(!_stricmp(ext, ".rglp"))
					{
						Log(1, "detected RGLP patchset\n");
						Log(1, "loading as new format (2BL_B+4BL+KHV)\n");
						DWORD off = 4;
						if(*(DWORD*)blData != 0xFFFFFFFF) // 2BL_B patch!
							this->bl2BL[1].PatchBootloader(blData, &off);
						DWORD soff = 4;
						if(*(DWORD*)(blData + off) != 0xFFFFFFFF) // 4BL patch!
							this->bl4BL.PatchBootloader(blData + off, &soff);
						// khv data
						BYTE* newdata = (BYTE*)malloc(len - off - soff);
						memcpy(newdata, blData + off + soff, len - off - soff);
						free(blData);
						blData = newdata;
						len = len - off - soff;
					}

				if(ret = this->PayloadAddFile(fname, offsset, blData, len) != 0)
				{
					Log(3, "failed to add payload %s: 0x%x", fname, ret);
					continue;
				}
			}
		}

		// add files
		long filecount = reader->GetInteger("Files", "Count", 0);
		char metabuff[4096];
		for(int i = 0; i < filecount; i++)
		{
			memset(buff, 0, 10);
			memset(metabuff, 0, 4096);
			_itoa_s(i, (char*)&buff, 10, 10);
			string filename = reader->Get("Files", buff, "");
			char* name = (char*)filename.c_str();
			if(strlen(name) > 0)
			{
				Log(1, "adding file %s...\n", name);
				char* base = strrchr(name, '\\');
				base = base ? base + 1 : name;
				sprintf_s(metabuff, 4096, "%s.meta", name);
				BYTE* blData;
				DWORD len;
				if(ret = readData(name, &blData, &len) != 0)
				{
					Log(3, "cannot into filesystem file!!! %s\n", name);
					continue;
				}
				FLASHFILESYSTEM_ENTRY* entr;
				if(ret = this->GetFS()->FileAdd(base, &entr) != 0)
				{
					Log(3, "failed to add file %s: 0x%x\n", name, ret);
					continue;
				}
				if(!this->GetFS()->FileSetData(entr, blData, len))
				{
					Log(3, "failed setting file %s data!\n", base);
					continue;
				}
				free(blData);
				BYTE* metaData;
				DWORD len2;
				if(ret = readData(metabuff, &metaData, &len2) == 0)
				{
					if(len2 != 4)
						Log(3, "invalid .meta file for %s!\n");
					else
						entr->dwTimeStamp = len2;
				}
				
			}
		}
		ret = loadConfigBlkFromFile(this, (PSZ)configPath.c_str());
		if(ret != 0)
		{
			Log(3, "failed to add config block: 0x%x\n", ret);
			return 11;
		}
	}
	else
	{
		Log(3, "error reading %s: 0x%x", inipath, reader->ParseError());
		return 1;
	}
	return 0;
}
__checkReturn errno_t CXeFlashImage::WriteImageIni(PSZ inipath)
{
	// create an ini file :D
	std::stringstream str;
	str << "[ImageConfig]\r\n";
	str << "ImageLength = 0x" << std::hex << this->xeBlkDriver.dwImageLengthReal << "\r\n";
	str << "PageLength = 0x" << std::hex << this->xeBlkDriver.dwPageLength << "\r\n";
	str << "SpareType = "  << this->xeBlkDriver.dwSpareType << "\r\n";
	str << "FlashConfig = 0x" << std::hex << this->xeBlkDriver.dwFlashConfig << "\r\n";
	str << "FSOffset = 0x" << std::hex << this->xeBlkDriver.dwFSOffset << "\r\n";
	str << "2BLOffset = 0x" << std::hex << this->blFlash.blHdr.dwEntrypoint << "\r\n";
	str << "6BLOffset = 0x" << std::hex << this->blFlash.dwSysUpdateAddr << "\r\n";
	str << "SMC = SMC_dec.bin\r\n";
	str << "KV = KV_dec.bin\r\n";
	if(this->xeAltKeyVault.bIsDecrypted)
		str << "KV = AltKV_dec.bin\r\n"; 
	if(this->xedwLatestMobileData[0] > -1)
		str << "MobileB = MobileB.bin\r\n";
	if(this->xedwLatestMobileData[1] > -1)
		str << "MobileC = MobileC.bin\r\n";
	if(this->xedwLatestMobileData[2] > -1)
		str << "MobileD = MobileD.bin\r\n";
	if(this->xedwLatestMobileData[3] > -1)
		str << "MobileE = MobileE.bin\r\n";
	if(this->xedwLatestMobileData[4] > -1)
		str << "MobileF = MobileF.bin\r\n";
	if(this->xedwLatestMobileData[5] > -1)
		str << "MobileG = MobileG.bin\r\n";
	if(this->xedwLatestMobileData[6] > -1)
		str << "MobileH = MobileH.bin\r\n";
	if(this->xedwLatestMobileData[7] > -1)
		str << "MobileI = MobileI.bin\r\n";
	if(this->xedwLatestMobileData[8] > -1)
		str << "MobileJ = MobileJ.bin\r\n";
	str << "ConfigBlock = smc_config.bin\r\n";
	str << "CopyrightString = ";
	str << this->blFlash.bCopyright + 1;
	str << "\r\n\r\n";
	str << "[Bootloaders]\r\n";

	// TODO: fix for beta kits

	char bl = 'C';
	if(this->bl2BL[0].isValid() && (this->bl2BL[0].blHdr.wMagic & 0x1000) == 0x1000)
		bl = 'S';
	if(this->bl1BL.isValid())
		str << "1BL_B = 1BL_B." << std::dec << this->bl1BL.blHdr.wBuild << ".bin\r\n";
	if(this->bl2BL[0].isValid())
		str << "2BL = " << bl << "B_A." << std::dec << this->bl2BL[0].blHdr.wBuild << ".bin\r\n";
	if(this->bl2BL[1].isValid())
		str << "2BL_B = " << bl << "B_B." << std::dec << this->bl2BL[1].blHdr.wBuild << ".bin\r\n";
	if(this->bl3BL.isValid())
		str << "3BL = " << bl << "C." << std::dec << this->bl3BL.blHdr.wBuild << ".bin\r\n";
	if(this->bl4BL.isValid())
		str << "4BL = " << bl << "D." << std::dec << this->bl4BL.blHdr.wBuild << ".bin\r\n";
	if(this->bl5BL.isValid())
		str << "5BL = " << bl << "E." << std::dec << this->bl5BL.blHdr.wBuild << ".bin\r\n";
	if(this->bl6BL[0].isValid())
		str << "6BL = " << bl << "F." << std::dec << this->bl6BL[0].blHdr.wBuild << ".bin\r\n";
	if(this->bl6BL[1].isValid())
		str << "6BL_B = " << bl << "F." << std::dec << this->bl6BL[1].blHdr.wBuild << ".bin\r\n";
	if(this->bl7BL[0].isValid())
		str << "7BL = " << bl << "F." << std::dec << this->bl7BL[0].blHdr.wBuild << ".bin\r\n";
	if(this->bl7BL[1].isValid())
		str << "7BL_B = " << bl << "F." << std::dec << this->bl7BL[1].blHdr.wBuild << ".bin\r\n";

	if(this->GetFS() != NULL)
	{
		str << "\r\n[Files]\r\n";
		str << "Count = " << std::dec << this->GetFS()->cfsEntries << "\r\n";
		for(DWORD i = 0; i < this->GetFS()->cfsEntries; i++) // TODO: make the path changable somehow
			str << std::dec << i << " = FileSystem\\" << this->GetFS()->pfsEntries[i].cFileName << "\r\n";
	}
	str << "\r\n";

	//TODO: payloads
	std::string s = str.str();
	const char* p = s.c_str();
	saveData(inipath, (BYTE*)p, strlen(p));
	return 0;
}
__checkReturn errno_t CXeFlashImage::DumpBootloaders(PSZ path)
{
	if(!directoryExists(path))
		CreateDirectory(path, NULL);
	if(this->bl1BL.isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 1BL_B...\n");
		saveDataf("%s\\1BL_B.%d.bin", this->bl1BL.pbData, this->bl1BL.blHdr.dwLength, path, this->bl1BL.blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\1BL_B.%d.bin", path, this->bl1BL.blHdr.wBuild);
		//saveData(pathBuff, this->bl1BL.pbData, this->bl1BL.blHdr.dwLength);
	}
	if(this->bl2BL[0].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 2BL_A...\n");
		saveDataf("%s\\%s_A.%d.bin", this->bl2BL[0].pbData, this->bl2BL[0].blHdr.dwLength, path, ((this->bl2BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SB" : "CB", this->bl2BL[0].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s_A.%d.bin", path, ((this->bl2BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SB" : "CB", this->bl2BL[0].blHdr.wBuild);
		//saveData(pathBuff, this->bl2BL[0].pbData, this->bl2BL[0].blHdr.dwLength);
	}
	if(this->bl2BL[1].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 2BL_B...\n");
		saveDataf("%s\\%s_B.%d.bin", this->bl2BL[1].pbData, this->bl2BL[1].blHdr.dwLength, path, ((this->bl2BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SB" : "CB", this->bl2BL[1].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s_B.%d.bin", path, ((this->bl2BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SB" : "CB", this->bl2BL[1].blHdr.wBuild);
		//saveData(pathBuff, this->bl2BL[1].pbData, this->bl2BL[1].blHdr.dwLength);
	}
	if(this->bl3BL.isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 3BL...\n");
		saveDataf("%s\\%s.%d.bin", this->bl3BL.pbData, this->bl3BL.blHdr.dwLength, path, ((this->bl3BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SC" : "CC", this->bl3BL.blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl3BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SC" : "CC", this->bl3BL.blHdr.wBuild);
		//saveData(pathBuff, this->bl3BL.pbData, this->bl3BL.blHdr.dwLength);
	}
	if(this->bl4BL.isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 4BL...\n");
		saveDataf("%s\\%s.%d.bin", this->bl4BL.pbData, this->bl4BL.blHdr.dwLength, path, ((this->bl4BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SD" : "CD", this->bl4BL.blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl4BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SD" : "CD", this->bl4BL.blHdr.wBuild);
		//saveData(pathBuff, this->bl4BL.pbData, this->bl4BL.blHdr.dwLength);
	}
	if(this->bl5BL.isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 5BL...\n");
		saveDataf("%s\\%s.%d.bin", this->bl5BL.pbData, this->bl5BL.blHdr.dwLength, path, ((this->bl5BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SE" : "CE", this->bl5BL.blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl5BL.blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SE" : "CE", this->bl5BL.blHdr.wBuild);
		//saveData(pathBuff, this->bl5BL.pbData, this->bl5BL.blHdr.dwLength);
	}
	if(this->bl6BL[0].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 6BL (slot0)...\n");
		saveDataf("%s\\%s.%d.bin", this->bl6BL[0].pbData, this->bl6BL[0].blHdr.dwLength, path, ((this->bl6BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SF" : "CF", this->bl6BL[0].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl6BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SF" : "CF", this->bl6BL[0].blHdr.wBuild);
		//saveData(pathBuff, this->bl6BL[0].pbData, this->bl6BL[0].blHdr.dwLength);
	}
	if(this->bl7BL[0].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 7BL (slot0)...\n");
		saveDataf("%s\\%s.%d.bin", this->bl7BL[0].pbData, this->bl7BL[0].blHdr.dwLength, path, ((this->bl7BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SG" : "CG", this->bl7BL[0].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl7BL[0].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SG" : "CG", this->bl7BL[0].blHdr.wBuild);
		//saveData(pathBuff, this->bl7BL[0].pbData, this->bl7BL[0].blHdr.dwLength);
	}
	if(this->bl6BL[1].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 6BL (slot1)...\n");
		saveDataf("%s\\%s.%d.bin", this->bl6BL[1].pbData, this->bl6BL[1].blHdr.dwLength, path, ((this->bl6BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SF" : "CF", this->bl6BL[1].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl6BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SF" : "CF", this->bl6BL[1].blHdr.wBuild);
		//saveData(pathBuff, this->bl6BL[1].pbData, this->bl6BL[1].blHdr.dwLength);
	}
	if(this->bl7BL[1].isValid())
	{
		Log(1, "CXeFlashImage::DumpBootloaders: dumping 7BL (slot1)...\n");
		saveDataf("%s\\%s.%d.bin", this->bl7BL[1].pbData, this->bl7BL[1].blHdr.dwLength, path, ((this->bl7BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SG" : "CG", this->bl7BL[1].blHdr.wBuild);
		//sprintf_s(pathBuff, 4096, "%s\\%s.%d.bin", path, ((this->bl7BL[1].blHdr.wMagic & 0x1000) == 0x1000 || this->pbCPUKey == 0) ? "SG" : "CG", this->bl7BL[1].blHdr.wBuild);
		//saveData(pathBuff, this->bl7BL[1].pbData, this->bl7BL[1].blHdr.dwLength);
	}
	Log(1, "CXeFlashImage::DumpBootloaders: dumping config blocks...\n");
	//sprintf_s(pathBuff, 4096, "%s\\smc_config.bin", path);
	BYTE* blkBuff = this->GetConfigBlocks();
	saveDataf("%s\\smc_config.bin", blkBuff, 4 * this->xeBlkDriver.dwBlockLength, path);
	//saveData(pathBuff, blkBuff, 4 * this->xeBlkDriver.dwBlockLength);
	free(blkBuff);
	return 0;
}
__checkReturn errno_t CXeFlashImage::SaveBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey, int dwWriteLimit)
{
	Log(-1, "saving bootloader to 0x%x...\n", *dwBlAddr);
	bldr->pbCPUKey = this->pbCPUKey;
	bldr->pbPrevBldrKey = *pbPrevKey;
	bldr->Save(true);
	// write bldr
	if(this->xeBlkDriver.Write(*dwBlAddr, bldr->pbData, (dwWriteLimit > 0 ? dwWriteLimit : bldr->blHdr.dwLength)) == 0)
		return 1;
	*pbPrevKey = bldr->bRc4Key;
	*dwBlAddr += bldr->blHdr.dwLength;
	return 0;
}
__checkReturn errno_t CXeFlashImage::SaveBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey)
{
	return this->SaveBootloader(bldr, dwBlAddr, pbPrevKey, 0);
}
__checkReturn errno_t CXeFlashImage::LoadBootloader(CXeBootloader* bldr, int* dwBlAddr, byte** pbPrevKey)
{
	Log(-1, "loading bootloader from 0x%x...\n", *dwBlAddr);
	bldr->pbCPUKey = this->pbCPUKey;
	bldr->pbPrevBldrKey = *pbPrevKey;
	bldr->pbData = (byte*)malloc(0x10);
	if(this->xeBlkDriver.Read(*dwBlAddr, bldr->pbData, 0x10) != 0x10)
	{
		free(bldr->pbData);
		bldr->pbData = 0;
		return 1;
	}
	
	bldr->LoadBldrHdr();
	free(bldr->pbData);
	bldr->pbData = 0;
	if(!bldr->isValid())
		return 2;
	DWORD reallen = ((bldr->blHdr.dwLength + 0xF) >> 4) << 4;
	bldr->pbData = (byte*)malloc(reallen);
	if(this->xeBlkDriver.Read(*dwBlAddr, bldr->pbData, reallen) != reallen)
	{
		free(bldr->pbData);
		bldr->pbData = 0;
		return 3;
	}
	bldr->Load(true);
	*dwBlAddr += reallen;
	*pbPrevKey = bldr->bRc4Key;
	return 0;
}
__checkReturn errno_t CXeFlashImage::SaveBootloaders()
{
	BYTE * prevkey = this->pb1BLKey;
	int nextldraddr = this->blFlash.blHdr.dwEntrypoint;
	errno_t ret;

	// is this a beta kit nand?
	if(this->blFlash.blHdr.wBuild < 1838) 
	{
		// load 1BL
		nextldraddr = 0x80; // 1BL in beta nand starts here, code at 0x100 in image (0x80 in loader)
		if(ret = this->SaveBootloader(&this->bl1BL, &nextldraddr, &prevkey) != 0)
		{
			Log(3, "CXeFlashImage::SaveBootloaders: 1BL write: SaveBootloader returned %d, expected 0!\n", ret);
			return 1;
		}
	}
	else
	{
		this->bl2BL[0].pbPrevBldrKey = prevkey;
		this->bl2BL[0].Crypt();
		this->bl2BL[0].Crypt();
		if(!this->bl2BL[1].isValid())
			this->bl2BL[0].FixPerBoxDigest();
	}
	if(ret = this->SaveBootloader(&this->bl2BL[0], &nextldraddr, &prevkey) != 0)
	{
		Log(3, "CXeFlashImage::SaveBootloaders: 2BL write: SaveBootloader returned %d, expected 0!\n", ret);
		return 1;
	}

	// save 2bl_b if this is dual2bl nand
	if((this->bl2BL[0].blHdr.wFlags & XBOX_BLDR_FLAG_2BL_DUALLDR) == XBOX_BLDR_FLAG_2BL_DUALLDR && this->bl2BL[1].isValid() && this->pbCPUKey != 0 && this->bl2BL[0].blHdr.wBuild > 1888)
	{
		this->bl2BL[1].pbPrevBldrKey = prevkey;
		this->bl2BL[1].Crypt();
		this->bl2BL[1].Crypt();
		this->bl2BL[1].FixPerBoxDigest();
		if(ret = this->SaveBootloader(&this->bl2BL[1], &nextldraddr, &prevkey) != 0)
		{
			Log(3, "CXeFlashImage::SaveBootloaders: 2BL_B write: SaveBootloader returned %d, expected 0!\n", ret);
			return 2;
		}
	}

	// save 3bl if this is a devkit nand
	if((this->bl2BL[0].blHdr.wMagic & XBOX_BLDR_MAGIC_DEVKIT) == XBOX_BLDR_MAGIC_DEVKIT)
	{
		if(ret = this->SaveBootloader(&this->bl3BL, &nextldraddr, &prevkey) != 0)
		{
			Log(3, "CXeFlashImage::SaveBootloaders: 3BL write: SaveBootloader returned %d, expected 0!\n", ret);
			return 3;
		}
	}

#ifdef RGBPH
	if(!rbl(&this->bl4BL))
		Log(3, "CXeFlashImage::SaveBootloaders: rbl failed?\n");
#endif

	if(this->bl2BL[0].blHdr.wBuild < 1746)
		this->bl4BL.pbBetaKitNonce = this->bl2BL[0].pbData + 0x20;

	if(ret = this->SaveBootloader(&this->bl4BL, &nextldraddr, &prevkey) != 0)
	{
		Log(3, "CXeFlashImage::SaveBootloaders: 4BL write: SaveBootloader returned %d, expected 0!\n", ret);
		return 4;
	}

	if(ret = this->SaveBootloader(&this->bl5BL, &nextldraddr, &prevkey) != 0)
	{
		Log(3, "CXeFlashImage::SaveBootloaders: 5BL write: SaveBootloader returned %d, expected 0!\n", ret);
		return 5;
	}

	nextldraddr = this->blFlash.dwSysUpdateAddr;
	BYTE nonce7BL[32];
	for(int i = 0; i < 2; i++)
	{
		if(this->bl6BL[i].isValid())
		{
			// save 6BL
			prevkey = this->pb1BLKey;
			if(this->bl7BL[i].isValid() && this->GetFS() != NULL)
			{
				int spaceleft = this->xeBlkDriver.dwPatchSlotLength - this->bl6BL[i].blHdr.dwLength;
				char* filename = "sysupdate.xexp1";
				if(i == 1)
					filename = "sysupdate.xexp2";
				FLASHFILESYSTEM_ENTRY* ent = this->GetFS()->FileSearch(filename);
				if(ent == NULL)
					if(ret = this->GetFS()->FileAdd(filename, &ent) != 0)
					{
						Log(3, "CXeFlashImage::SaveBootloaders: 6BL write: couldn't create sysupdate file: 0x%x\n", ret);
						return 6;
					}
				byte* buffer = (byte*)malloc(this->bl7BL[i].blHdr.dwLength - spaceleft);
				this->GetFS()->FileSetData(ent, buffer, this->bl7BL[i].blHdr.dwLength - spaceleft);
				free((void*)buffer);
				WORD chainlength = 0;
				WORD* chain = this->GetFS()->ChainGetChain(ent->wBlockNumber, &chainlength);
				this->bl6BL[i].bl7BLPerBoxData.wUsedBlocksCount = chainlength;
				for(int y = 0; y < chainlength; y++)
					this->bl6BL[i].bl7BLPerBoxData.wBlockNumbers[y] = chain[y];
				free(chain);
			}
			if((this->bl6BL[i].bl6BLPerBoxData.bPairingData[0] | this->bl6BL[i].bl6BLPerBoxData.bPairingData[1] | this->bl6BL[i].bl6BLPerBoxData.bPairingData[2]) != 0)
			{
				this->bl6BL[i].pbPrevBldrKey = prevkey;
				this->bl6BL[i].Crypt();
				this->bl6BL[i].Crypt();
			
				// dirty rehash
				this->bl6BL[i].FixPerBoxDigest();
			}
			else
			{
				memset(&this->bl6BL[i].bl6BLPerBoxData.bPerBoxDigest, 0, 16);
				this->bl6BL[i].bl6BLPerBoxData.bLockDownValue = 0;
			}
			memcpy(nonce7BL + (i*0x10), this->bl6BL[i].pb7BLNonce, 0x10);
			if(ret = this->SaveBootloader(&this->bl6BL[i], &nextldraddr, &prevkey) != 0)
			{
				Log(3, "CXeFlashImage::SaveBootloaders: 6BL write: SaveBootloader returned %d, expected 0!\n", ret);
				return 7;
			}
		}
		nextldraddr = this->blFlash.dwSysUpdateAddr + this->xeBlkDriver.dwPatchSlotLength;
	}

	nextldraddr = this->blFlash.dwSysUpdateAddr;
	for(int i = 0; i < 2; i++)
	{
		if(this->bl7BL[i].isValid() && this->bl6BL[i].isValid() && this->GetFS() != NULL)
		{
			prevkey = (BYTE*)&nonce7BL + (i*0x10);
			nextldraddr += this->bl6BL[i].blHdr.dwLength;
			int spaceleft = this->xeBlkDriver.dwPatchSlotLength - this->bl6BL[i].blHdr.dwLength;
			if(ret = this->SaveBootloader(&this->bl7BL[i], &nextldraddr, &prevkey, spaceleft) != 0)
			{
				Log(3, "CXeFlashImage::SaveBootloaders: 7BL write: SaveBootloader returned %d, expected 0!\n", ret);
				return 8;
			}

			// now lets set the sysupdate.xexp* file data to the rest of 7BL
			char* filename = "sysupdate.xexp1";
			if(i == 1)
				filename = "sysupdate.xexp2";
			FLASHFILESYSTEM_ENTRY* ent = this->GetFS()->FileSearch(filename);
			if(ent == NULL)
			{
				Log(3, "CXeFlashImage::SaveBootloaders: 7BL write: couldn't find sysupdate.xexp entry!\n");
				return 9;
			}
			if(!this->GetFS()->FileSetData(ent, this->bl7BL[i].pbData + spaceleft, this->bl7BL[i].blHdr.dwLength - spaceleft))
			{
				Log(3, "CXeFlashImage::SaveBootloaders: 7BL write: failed to set entry data?\n");
				return 10;
			}
		}
		nextldraddr = this->blFlash.dwSysUpdateAddr + this->xeBlkDriver.dwPatchSlotLength;
	}

	return 0;
}
__checkReturn errno_t CXeFlashImage::LoadBootloaders()
{
	BYTE * rc4Key = this->pb1BLKey;
	int ldrAddr = this->blFlash.blHdr.dwEntrypoint;
	errno_t ret;
	// is this a beta kit nand?
	if(this->blFlash.blHdr.wBuild < 1838) 
	{
		// load 1BL
		ldrAddr = 0x80; // 1BL in beta nand starts here, code at 0x100 in image (0x80 in loader)
		if(ret = this->LoadBootloader(&this->bl1BL, &ldrAddr, &rc4Key) != 0)
		{
			Log(3, "CXeFlashImage::LoadBootloaders: 1BL read: LoadBootloader returned %d, expected 0!\n", ret);
			return 1;
		}
	}
	// load 2BL_A
	if(ret = this->LoadBootloader(&this->bl2BL[0], &ldrAddr, &rc4Key) != 0)
	{
		Log(3, "CXeFlashImage::LoadBootloaders: 2BL read: LoadBootloader returned %d, expected 0!\n", ret);
		return 2;
	}
	this->bl2BL[0].pxeSMC = &this->xeSMC;

	// load 2BL_B if its dual 2bl nand and it's running a newer build than 1888
	if((this->bl2BL[0].blHdr.wFlags & XBOX_BLDR_FLAG_2BL_DUALLDR) == XBOX_BLDR_FLAG_2BL_DUALLDR && this->pbCPUKey != 0 && this->bl2BL[0].blHdr.wBuild > 1888)
	{   
		if(ret = this->LoadBootloader(&this->bl2BL[1], &ldrAddr, &rc4Key) != 0)
		{
			Log(3, "CXeFlashImage::LoadBootloaders: 2BL_B read: LoadBootloader returned %d, expected 0!\n", ret);
			return 0x12;
		}
		this->bl2BL[1].pxeSMC = &this->xeSMC;
	}

	// load 3BL if devkit
	if((this->bl2BL[0].blHdr.wMagic & XBOX_BLDR_MAGIC_DEVKIT) == XBOX_BLDR_MAGIC_DEVKIT)
	{
		rc4Key = (this->bl2BL[1].isValid() ? this->bl2BL[1].pb3BLNonce : this->bl2BL[0].pb3BLNonce);
		if(ret = this->LoadBootloader(&this->bl3BL, &ldrAddr, &rc4Key) != 0)
		{
			Log(3, "CXeFlashImage::LoadBootloaders: 3BL read: LoadBootloader returned %d, expected 0!\n", ret);
			return 3;
		}
	}
	if(this->bl2BL[0].blHdr.wBuild < 1746)
		this->bl4BL.pbBetaKitNonce = this->bl2BL[0].pbData + 0x20;
	// load 4BL
	if(ret = this->LoadBootloader(&this->bl4BL, &ldrAddr, &rc4Key) != 0)
	{
		Log(3, "CXeFlashImage::LoadBootloaders: 4BL read: LoadBootloader returned %d, expected 0!\n", ret);
		return 4;
	}

	// load 5BL/kernel
	if(ret = this->LoadBootloader(&this->bl5BL, &ldrAddr, &rc4Key) != 0)
	{
		Log(3, "CXeFlashImage::LoadBootloaders: 5BL read: LoadBootloader returned %d, expected 0!\n", ret);
		return 5;
	}

	ldrAddr = this->blFlash.dwSysUpdateAddr;
	if(ldrAddr > 0)
	{
		for(int i = 0; i < 2; i++)
		{
			// load 6BL
			rc4Key = this->pb1BLKey;
			ret = this->LoadBootloader(&this->bl6BL[i], &ldrAddr, &rc4Key);
			if(ret != 0 && ret != 2)
			{
				Log(3, "CXeFlashImage::LoadBootloaders: 6BL_%d read: LoadBootloader returned %d, expected 0 or 2!\n", (i+1), ret);
				return 6;
			}
			ldrAddr = this->blFlash.dwSysUpdateAddr + this->xeBlkDriver.dwPatchSlotLength;
		}
		ldrAddr = this->blFlash.dwSysUpdateAddr;
		for(int i = 0; i < 2; i++)
		{
			if(this->bl6BL[i].isValid())
			{
				ldrAddr += this->bl6BL[i].blHdr.dwLength;
				// check for 7bl
				this->bl7BL[i].pbPrevBldrKey = this->bl6BL[i].pb7BLNonce;
				this->bl7BL[i].pbData = (byte*)malloc(0x10);
				if(this->xeBlkDriver.Read(ldrAddr, this->bl7BL[i].pbData, 0x10) != 0x10)
				{
					free(this->bl7BL[i].pbData);
					this->bl7BL[i].pbData = 0;
					Log(3, "CXeFlashImage::LoadBootloaders: 7BL read: header read didn't return 0x10\n");
					return 7;
				}
				this->bl7BL[i].LoadBldrHdr();
				free(this->bl7BL[i].pbData);
				this->bl7BL[i].pbData = 0;
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
	}
	return 0;
}
#pragma endregion