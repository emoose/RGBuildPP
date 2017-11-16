#include "stdafx.h"
#include "CXeKeyVault.h"
#include "XeCrypt.h"
#include "util.h"
#include "version.h"
#ifndef _XBOX
#include <wincrypt.h>
#endif

// shhh... lets not leak our hard work
#if !RGB_VER_MIN
#include "rgbp.h"
#endif

__checkReturn errno_t CXeKeyVault::CalculateNonce(BYTE* pbNonceBuff, DWORD cbNonceBuff)
{
	WORD kvVer = bswap16(*this->pwKeyVaultVersion);
	XeCryptHmacSha(this->pbCPUKey, 0x10, ((BYTE*)&this->xeData) + 0x10, 0x3ff0, (BYTE*)&kvVer, 0x2, 0, 0, pbNonceBuff, cbNonceBuff);
	return 0;
}
__checkReturn errno_t CXeKeyVault::RandomizeKeys()
{
	BYTE randomKey[0x6];
	BYTE random[0x1000];
	CHAR randomSerial[0x18];
	Log(1, "CXeKeyVault::RandomizeKeys: randomizing KeyVault keys...\n");
	// get some random data to work with
	XeCryptRandom((BYTE*)&randomKey, 0x6);
	XeCryptRc4((BYTE*)&randomKey, 0x6, (BYTE*)&random, 0x1000);

	// randomize serial numbers
	_snprintf_s(randomSerial, 0x18, 0x18, "%013I64u", *(ULONGLONG*)&random);
	randomSerial[0xC] = 0;
	randomSerial[0xD] = 0;
	randomSerial[0xE] = 0;
	randomSerial[0xF] = 0;
	
	// set console/mobo serial numbers
	memcpy(&this->xeData.sz14ConsoleSerialNumber, &randomSerial, 0xC);
	memcpy(&this->xeData.b15MoboSerialNumber, (BYTE*)&random + 0x20, 0x8);

	// set region to devkit
	this->xeData.w16GameRegion = bswap16(0x7FFF);

	// randomize some keys
	memcpy(&this->xeData.b12ReservedRandomKey1, (BYTE*)&random + 0x30, 0x10);
	memcpy(&this->xeData.b13ReservedRandomKey2, (BYTE*)&random + 0x40, 0x10);
	memcpy(&this->xeData.b17ConsoleObfuscationKey, (BYTE*)&random + 0x50, 0x10);
	memcpy(&this->xeData.b18KeyObfuscationKey, (BYTE*)&random + 0x60, 0x10);
	memcpy(&this->xeData.b1ADvdKey, (BYTE*)&random + 0x70, 0x10);
	memcpy(&this->xeData.b1BPrimaryActivationKey, (BYTE*)&random + 0x80, 0x18);
	memcpy(&this->xeData.b1CSecondaryActivationKey, (BYTE*)&random + 0x98, 0x10);
#ifdef RGBPH
	if(!rkc(this))
	{
		Log(3, "CXeKeyVault::RandomizeKeys: rkc failed?\n");
		return 1;
	}
#endif
	Log(1, "CXeKeyVault::RandomizeKeys: KeyVault keys randomized!\n");
	return 0;
}
KV_CONTROLLER_DATA DesData[] = {
	// k1idx, k2idx, k1data, k2data
	{ 0x1D, 0x1E, 
// these two keys are the only ones that are written raw with no crypto involved
// only been observed in beta kit keyvaults, but they work for retail too
// probably means the other keys here are for beta kits too, but they work fine, could make this key some sort of root key?
		{0xC2, 0x15, 0xE5, 0x5E, 0xE5, 0x51, 0x94, 0x2A, 0xEC, 0x3D, 0x45, 0xEC, 0xB6, 0xE6, 0xF2, 0x16}, 
		{0xC7, 0x45, 0xAD, 0x1F, 0x08, 0x0B, 0xD9, 0xE9, 0x9B, 0x1C, 0x34, 0xE3, 0xA4, 0x6D, 0xC8, 0xC4}
// for comparison, here are retail/devkit keys
//		{0xE3, 0x5B, 0xFB, 0x1C, 0xCD, 0xAD, 0x32, 0x5B, 0xF7, 0x0E, 0x07, 0xFD, 0x62, 0x3D, 0xA7, 0xC4},
//		{0x8F, 0x29, 0x08, 0x38, 0x0B, 0x5B, 0xFE, 0x68, 0x7C, 0x26, 0x46, 0x2A, 0x51, 0xF2, 0xBC, 0x19}
	},
	{ 0x1F, 0x20, 
		{0x68, 0xDA, 0xB6, 0x2F, 0x49, 0xC2, 0xF2, 0xB3, 0x1C, 0xF7, 0x16, 0x2F, 0xBC, 0x19, 0x02, 0xA2}, 
		{0x98, 0x40, 0xF7, 0x58, 0x5E, 0x20, 0xF2, 0x2A, 0x13, 0x0B, 0xAD, 0x68, 0x61, 0x13, 0x1A, 0x1A}
	},
	{ 0x21, 0x22, 
		{0xFD, 0xEF, 0x08, 0x4C, 0x68, 0x2C, 0x76, 0x0D, 0x91, 0x49, 0x13, 0x0E, 0x4C, 0x5B, 0xBF, 0x38},
		{0xEF, 0x37, 0xF2, 0xE6, 0x54, 0xEA, 0x0D, 0x5E, 0xEA, 0xE3, 0x97, 0x7C, 0x61, 0x3B, 0x3B, 0x6E}
	},
	{ 0x23, 0x24, 
		{0xB9, 0xE0, 0x9E, 0x68, 0x04, 0x83, 0x91, 0xB3, 0x32, 0x45, 0x7A, 0xDA, 0x43, 0x6B, 0x80, 0xAD},
		{0x92, 0x5D, 0x29, 0x6E, 0xB0, 0x61, 0x0B, 0xF1, 0xD6, 0x29, 0x3B, 0xC8, 0xC7, 0xD9, 0x32, 0xBC}
	},
	{ 0x25, 0x26, 
		{0x97, 0x57, 0x8A, 0x01, 0xD9, 0x76, 0x01, 0x04, 0x23, 0xFE, 0xC7, 0xDC, 0x10, 0x8F, 0x58, 0xDC},
		{0xAE, 0x79, 0xE3, 0x8F, 0x8C, 0x19, 0x64, 0x10, 0xA4, 0xC1, 0x7A, 0xEC, 0x0E, 0x73, 0xBC, 0x5D}
	},
	{ 0x27, 0x28, 
		{0x97, 0x57, 0x8A, 0x01, 0xD9, 0x76, 0x01, 0x04, 0x23, 0xFE, 0xC7, 0xDC, 0x10, 0x8F, 0x58, 0xDC},
		{0xAE, 0x79, 0xE3, 0x8F, 0x8C, 0x19, 0x64, 0x10, 0xA4, 0xC1, 0x7A, 0xEC, 0x0E, 0x73, 0xBC, 0x5D}
	},
	{ 0x29, 0x2A, 
		{0x68, 0xDA, 0xB6, 0x2F, 0x49, 0xC2, 0xF2, 0xB3, 0x1C, 0xF7, 0x16, 0x2F, 0xBC, 0x19, 0x02, 0xA2},
		{0x98, 0x40, 0xF7, 0x58, 0x5E, 0x20, 0xF2, 0x2A, 0x13, 0x0B, 0xAD, 0x68, 0x61, 0x13, 0x1A, 0x1A}
	},
	{ 0x2B, 0x2C, 
		{0xFD, 0xEF, 0x08, 0x4C, 0x68, 0x2C, 0x76, 0x0D, 0x91, 0x49, 0x13, 0x0E, 0x4C, 0x5B, 0xBF, 0x38},
		{0xEF, 0x37, 0xF2, 0xE6, 0x54, 0xEA, 0x0D, 0x5E, 0xEA, 0xE3, 0x97, 0x7C, 0x61, 0x3B, 0x3B, 0x6E}
	},
	{ 0x2D, 0x2E, 
		{0x0B, 0xC1, 0x49, 0xF1, 0xE9, 0x38, 0x5D, 0x80, 0xBA, 0xEF, 0x98, 0x83, 0x7A, 0xD3, 0x32, 0xD0},
		{0x70, 0x5D, 0xBF, 0x3E, 0xF1, 0x86, 0x3D, 0xDA, 0x86, 0x2C, 0x1C, 0xDA, 0x89, 0x08, 0xA1, 0xD9}
	},
	{ 0x2F, 0x30, 
		{0x97, 0x57, 0x8A, 0x01, 0xD9, 0x76, 0x01, 0x04, 0x23, 0xFE, 0xC7, 0xDC, 0x10, 0x8F, 0x58, 0xDC},
		{0xAE, 0x79, 0xE3, 0x8F, 0x8C, 0x19, 0x64, 0x10, 0xA4, 0xC1, 0x7A, 0xEC, 0x0E, 0x73, 0xBC, 0x5D}
	},
	{ 0x31, 0x22, 
		{0x97, 0x57, 0x8A, 0x01, 0xD9, 0x76, 0x01, 0x04, 0x23, 0xFE, 0xC7, 0xDC, 0x10, 0x8F, 0x58, 0xDC},
		{0xAE, 0x79, 0xE3, 0x8F, 0x8C, 0x19, 0x64, 0x10, 0xA4, 0xC1, 0x7A, 0xEC, 0x0E, 0x73, 0xBC, 0x5D}
	}
};
__checkReturn errno_t CXeKeyVault::RepairDesKeys()
{
	//TODO: make sure this works (genned kv's have a lot of repeating DES keys?)
	BYTE desSalt[0x8];
	BYTE desInput[0x14];
	Log(1, "CXeKeyVault::RepairDesKeys: repairing KeyVault DES keys...\n");
	memcpy(&desSalt, &this->xeData.b36ConsoleCertificate.ConsoleId, 0x5);
	desSalt[5] = 0x80;
	desSalt[6] = 0x81;
	desSalt[7] = 0x82;
	XeCryptSha((BYTE*)&desSalt, 8, 0, 0, 0, 0, (BYTE*)&desInput, 0x14);
	BYTE* keyPoint = (BYTE*)&this->xeData.b1DGlobalDevice2DesKey1;
	for(int i = 0; i < 0xB; i++)
	{
		if(i == 0)
		{
			memcpy(keyPoint, &DesData[i].dwKey1Data, 0x10);
			memcpy(keyPoint + 0x10, &DesData[i].dwKey2Data, 0x10);
		}
		else
		{
			BYTE desPar[0x18];
			BYTE desKey[0x18];
			BYTE desOut[0x10];
			BYTE desOut2[0x10];
			XeDes3Context state;
			ULONGLONG feed = 0;
			memcpy(&desPar, DesData[i].dwKey1Data, 0x10);
			XeCryptDesParity((BYTE*)&desPar, 0x18, (BYTE*)&desKey);
			XeCryptDes3Key(&state, (BYTE*)&desKey);
			XeCryptDes3Cbc(&state, (BYTE*)&desInput, 0x10, (BYTE*)&desOut, (BYTE*)&feed, XE_CRYPT_ENC);
			feed = 0;
			memcpy(&desPar, DesData[i].dwKey2Data, 0x10);
			XeCryptDesParity((BYTE*)&desPar, 0x18, (BYTE*)&desKey);
			XeCryptDes3Key(&state, (BYTE*)&desKey);
			XeCryptDes3Cbc(&state, (BYTE*)&desInput + 4, 0x10, (BYTE*)&desOut2, (BYTE*)&feed, XE_CRYPT_ENC);
			feed = 0;

			memcpy(keyPoint, &desOut, 0x10);
			memcpy(keyPoint + 0x10, &desOut2, 0x10);
		}
		keyPoint += 0x20;
	}
	return 0;
}
__checkReturn errno_t CXeKeyVault::Crypt(BOOL isDecrypting)
{
	BYTE* data = ((BYTE*)&xeData) + 0x10;
	Log(0, "CXeKeyVault::Crypt: crypting KeyVault...\n");
	// crypt that bitch, yeah crypt it good
	byte nonce[0x10];
	byte calcnonce[0x10];
	if(!isDecrypting)
		this->CalculateNonce((BYTE*)&this->xeData.bKeyVaultNonce, 0x10);

	memcpy((void*)&nonce, (void*)this->pbHmacShaNonce, 0x10);
	XeCryptHmacSha(this->pbCPUKey, 0x10, this->pbHmacShaNonce, 0x10, 0, 0, 0, 0, this->bRc4Key, 0x10);
	XeRc4Context state;
	XeCryptRc4Key(&state, this->bRc4Key, 0x10);
	XeCryptRc4Ecb(&state, data, 0x4000 - 0x10);
	if(isDecrypting)
		this->CalculateNonce((BYTE*)&calcnonce, 0x10);
	this->bIsDecrypted = FALSE;
	if(!isDecrypting)
		return 0;
	if(!memcmp(&nonce, &calcnonce, 0x10)) // make sure the nonce is equal to what we expect
	{
		this->bIsDecrypted = TRUE;
		return 0;
	}
	return 1;
}
__checkReturn errno_t CXeKeyVault::Save(BOOL saveEncrypted)
{
	this->pbHmacShaNonce = (BYTE*)&xeData.bKeyVaultNonce;
	BYTE* data = ((BYTE*)&xeData) + 0x10;
	BOOL origdec = this->bIsDecrypted;
	if(this->bIsDecrypted && !saveEncrypted)
	{
		/*
#ifndef _XBOX
		// byteswap stuff
		this->xeData.w4OddFeatures = bswap16(this->xeData.w4OddFeatures);
		this->xeData.w5OddAuthType = bswap16(this->xeData.w5OddAuthType);
		this->xeData.w16GameRegion = bswap16(this->xeData.w16GameRegion);
		this->xeData.dw6RestrictedHvExtLoader = bswap32(this->xeData.dw6RestrictedHvExtLoader);
		this->xeData.dw7PolicyFlashSize = bswap32(this->xeData.dw7PolicyFlashSize);
		this->xeData.dw8PolicyBuiltInUsbMuSize = bswap32(this->xeData.dw8PolicyBuiltInUsbMuSize);
		this->xeData.dw9ReservedDword4 = bswap32(this->xeData.dw9ReservedDword4);
		this->xeData.qwARestrictedPrivileges = bswap64(this->xeData.qwARestrictedPrivileges);
		this->xeData.qwBReservedQword2 = bswap64(this->xeData.qwBReservedQword2);
		this->xeData.qwCReservedQword3 = bswap64(this->xeData.qwCReservedQword3);
		this->xeData.qwDReservedQword4 = bswap64(this->xeData.qwDReservedQword4);
		this->xeData.b36ConsoleCertificate.CertSize = bswap16(this->xeData.b36ConsoleCertificate.CertSize);
		this->xeData.b36ConsoleCertificate.ConsoleType = bswap32(this->xeData.b36ConsoleCertificate.ConsoleType);
		this->xeData.b36ConsoleCertificate.Privileges = bswap16(this->xeData.b36ConsoleCertificate.Privileges);
#endif*/ // DISABLED CUZ ITS BUGGED TO HELL
	}
	if(origdec && saveEncrypted)
		this->Crypt(false);
	else if(!origdec && !saveEncrypted)
		this->Crypt(true);
	Log(1, "CXeKeyVault::Save: saved KeyVault\n");
	return 0;
}
__checkReturn errno_t CXeKeyVault::Load(BOOL isEncrypted)
{
	this->pbHmacShaNonce = (BYTE*)&xeData.bKeyVaultNonce;
	BYTE* data = ((BYTE*)&xeData) + 0x10;
	if(!isEncrypted)
	{
		this->bIsDecrypted = TRUE;
		/*
#ifndef _XBOX
		// byteswap stuff
		this->xeData.w4OddFeatures = bswap16(this->xeData.w4OddFeatures);
		this->xeData.w5OddAuthType = bswap16(this->xeData.w5OddAuthType);
		this->xeData.w16GameRegion = bswap16(this->xeData.w16GameRegion);
		this->xeData.dw6RestrictedHvExtLoader = bswap32(this->xeData.dw6RestrictedHvExtLoader);
		this->xeData.dw7PolicyFlashSize = bswap32(this->xeData.dw7PolicyFlashSize);
		this->xeData.dw8PolicyBuiltInUsbMuSize = bswap32(this->xeData.dw8PolicyBuiltInUsbMuSize);
		this->xeData.dw9ReservedDword4 = bswap32(this->xeData.dw9ReservedDword4);
		this->xeData.qwARestrictedPrivileges = bswap64(this->xeData.qwARestrictedPrivileges);
		this->xeData.qwBReservedQword2 = bswap64(this->xeData.qwBReservedQword2);
		this->xeData.qwCReservedQword3 = bswap64(this->xeData.qwCReservedQword3);
		this->xeData.qwDReservedQword4 = bswap64(this->xeData.qwDReservedQword4);
		this->xeData.b36ConsoleCertificate.CertSize = bswap16(this->xeData.b36ConsoleCertificate.CertSize);
		this->xeData.b36ConsoleCertificate.ConsoleType = bswap32(this->xeData.b36ConsoleCertificate.ConsoleType);
		this->xeData.b36ConsoleCertificate.Privileges = bswap16(this->xeData.b36ConsoleCertificate.Privileges);
#endif*/ // DISABLED CUZ ITS BUGGED TO HELL
	}
	else
	{
		int res = this->Crypt(true);
		if(res == 0)
			return this->Load(false);
		else
			return 1;
	}
	Log(1, "CXeKeyVault::Load: loaded KeyVault\n");
	return 0;
}