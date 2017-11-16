#include "stdafx.h"
#include "CXeBootloaders.h"
#include "util.h"
#pragma region Bootloader load functions
int CXeBootloader::LoadBldrHdr()
{
	this->blHdr.wMagic = bswap16(*(WORD*)pbData);
	this->blHdr.wBuild = bswap16(*(WORD*)(pbData + 2));
	this->blHdr.wQfe = bswap16(*(WORD*)(pbData + 4));
	this->blHdr.wFlags = bswap16(*(WORD*)(pbData + 6));
	this->blHdr.dwEntrypoint = bswap32(*(DWORD*)(pbData + 8));
	this->blHdr.dwLength = bswap32(*(DWORD*)(pbData + 0xC));
	return 1;
}
int CXeBootloader::SaveBldrHdr()
{
	*(WORD*)pbData = bswap16(this->blHdr.wMagic);
	*(WORD*)(pbData + 2) = bswap16(this->blHdr.wBuild);
	*(WORD*)(pbData + 4) = bswap16(this->blHdr.wQfe);
	*(WORD*)(pbData + 6) = bswap16(this->blHdr.wFlags);
	*(DWORD*)(pbData + 8) = bswap32(this->blHdr.dwEntrypoint);
	*(DWORD*)(pbData + 0xC) = bswap32(this->blHdr.dwLength);
	return 1;
}
bool CXeBootloader::isValid()
{
	return (blHdr.wMagic == wValidMagic || (blHdr.wMagic == (wValidMagic | 0x1000)));
}

bool CSMC::Munge()
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
	return TRUE;
}
bool CSMC::UnMunge()
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
	return TRUE;
}
PSZ CSMC::GetMobo()
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
int CSMC::Save(bool saveEncrypted)
{
	BYTE* data = pbData;
	if(this->bIsDecrypted && saveEncrypted)
		this->Munge();
	Log(1, "saved SMC\n");
	return 0;
}
int CSMC::Load(bool isEncrypted)
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
		this->UnMunge();
		return this->Load(false);
	}
	Log(1, "loaded SMC\n");
	return 0;
}
int CXeBootloaderFlash::Save(bool saveEncrypted)
{
	CXeBootloader::SaveBldrHdr();
	BYTE* data = pbData + 0x10;
	*(BYTE*)data = this->bCopyrightSign;
	// bCopyright and bReserved should get updated auto?
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
	Log(1, "saved image header\n");
	return 0;
}
int CXeBootloaderFlash::Load(bool isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	BYTE* data = pbData + 0x10;
	this->bCopyrightSign = *(BYTE*)(data);
	this->bCopyright = (CHAR*)(data + 1);
	this->bReserved = (BYTE*)(data + 0x40);
	this->dwKeyVaultLength = bswap32(*(DWORD*)(data + 0x50));
	this->dwSysUpdateAddr = bswap32(*(DWORD*)(data + 0x54));
	this->wSysUpdateCount = bswap16(*(WORD*)(data + 0x58));
	this->wKeyVaultVersion = bswap16(*(WORD*)(data + 0x5A));
	this->dwKeyVaultAddr = bswap32(*(DWORD*)(data + 0x5C));
	this->dwFileSystemAddr = bswap32(*(DWORD*)(data + 0x60));
	this->dwSmcConfigAddr = bswap32(*(DWORD*)(data + 0x64));
	this->dwSmcLength = bswap32(*(DWORD*)(data + 0x68));
	this->dwSmcAddr = bswap32(*(DWORD*)(data + 0x6C));
	this->wRgbpIndicator = bswap16(*(WORD*)(data + 0x70));
	this->bIsDecrypted = TRUE;
	Log(1, "loaded image header\n");
	return 0;
}

int CXeBootloader2BL::Crypt()
{
	if(this->pbPrevBldrKey != 0 && (this->pbCPUKey == 0 || (this->blHdr.wFlags & 0x800) == 0x800))
	{
		// 2BL_A
		Log(-1, "crypting 2BL...\n");
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
	}
	else
	{
		// 2BL_B
		BYTE * nonce;
		DWORD noncelen;
		Log(-1, "crypting 2BL_B %s...\n", ((blHdr.wFlags & XBOX_BLDR_FLAG_2BL_NEWCRYPTO) == XBOX_BLDR_FLAG_2BL_NEWCRYPTO ? "(new crypto)" : "(old crypto)"));
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
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, nonce, noncelen, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
		free(nonce);
	}
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	return 0;
}
int CXeBootloader2BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
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
	Log(1, "saved 2BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader2BL::Load(bool isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->blPerBoxData = *(PBOOTLOADER_2BL_PERBOX)data;
		this->bSignature = (BYTE*)(data + 0x20);
		this->bAesInvData = (BYTE*)(data + 0x120);
		this->ulPostOutputAddr = bswap64(*(ULONGLONG*)(data + 0x230));
		this->ulSbFlashAddr = bswap64(*(ULONGLONG*)(data + 0x238));
		this->ulSocMmioAddr = bswap64(*(ULONGLONG*)(data + 0x240));
		this->bRsaPublicKey = (BYTE*)(data + 0x248);
		this->b3BLNonce = (BYTE*)(data + 0x358);
		this->b3BLSalt = (BYTE*)(data + 0x368);
		this->b4BLSalt = (BYTE*)(data + 0x372);
		this->b4BLDigest = (BYTE*)(data + 0x37C);
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
	Log(1, "loaded 2BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader3BL::Crypt()
{
	Log(-1, "crypting 3BL...\n");
	if(this->pbPrevBldrKey != 0)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
		
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	return 0;
}
int CXeBootloader3BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted && saveEncrypted)
		this->Crypt();

	Log(1, "saved 3BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader3BL::Load(bool isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->bSignature = (BYTE*)(data);
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "loaded 3BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader4BL::Crypt(bool isDecrypting)
{
	Log(-1, "attempting to crypt 4BL with %s key...\n", (this->bUsesCpuKey ? "CPU" : "RC4"));
	if(!this->bUsesCpuKey)
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
	else
		XeCryptHmacSha(this->pbCPUKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
		
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	if(isDecrypting)
	{
		this->bIsDecrypted = (this->pbData[0x20] == 0 && this->pbData[0x21] == 0 && this->pbData[0x22] == 0 && this->pbData[0x23] == 0 && this->pbData[0x210] == 0 && this->pbData[0x211] == 0 && this->pbData[0x212] == 0 && this->pbData[0x213] == 0);
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
				Log(3, "can't decrypt 4BL?\n");
				return 1;
			}
		}
		return 0;
	}
	return 0;
}
int CXeBootloader4BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(this->bIsDecrypted)
	{
		*(WORD*)(data + 0x22A) = bswap16(this->wPadding);
		if(saveEncrypted)
			this->Crypt(false);
	}
	Log(1, "saved 4BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader4BL::Load(bool isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->bSignature = (BYTE*)(data);
		this->bRsaPublicKey = (BYTE*)(data + 0x100);
		this->b6BLNonce = (BYTE*)(data + 0x210);
		this->b6BLSalt = (BYTE*)(data + 0x220);
		this->wPadding = bswap16(*(WORD*)(data + 0x22A));
		this->b5BLDigest = (BYTE*)(data + 0x22C);
		this->bIsDecrypted = TRUE;
	}
	else
	{
		if(this->Crypt(true) != 0)
			return 0;
		return this->Load(!this->bIsDecrypted);
	}
	Log(1, "loaded 4BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader5BL::Crypt()
{
	Log(-1, "crypting 5BL...\n");
	if(this->pbPrevBldrKey != 0)
	{
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
	}
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	return 0;
}
int CXeBootloader5BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
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

	Log(1, "saved 5BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader5BL::Load(bool isEncrypted)
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
	Log(1, "loaded 5BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader6BL::Crypt()
{
	Log(-1, "crypting 6BL...\n");
	if(this->pbPrevBldrKey != 0)
	{
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
	}
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x30, blHdr.dwLength - 0x30);
	return 0;
}
int CXeBootloader6BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
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
			this->bl7BLPerBoxData.wUsedBlocksCount = bswap16(this->bl7BLPerBoxData.wUsedBlocksCount);
			for(int i = 0; i < this->bl7BLPerBoxData.wUsedBlocksCount; i++)
				this->bl7BLPerBoxData.wBlockNumbers[i] = bswap16(this->bl7BLPerBoxData.wBlockNumbers[i]);
		#endif
		*(PBOOTLOADER_7BL_PERBOX)(data + 0x10) = this->bl7BLPerBoxData;
		*(PBOOTLOADER_6BL_PERBOX)(data + 0x1D0) = this->bl6BLPerBoxData;
		*(DWORD*)(data + 0x334) = bswap32(this->dwPadding);
		if(saveEncrypted)
			this->Crypt();
	}

	Log(1, "saved 6BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader6BL::Load(bool isEncrypted)
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
		this->bSignature = (BYTE*)(data + 0x210);
		this->b7BLNonce = (BYTE*)(data + 0x310);
		this->b7BLDigest = (BYTE*)(data + 0x320);
		this->dwPadding = bswap32(*(DWORD*)(data + 0x334));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "loaded 6BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader7BL::Crypt()
{
	Log(-1, "crypting 7BL...\n");
	if(this->pbPrevBldrKey != 0)
	{
		XeCryptHmacSha(this->pbPrevBldrKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, (BYTE*)&this->pbRc4Key, 0x10);
	}
	XeRc4Context state;
	XeCryptRc4Key(&state, (BYTE*)&this->pbRc4Key, 0x10);
	XeCryptRc4Ecb(&state, this->pbData + 0x20, blHdr.dwLength - 0x20);
	return 0;
}
int CXeBootloader7BL::Save(bool saveEncrypted)
{
	CXeBootloader::LoadBldrHdr();
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

	Log(1, "saved 7BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
int CXeBootloader7BL::Load(bool isEncrypted)
{
	CXeBootloader::LoadBldrHdr();
	this->pbHmacShaNonce = pbData + 0x10;
	BYTE* data = pbData + 0x20;
	if(!isEncrypted)
	{
		this->dwSourceImageLength = bswap32(*(DWORD*)(data));
		this->bSourceDigest = (BYTE*)(data + 0x4);
		this->dwTargetImageLength = bswap32(*(DWORD*)(data + 0x18));
		this->bTargetDigest = (BYTE*)(data + 0x1C);
		this->dw6BLLength = bswap32(*(DWORD*)(data + 0x30));
		this->bIsDecrypted = TRUE;
	}
	else
	{
		this->Crypt();
		return this->Load(false);
	}
	Log(1, "loaded 7BL build %d, flags 0x%x, entrypoint 0x%x, length %d\n", this->blHdr.wBuild, this->blHdr.wFlags, this->blHdr.dwEntrypoint, this->blHdr.dwLength);
	return 0;
}
#pragma endregion Bootloader load/save functions
