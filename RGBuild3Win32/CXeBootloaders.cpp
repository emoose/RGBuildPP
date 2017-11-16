#include "stdafx.h"
#include "CXeBootloaders.h"
void DbgPrint(const char* szFormat, ...)
{
#ifndef _XBOX
	char szBuff[1024];
	va_list arg;
	va_start(arg, szFormat);
	_vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
	va_end(arg);
	OutputDebugString(szBuff);
#else
	printf(szFormat, ...);
#endif
}
int CXeBootloader::Load()
{
	this->blHdr.wMagic = (WORD*)pbData;
	this->blHdr.wBuild = (WORD*)pbData + 2;
	this->blHdr.wQfe = (WORD*)pbData + 4;
	this->blHdr.wFlags = (WORD*)pbData + 6;
	this->blHdr.dwEntrypoint = (DWORD*)pbData + 8;
	this->blHdr.dwLength = (DWORD*)pbData + 0xC;
	printf("whee!");
	return 1;
}
int CXeBootloader_2BL::Load()
{
	void* data = pbData + 0x20;
	this->blHdr.blPerBoxData = (PBOOTLOADER_2BL_PERBOX)data;
	this->blHdr.bSignature = (BYTE*)data + 0x20;
	this->blHdr.bAesInvData = (BYTE*)data + 0x120;
	this->blHdr.ulPostOutputAddr = (ULONG*)data + 0x230;
	this->blHdr.ulSbFlashAddr = (ULONG*)data + 0x238;
	this->blHdr.ulSocMmioAddr = (ULONG*)data + 0x240;
	this->blHdr.bRsaPublicKey = (BYTE*)data + 0x248;
	this->blHdr.b3BLNonce = (BYTE*)data + 0x358;
	this->blHdr.b3BLSalt = (BYTE*)data + 0x368;
	this->blHdr.b4BLSalt = (BYTE*)data + 0x372;
	this->blHdr.b4BLDigest = (BYTE*)data + 0x37C;
	this->blHdr.blAllowData = (PBOOTLOADER_2BL_ALLOWDATA)data + 0x390;
	this->blHdr.dwPadding = (DWORD*)data + 0x394;
	printf("whoa!");
	return 1;
}