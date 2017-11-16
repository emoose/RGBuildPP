#include "stdafx.h"
#include "CXeBootloaders.h"
#include "util.h"
#pragma region Bootloader load functions
void CXeBootloader::LoadBldrHdr()
{
	assert(this->pbData != 0);
	this->blHdr.wMagic = bswap16(*(WORD*)pbData);
	this->blHdr.wBuild = bswap16(*(WORD*)(pbData + 2));
	this->blHdr.wQfe = bswap16(*(WORD*)(pbData + 4));
	this->blHdr.wFlags = bswap16(*(WORD*)(pbData + 6));
	this->blHdr.dwEntrypoint = bswap32(*(DWORD*)(pbData + 8));
	this->blHdr.dwLength = bswap32(*(DWORD*)(pbData + 0xC));
}
void CXeBootloader::SaveBldrHdr()
{
	assert(this->pbData != 0);
	*(WORD*)pbData = bswap16(this->blHdr.wMagic);
	*(WORD*)(pbData + 2) = bswap16(this->blHdr.wBuild);
	*(WORD*)(pbData + 4) = bswap16(this->blHdr.wQfe);
	*(WORD*)(pbData + 6) = bswap16(this->blHdr.wFlags);
	*(DWORD*)(pbData + 8) = bswap32(this->blHdr.dwEntrypoint);
	*(DWORD*)(pbData + 0xC) = bswap32(this->blHdr.dwLength);
}
void CXeBootloader::PatchBootloader(BYTE* pbData, DWORD* cbData)
{
	DWORD* point = (DWORD*)pbData;
	DWORD offset = bswap32(*point++);
	while(offset != 0xFFFFFFFF)
	{
		if(offset == 0x504C4752 || offset == 0x52474c50)
			offset = bswap32(*point++);
		DWORD patchLen = bswap32(*point++);
		DWORD patchData = 0;
		DWORD patchAddr = 0;
		for(DWORD i = 0; i < patchLen; i++)
		{
			patchData = bswap32(*point++);
			patchAddr = offset + i*4;
			if(patchAddr == 0xC) // if we're patching the size field in bldr header
				if(this->Resize(patchData) != 0)
					Log(3, "CXeBootloader::PatchBootloader: failed to resize bootloader?\n");
			*(DWORD*)(this->pbData + patchAddr) = patchData;
		}
		offset = bswap32(*point++);
	}
	*cbData = (BYTE*)point - pbData;
}

__checkReturn BOOL CXeBootloader::Resize(DWORD size)
{
	if(this->pbData == 0)
		return 1;

	BYTE* newdata = (BYTE*)malloc(size);
	memcpy(newdata, this->pbData, size);
	free(this->pbData);
	this->pbData = newdata;
	this->cbData = size;
	return 0;
}

__checkReturn BOOL CXeBootloader::isValid()
{
	return (blHdr.wMagic == wValidMagic || 
		(blHdr.wMagic - 0x1000 == wValidMagic) || // devkits
		(blHdr.wMagic == (wValidMagic - 0x4010)) || // beta kits pre-1838
		(blHdr.wMagic == (wValidMagic - 0x4011)) || // beta kits pre-1838 4BL
		(blHdr.wMagic == (wValidMagic - 0x4110)) || // beta kits pre-1838
		(blHdr.wMagic - 0x1000 == (wValidMagic - 0x11)) || // beta kits 1838+
		(blHdr.wMagic - 0x1000 == (wValidMagic - 0x10)) // beta kits 1838+
		);
}

__checkReturn errno_t CXeSMC::Munge()
{
	BYTE key[4] = {0x42, 0x75, 0x4e, 0x79};
	for(DWORD i = 0; i < this->cbData; i++)
	{
		this->pbData[i] = (this->pbData[i] ^ (key[i&3] & 0xFF));
		int mod = this->pbData[i] * 0xFB;
		key[(i+1)&3] += mod & 0xFF;
		key[(i+2)&3] += (mod >> 8)&0xFF;
	}
	this->bIsDecrypted = FALSE;
	return 0;
}
__checkReturn errno_t CXeSMC::UnMunge()
{
	BYTE key[4] = {0x42, 0x75, 0x4e, 0x79};
	for(DWORD i = 0; i < this->cbData; i++)
	{
		int mod = this->pbData[i] * 0xFB;
		this->pbData[i] = (this->pbData[i] ^ (key[i&3] & 0xFF));
		key[(i+1)&3] += mod & 0xFF;
		key[(i+2)&3] += (mod >> 8)&0xFF;
	}
	this->bIsDecrypted = TRUE;
	return 0;
}
__checkReturn errno_t CXeSMC::GetHash(BYTE* pbHash, BOOL bUseDecrypted)
{
	Log(-1, "CXeSMC::GetHash: hashing SMC...\n");
	errno_t ret;
	if(this->bIsDecrypted && !bUseDecrypted)
		if(ret = this->Munge() != 0)
			return ret;
	if(bUseDecrypted && !this->bIsDecrypted)
		if(ret = this->UnMunge() != 0)
			return ret;
	
    ULONGLONG s0 = 0;
    ULONGLONG s1 = 0;
    for (DWORD i = 0; i < this->cbData / 4; i++)
    {
        DWORD tmp = *(DWORD*)(&this->pbData[i * 4]);
		tmp = bswap32(tmp);
        s0 += tmp;
        s1 -= tmp;
        s0 = (s0 << 29) | ((s0 & 0xFFFFFFF800000000) >> 35); // poor man's rotate left 29
        s1 = (s1 << 31) | ((s1 & 0xFFFFFFFE00000000) >> 33); // poor man's rotate left 31
    }
    pbHash[7] = (byte)(s0 & 0xFF);
    pbHash[6] = (byte)((s0 & 0xFF00) >> 8);
    pbHash[5] = (byte)((s0 & 0xFF0000) >> 16);
    pbHash[4] = (byte)((s0 & 0xFF000000) >> 24);
    pbHash[3] = (byte)((s0 & 0xFF00000000) >> 32);
    pbHash[2] = (byte)((s0 & 0xFF0000000000) >> 40);
    pbHash[1] = (byte)((s0 & 0xFF000000000000) >> 48);
    pbHash[0] = (byte)((s0 & 0xFF00000000000000) >> 56);
    pbHash[15] = (byte)(s1 & 0xFF);
    pbHash[14] = (byte)((s1 & 0xFF00) >> 8);
    pbHash[13] = (byte)((s1 & 0xFF0000) >> 16);
    pbHash[12] = (byte)((s1 & 0xFF000000) >> 24);
    pbHash[11] = (byte)((s1 & 0xFF00000000) >> 32);
    pbHash[10] = (byte)((s1 & 0xFF0000000000) >> 40);
    pbHash[9] = (byte)((s1 & 0xFF000000000000) >> 48);
    pbHash[8] = (byte)((s1 & 0xFF00000000000000) >> 56);
	if(this->bIsDecrypted && !bUseDecrypted)
		if(ret = this->UnMunge() != 0)
			return ret;
	if(bUseDecrypted && !this->bIsDecrypted)
		if(ret = this->Munge() != 0)
			return ret;
	return 0;
}
__checkReturn errno_t CXeSMC::GetHash(BYTE* pbHash)
{
	return this->GetHash(pbHash, false);
}
PSZ CXeSMC::GetMobo()
{
	switch(*this->pbMobo >> 4)
	{
	case 0:
	case 1:
		return "Xenon";
		break;
	case 2:
		return "Zephyr";
		break;
	case 3:
		return "Falcon";
		break;
	case 4:
		return "Jasper";
		break;
	case 5:
		return "Trinity";
		break;
	case 6:
		return "Corona";
		break;
	case 7:
		return "Winchester";
		break;
	case 8:
		return "Durango";
		break;
	}
	return "Unknown";
}
__checkReturn errno_t CXeSMC::Save(BOOL saveEncrypted)
{
	BYTE* data = pbData;
	if(this->bIsDecrypted && saveEncrypted)
		if(errno_t ret = this->Munge() != 0)
			return ret;
	Log(1, "CXeSMC::Save: saved SMC\n");
	return 0;
}
__checkReturn errno_t CXeSMC::Load(BOOL isEncrypted)
{
	BYTE* data = pbData;
	if(!isEncrypted)
	{
		this->pbMobo = (data + 0x100);
		this->pwVersion = (WORD*)(data+0x101);
		this->bIsDecrypted = TRUE;
	}
	else
	{
		if(errno_t ret = this->UnMunge() != 0)
			return ret;
		return this->Load(false);
	}
	Log(1, "CXeSMC::Load: loaded SMC\n");
	return 0;
}
__checkReturn errno_t CXeBootloaderFlash::CreateDefaults()
{
	if(this->pbData == 0)
	{
		this->pbData = (BYTE*)malloc(0x200);
		memset(this->pbData, 0, 0x200);
	}
	this->blHdr.wMagic = 0xFF4F;
	this->blHdr.wBuild = 0x760;
	this->blHdr.wFlags = 0;
	this->blHdr.wQfe = 0;
	this->blHdr.dwEntrypoint = 0x8000;
	this->blHdr.dwLength = 0x70000;
	this->bCopyrightSign = 0xA9;
	memset(&this->bCopyright, 0, 0x3F);
	strcpy_s(this->bCopyright, 0x37, " 2004-2010 Microsoft Corporation. All rights reserved.");
	memset(&this->bReserved, 0, 0x10);
	this->dwKeyVaultLength = 0x4000;
	this->dwSysUpdateAddr = 0x70000;
	this->wSysUpdateCount = 2;
	this->wKeyVaultVersion = 0x712;
	this->dwKeyVaultAddr = 0x4000;
	this->dwFileSystemAddr = 0x10000;
	this->dwSmcConfigAddr = 0;
	this->dwSmcLength = 0x3000;
	this->dwSmcAddr = 0x1000;
	return 0;
}
__checkReturn errno_t CXeBootloaderFlash::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	BYTE* data = pbData + 0x10;
	*(BYTE*)data = this->bCopyrightSign;
	memcpy(data + 1, this->bCopyright, 0x3F);
	memcpy(data + 0x40, this->bReserved, 0x10);
	*(DWORD*)(data + 0x50) = bswap32(this->dwKeyVaultLength);
	*(DWORD*)(data + 0x54) = bswap32(this->dwSysUpdateAddr);
	*(WORD*)(data + 0x58) = bswap16(this->wSysUpdateCount);
	*(WORD*)(data + 0x5A) = bswap16(this->wKeyVaultVersion);
	*(DWORD*)(data + 0x5C) = bswap32(this->dwKeyVaultAddr);
	*(DWORD*)(data + 0x60) = bswap32(this->dwFileSystemAddr);
	*(DWORD*)(data + 0x64) = bswap32(this->dwSmcConfigAddr);
	*(DWORD*)(data + 0x68) = bswap32(this->dwSmcLength);
	*(DWORD*)(data + 0x6C) = bswap32(this->dwSmcAddr);
	*(WORD*)(data + 0x70) = bswap16(this->wRgbpIndicator);
	Log(1, "CXeBootloaderFlash::Save: saved image header\n");
	return 0;
}
__checkReturn errno_t CXeBootloaderFlash::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	BYTE* data = pbData + 0x10;
	this->bCopyrightSign = *(BYTE*)(data);
	memcpy(&this->bCopyright, data + 1, 0x3F);
	memcpy(&this->bReserved, data + 0x40, 0x10);
	this->dwKeyVaultLength = bswap32(*(DWORD*)(data + 0x50));
	this->dwSysUpdateAddr = bswap32(*(DWORD*)(data + 0x54));
	this->wSysUpdateCount = bswap16(*(WORD*)(data + 0x58));
	this->wKeyVaultVersion = bswap16(*(WORD*)(data + 0x5A));
	this->dwKeyVaultAddr = bswap32(*(DWORD*)(data + 0x5C));
	this->dwFileSystemAddr = bswap32(*(DWORD*)(data + 0x60));
	this->dwSmcConfigAddr = bswap32(*(DWORD*)(data + 0x64));
	this->dwSmcLength = bswap32(*(DWORD*)(data + 0x68));
	this->dwSmcAddr = bswap32(*(DWORD*)(data + 0x6C));
	if(this->blHdr.wBuild >= 1838)
		this->wRgbpIndicator = bswap16(*(WORD*)(data + 0x70));
	else
		this->blHdr.dwEntrypoint = 0x80;

	this->bIsDecrypted = TRUE;
	Log(1, "CXeBootloaderFlash::Load: loaded image header\n");
	return 0;
}

__checkReturn errno_t CXeBootloader1BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	memcpy(this->pbData + 0x10, this->bRc4Key, 0x10);
	Log(1, "CXeBootloader1BL::Save: saved 1BL_B, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader1BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	memcpy(this->bRc4Key, this->pbData + 0x10, 0x10);
	Log(1, "CXeBootloader1BL::Load: loaded 1BL_B, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader2BL::FixPerBoxDigest()
{
	/*
	            byte[] smchash = Bootloader.Image.SMC.GetHash();
            X360IO io = new X360IO(new MemoryStream());
            io.Writer.Write(Bootloader.Rc4Key);
            io.Writer.Write(PairingData);
            io.Writer.Write(LockDownValue);
            io.Writer.Write(Reserved);
            io.Writer.Write(smchash);
            byte[] data = ((MemoryStream) io.Stream).ToArray();
            io.Close();
            data = new HMACSHA1(Bootloader.Image.CPUKey).ComputeHash(data);
            Array.Resize(ref data, 0x10);
            return data;
			*/
	BYTE pbData[0x30];
	if(errno_t ret = pxeSMC->GetHash((BYTE*)&pbData + 0x20) != 0)
		return ret;
	Log(-1, "CXeBootloader2BL::FixPerBoxDigest: fixing 2BL per-box digest...\n");
	// don't touch the digest if rc4key isnt set
	for(int i = 0; i < 0x10; i++)
		if(this->bRc4Key[i] != 0)
			break;
		else if(i+1 == 0x10)
			return 0;
	memcpy((BYTE*)&pbData, this->bRc4Key, 0x10);
	memcpy((BYTE*)&pbData + 0x10, &this->blPerBoxData.bPairingData, 0x3);
	pbData[0x13] = this->blPerBoxData.bLockDownValue;
	memcpy((BYTE*)&pbData + 0x14, this->blPerBoxData.bReserved, 0xC);
	XeCryptHmacSha(this->pbCPUKey, 0x10, pbData, 0x30, 0, 0, 0, 0, (BYTE*)&this->blPerBoxData.bPerBoxDigest, 0x10);
	return 0;
}
void CXeBootloader2BL::Crypt()
{
	assert(this->pbData != 0);
	if(this->pbPrevBldrKey != 0 && this->pbCPUKey != 0 && ((this->blHdr.wFlags & 0x800) == 0x800 || (this->blHdr.wMagic & XBOX_BLDR_MAGIC_DEVKIT) == XBOX_BLDR_MAGIC_DEVKIT))
	{
		// 2BL_A
		Log(0, "CXeBootloader2BL::Crypt: crypting 2BL...\n");
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	}
	// beta kits
	else if(this->pbPrevBldrKey != 0 && this->pbCPUKey == 0)
		memcpy(this->bRc4Key, this->pbPrevBldrKey, 0x10);
	else
	{
		// 2BL_B
		BYTE * nonce;
		DWORD noncelen;
		Log(0, "CXeBootloader2BL::Crypt: crypting 2BL_B %s...\n", ((blHdr.wFlags & XBOX_BLDR_FLAG_2BL_NEWCRYPTO) == XBOX_BLDR_FLAG_2BL_NEWCRYPTO ? "(new crypto)" : "(old crypto)"));
		if((blHdr.wFlags & XBOX_BLDR_FLAG_2BL_NEWCRYPTO) == XBOX_BLDR_FLAG_2BL_NEWCRYPTO)
		{
			// new 2BL_B crypto
			noncelen = 0x30;
			nonce = (BYTE*)malloc(noncelen);
			memcpy(nonce, this->pbHmacShaNonce, 0x10);
			memcpy(nonce + 0x10, this->pbCPUKey, 0x10);
			memcpy(nonce + 0x20, this->pbData, 0x10);
			// clear wFlags in bldr header
			nonce[0x26] = 0;
			nonce[0x27] = 0;
		}
		else
		{
			noncelen = 0x20;
			nonce = (BYTE*)malloc(noncelen);
			memcpy(nonce, this->pbHmacShaNonce, 0x10);
			memcpy(nonce + 0x10, this->pbCPUKey, 0x10);
		}
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, nonce, noncelen, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
		free(nonce);
	}
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
}
__checkReturn errno_t CXeBootloader2BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted)
	{
		// lets write our values out
		*(PBOOTLOADER_2BL_PERBOX)data = this->blPerBoxData;
		// skip signature and aes data
		*(ULONGLONG*)(data + 0x230) = bswap64(this->ulPostOutputAddr);
		*(ULONGLONG*)(data + 0x238) = bswap64(this->ulSbFlashAddr);
		*(ULONGLONG*)(data + 0x240) = bswap64(this->ulSocMmioAddr);
		// skip rsa public key - 4bl digest
		this->blAllowData.wConsoleSequenceAllow = bswap16(this->blAllowData.wConsoleSequenceAllow);
		*(PBOOTLOADER_2BL_ALLOWDATA)(data + 0x390) = this->blAllowData;
		*(DWORD*)(data + 0x394) = bswap32(this->dwPadding);

		if(saveEncrypted)
			this->Crypt();
	}
	Log(1, "CXeBootloader2BL::Save: saved 2BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader2BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->blPerBoxData = *(PBOOTLOADER_2BL_PERBOX)data;
		this->pbSignature = (BYTE*)(data + 0x20);
		this->pbAesInvData = (BYTE*)(data + 0x120);
		this->ulPostOutputAddr = bswap64(*(ULONGLONG*)(data + 0x230));
		this->ulSbFlashAddr = bswap64(*(ULONGLONG*)(data + 0x238));
		this->ulSocMmioAddr = bswap64(*(ULONGLONG*)(data + 0x240));
		this->pbRsaPublicKey = (BYTE*)(data + 0x248);
		this->pb3BLNonce = (BYTE*)(data + 0x358);
		this->pb3BLSalt = (BYTE*)(data + 0x368);
		this->pb4BLSalt = (BYTE*)(data + 0x372);
		this->pb4BLDigest = (BYTE*)(data + 0x37C);
		this->blAllowData = *(PBOOTLOADER_2BL_ALLOWDATA)(data + 0x390);
		this->blAllowData.wConsoleSequenceAllow = bswap16(this->blAllowData.wConsoleSequenceAllow);
		this->dwPadding = bswap32(*(DWORD*)(data + 0x394));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		// we gotta decrypt this shit!
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "CXeBootloader2BL::Load: loaded 2BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
void CXeBootloader3BL::Crypt()
{
	Log(0, "CXeBootloader3BL::Crypt: crypting 3BL...\n");
	assert(this->pbData != 0);
	if(this->pbPrevBldrKey != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
}
__checkReturn errno_t CXeBootloader3BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted && saveEncrypted)
		this->Crypt();

	Log(1, "CXeBootloader3BL::Save: saved 3BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader3BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->pbSignature = (BYTE*)(data);
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "CXeBootloader3BL::Load: loaded 3BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader4BL::Crypt(BOOL isDecrypting)
{
	assert(this->pbData != 0);
	Log(0, "CXeBootloader4BL::Crypt: attempting to crypt 4BL with %s key...\n", (this->bUsesCpuKey ? "CPU" : "RC4"));
	if(this->pbCPUKey == 0 && this->pbBetaKitNonce != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, this->pbBetaKitNonce, 0x20, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	else if(!this->bUsesCpuKey)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	else
		XeCryptHmacSha(this->pbCPUKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	if(isDecrypting)
	{
		this->bIsDecrypted = this->pbCPUKey == 0 || ((this->pbData[0x120] == 0 && this->pbData[0x121] == 0 && this->pbData[0x122] == 0 && this->pbData[0x123] == 0x20) || (this->pbData[0x210] == 0 && this->pbData[0x211] == 0 && this->pbData[0x212] == 0 && this->pbData[0x213] == 0));
		if(!this->bIsDecrypted)
		{
			if(!this->bUsesCpuKey)
			{
				this->bUsesCpuKey = true;
				return 0;
			}
			else
			{
				// cant decrypt?
				Log(3, "CXeBootloader4BL::Crypt: can't decrypt 4BL, wrong cpukey?\n");
				return 1;
			}
		}
		return 0;
	}
	return 0;
}
__checkReturn errno_t CXeBootloader4BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted)
	{
		*(WORD*)(data + 0x22A) = bswap16(this->wPadding);
		if(saveEncrypted)
			if(errno_t ret = this->Crypt(false) != 0)
				return ret;
	}
	Log(1, "CXeBootloader4BL::Save: saved 4BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader4BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->pbSignature = (BYTE*)(data);
		this->pbRsaPublicKey = (BYTE*)(data + 0x100);
		this->pb6BLNonce = (BYTE*)(data + 0x210);
		this->pb6BLSalt = (BYTE*)(data + 0x220);
		this->wPadding = bswap16(*(WORD*)(data + 0x22A));
		this->pb5BLDigest = (BYTE*)(data + 0x22C);
		this->bIsDecrypted = TRUE;
	}
	else
	{
		if(this->Crypt(true) != 0)
			return 0;
		return this->Load(!this->bIsDecrypted);
	}
	Log(1, "CXeBootloader4BL::Load: loaded 4BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
void CXeBootloader5BL::Crypt()
{
	assert(this->pbData != 0);
	Log(0, "CXeBootloader5BL::Crypt: crypting 5BL...\n");
	if(this->pbPrevBldrKey != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
}
__checkReturn errno_t CXeBootloader5BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted)
	{
		*(ULONGLONG*)(data) = bswap64(this->ulImageAddr);
		*(DWORD*)(data + 0x8) = bswap32(this->dwImageLength);
		*(DWORD*)(data + 0xC) = bswap32(this->dwPadding);
		if(saveEncrypted)
			this->Crypt();
	}

	Log(1, "CXeBootloader5BL::Save: saved 5BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader5BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->ulImageAddr = bswap64(*(ULONGLONG*)(data));
		this->dwImageLength = bswap32(*(DWORD*)(data + 0x8));
		this->dwPadding = bswap32(*(DWORD*)(data + 0xC));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "CXeBootloader5BL::Load: loaded 5BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader6BL::FixPerBoxDigest()
{
	/*
	            byte[] data = Bootloader.GetData();
            Bootloader.Rc4Key = new HMACSHA1(Bootloader.HmacShaKey).ComputeHash(data, 0x20, 0x10);
            for (int i = 0; i < 0x10; i++)
                data[0x20 + i] = Bootloader.Rc4Key[i];
            data = new HMACSHA1(Bootloader.Image.CPUKey).ComputeHash(data, 0, 0x220);
            Array.Resize(ref data, 0x10);
            return data;
			*/
	Log(-1, "CXeBootloader6BL::FixPerBoxDigest: fixing 6BL per-box digest...\n");
	if(this->pbPrevBldrKey != 0)
	{
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	}
	BYTE* data = (BYTE*)malloc(0x220);
	memcpy(data, this->pbData, 0x220);
	memcpy(data + 0x20, this->bRc4Key, 0x10);
	XeCryptHmacSha(this->pbCPUKey, 0x10, data, 0x220, 0, 0, 0, 0, (BYTE*)&this->bl6BLPerBoxData.bPerBoxDigest, 0x10);
	free(data);
	return 0;
}
void CXeBootloader6BL::Crypt()
{
	assert(this->pbData != 0);
	Log(0, "CXeBootloader6BL::Crypt: crypting 6BL...\n");
	if(this->pbPrevBldrKey != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x30, blHdr.dwLength - 0x30);
}
__checkReturn errno_t CXeBootloader6BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x20;
	BYTE* data = pbData + 0x10;
	*(DWORD*)(data) = bswap32(this->dwBuildQfeSource << 16);
	*(DWORD*)(data + 0x4) = bswap32(this->dwBuildQfeTarget << 16);
	*(DWORD*)(data + 0x8) = bswap32(this->dwReserved);
	*(DWORD*)(data + 0xC) = bswap32(this->dw7BLLength);
	if(this->bIsDecrypted)
	{
		data = pbData + 0x20;
		#ifndef _XBOX // byteswap everything if we aren't on xbox
			for(int i = 0; i < this->bl7BLPerBoxData.wUsedBlocksCount; i++)
				this->bl7BLPerBoxData.wBlockNumbers[i] = bswap16(this->bl7BLPerBoxData.wBlockNumbers[i]);
			this->bl7BLPerBoxData.wUsedBlocksCount = bswap16(this->bl7BLPerBoxData.wUsedBlocksCount);
		#endif
		*(PBOOTLOADER_7BL_PERBOX)(data + 0x10) = this->bl7BLPerBoxData;
		*(PBOOTLOADER_6BL_PERBOX)(data + 0x1D0) = this->bl6BLPerBoxData;
		*(DWORD*)(data + 0x334) = bswap32(this->dwPadding);
		if(saveEncrypted)
			this->Crypt();
	}

	Log(1, "CXeBootloader6BL::Save: saved 6BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader6BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x20;
	BYTE* data = pbData + 0x10;
	if(!isEncrypted)
	{
		this->dwBuildQfeSource = bswap32(*(DWORD*)(data)) >> 16;
		this->dwBuildQfeTarget = bswap32(*(DWORD*)(data + 0x4)) >> 16;
		this->dwReserved = bswap32(*(DWORD*)(data + 0x8));
		this->dw7BLLength = bswap32(*(DWORD*)(data + 0xC));
		data = pbData + 0x20;
		this->bl7BLPerBoxData = *(PBOOTLOADER_7BL_PERBOX)(data + 0x10);
#ifndef _XBOX // byteswap everything if we aren't on xbox
		this->bl7BLPerBoxData.wUsedBlocksCount = bswap16(this->bl7BLPerBoxData.wUsedBlocksCount);
		for(int i = 0; i < this->bl7BLPerBoxData.wUsedBlocksCount; i++)
			this->bl7BLPerBoxData.wBlockNumbers[i] = bswap16(this->bl7BLPerBoxData.wBlockNumbers[i]);
#endif
		this->bl6BLPerBoxData = *(PBOOTLOADER_6BL_PERBOX)(data + 0x1D0);
		this->pbSignature = (BYTE*)(data + 0x210);
		this->pb7BLNonce = (BYTE*)(data + 0x310);
		this->pb7BLDigest = (BYTE*)(data + 0x320);
		this->dwPadding = bswap32(*(DWORD*)(data + 0x334));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "CXeBootloader6BL::Load: loaded 6BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
void CXeBootloader7BL::Crypt()
{
	assert(this->pbData != 0);
	Log(0, "CXeBootloader7BL::Crypt: crypting 7BL...\n");
	if(this->pbPrevBldrKey != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->bRc4Key, 0x10);
	
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
}
__checkReturn errno_t CXeBootloader7BL::Save(BOOL saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted)
	{
		*(DWORD*)(data) = bswap32(this->dwSourceImageLength);
		*(DWORD*)(data + 0x18) = bswap32(this->dwTargetImageLength);
		*(DWORD*)(data + 0x30) = bswap32(this->dw6BLLength);
		if(saveEncrypted)
			this->Crypt();
	}

	Log(1, "CXeBootloader7BL::Save: saved 7BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
__checkReturn errno_t CXeBootloader7BL::Load(BOOL isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->dwSourceImageLength = bswap32(*(DWORD*)(data));
		this->pbSourceDigest = (BYTE*)(data + 0x4);
		this->dwTargetImageLength = bswap32(*(DWORD*)(data + 0x18));
		this->pbTargetDigest = (BYTE*)(data + 0x1C);
		this->dw6BLLength = bswap32(*(DWORD*)(data + 0x30));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "CXeBootloader7BL::Load: loaded 7BL, build %d, flags 0x%x, entrypoint 0x%x, length 0x%x\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
#pragma endregion Bootloader load/save functions
