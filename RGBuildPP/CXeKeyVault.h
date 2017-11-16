#ifndef _CXEKEYVAULT_H
#define _CXEKEYVAULT_H
typedef struct _KV_CONTROLLER_DATA
{
	DWORD dwKey1Idx;
	DWORD dwKey2Idx;
	BYTE dwKey1Data[0x10];
	BYTE dwKey2Data[0x10];
} KV_CONTROLLER_DATA, *PKV_CONTROLLER_DATA;

typedef struct _CONSOLE_PUBLIC_KEY {
   DWORD  PublicExponent;                   // 0x00-0x04
   BYTE   Modulus[0x80];                    // 0x04-0x84
} CONSOLE_PUBLIC_KEY, *PCONSOLE_PUBLIC_KEY; // 0x84

typedef struct _XE_CONSOLE_CERTIFICATE {
   WORD               CertSize;                     // 0x00-0x02
   BYTE               ConsoleId[0x5];               // 0x02-0x07
   CHAR               ConsolePartNumber[0xB];       // 0x07-0x12
   BYTE               Reserved[0x4];                // 0x12-0x16
   WORD               Privileges;                   // 0x16-0x18
   DWORD              ConsoleType;                  // 0x18-0x1C
   //unsigned int       ManufacturingDate[2];         // 0x1C-0x24
   CHAR               ManufacturingDate[8];
   CONSOLE_PUBLIC_KEY ConsolePublicKey;             // 0x24-0xA8
   BYTE               Signature[0x100];             // 0xA8-0x1A8
} XE_CONSOLE_CERTIFICATE, *PXE_CONSOLE_CERTIFICATE; // 0x1A8

typedef struct _XE_KEYVAULT_DATA {
	BYTE bKeyVaultNonce[0x10];							// 0x0
	BYTE bKeyVaultPairData[0x8];						// 0x10 - not sure what this really is, seems to be first 8 bytes of bootloader/secured hmacsha-rc4 file nonces
														// which on stock are all equal to each other and have the pairing data from CB as first 3 bytes (observed on stock trinity nand)
														// could just be random data, and MS got lazy with randomizing stuff on newer models?

	BYTE b0ManufacturingMode;							// 0x18
	BYTE b1AlternativeKeyVault;							// 0x19
	BYTE b2RestrictedPrivilegesFlags;					// 0x1A
	BYTE b3ReservedByte3;								// 0x1B
	WORD w4OddFeatures;									// 0x1C
	WORD w5OddAuthType;									// 0x1E
	DWORD dw6RestrictedHvExtLoader;						// 0x20
	DWORD dw7PolicyFlashSize;							// 0x24
	DWORD dw8PolicyBuiltInUsbMuSize;					// 0x28
	DWORD dw9ReservedDword4;							// 0x2C
	ULONGLONG qwARestrictedPrivileges;					// 0x30
	ULONGLONG qwBReservedQword2;						// 0x38
	ULONGLONG qwCReservedQword3;						// 0x40
	ULONGLONG qwDReservedQword4;						// 0x48
	BYTE bEReservedKey1[0x10];							// 0x50
	BYTE bFReservedKey2[0x10];							// 0x60
	BYTE b10ReservedKey3[0x10];							// 0x70
	BYTE b11ReservedKey4[0x10];							// 0x80
	BYTE b12ReservedRandomKey1[0x10];					// 0x90
	BYTE b13ReservedRandomKey2[0x10];					// 0xA0
	CHAR sz14ConsoleSerialNumber[0xC];					// 0xB0
	DWORD dw14Padding;									// 0xBC
	BYTE b15MoboSerialNumber[0x8];						// 0xC0
	WORD w16GameRegion;									// 0xC8
	BYTE b16Padding[6];									// 0xCA
	BYTE b17ConsoleObfuscationKey[0x10];				// 0xD0
	BYTE b18KeyObfuscationKey[0x10];					// 0xE0
	BYTE b19RoamableObfuscationKey[0x10];				// 0xF0
	BYTE b1ADvdKey[0x10];								// 0x100
	BYTE b1BPrimaryActivationKey[0x18];					// 0x110
	BYTE b1CSecondaryActivationKey[0x10];				// 0x128
	BYTE b1DGlobalDevice2DesKey1[0x10];					// 0x138
	BYTE b1EGlobalDevice2DesKey2[0x10];					// 0x148
	BYTE b1FWirelessControllerMS2DesKey1[0x10];			// 0x158
	BYTE b20WirelessControllerMS2DesKey2[0x10];			// 0x168
	BYTE b21WiredWebcamMS2DesKey1[0x10];				// 0x178
	BYTE b22WiredWebcamMS2DesKey2[0x10];				// 0x188
	BYTE b23WiredControllerMS2DesKey1[0x10];			// 0x198
	BYTE b24WiredControllerMS2DesKey2[0x10];			// 0x1A8
	BYTE b25MemoryUnitMS2DesKey1[0x10];					// 0x1B8
	BYTE b26MemoryUnitMS2DesKey2[0x10];					// 0x1C8
	BYTE b27OtherXSM3DeviceMS2DesKey1[0x10];			// 0x1D8
	BYTE b28OtherXSM3DeviceMS2DesKey2[0x10];			// 0x1E8
	BYTE b29WirelessController3P2DesKey1[0x10];			// 0x1F8
	BYTE b2AWirelessController3P2DesKey2[0x10]; 		// 0x208
	BYTE b2BWiredWebcam3P2DesKey1[0x10];				// 0x218
	BYTE b2CWiredWebcam3P2DesKey2[0x10];				// 0x228
	BYTE b2DWiredController3P2DesKey1[0x10];			// 0x238
	BYTE b2EWiredController3P2DesKey2[0x10];			// 0x248
	BYTE b2FMemoryUnit3P2DesKey1[0x10];					// 0x258
	BYTE b30MemoryUnit3P2DesKey2[0x10];					// 0x268
	BYTE b31OtherXSM3Device3P2DesKey1[0x10];			// 0x278
	BYTE b32OtherXSM3Device3P2DesKey2[0x10];			// 0x288
	BYTE b33ConsolePrivateKey[0x1D0];					// 0x298
	BYTE b34XeikaPrivateKey[0x390];						// 0x468
	BYTE b35CardeaPrivateKey[0x1D0];					// 0x7F8
	XE_CONSOLE_CERTIFICATE b36ConsoleCertificate;		// 0x9C8
	BYTE b37XeikaCertificate[0x142];					// 0xB72
	BYTE b37Padding[0x1146];							// 0xCB2
	BYTE b39SpecialKeyVaultSignature[0x100];			// 0x1DF8
	BYTE b38CardeaCertificate[0x2108];					// 0x1EF8
} XE_KEYVAULT_DATA, *PXE_KEYVAULT_DATA;

class CXeKeyVault
{
public:
	XE_KEYVAULT_DATA xeData;
	BYTE * pbCPUKey;
	BYTE bRc4Key[0x10];
	BYTE * pbHmacShaNonce;
	WORD * pwKeyVaultVersion; // pointer to KeyVaultVersion field in flash header
	BOOL bIsDecrypted;

	__checkReturn errno_t RandomizeKeys();
	__checkReturn errno_t RepairDesKeys();
	__checkReturn errno_t Crypt(BOOL isDecrypting);
	__checkReturn errno_t Load(BOOL isEncrypted);
	__checkReturn errno_t Save(BOOL saveEncrypted);
	__checkReturn errno_t CalculateNonce(BYTE* pbNonceBuff, DWORD cbNonceBuff);
	CXeKeyVault(){pbCPUKey = 0; pbHmacShaNonce = 0;};
};

typedef struct _XE_FCRT_DATA
{
	BYTE bSignature[0x100];
	// signature is made from hmac sha of next 0x40 bytes
	BYTE bAesIv[0x10];
	DWORD dwUnknown; // this & 0x7ffffffe must be 0
	DWORD dwUnknown2; // must be 1
	DWORD dwDataLength; // can't be 0 or higher than 0x3ec0
	DWORD dwDataOffset; // must be higher than 0x140
	BYTE bUnknown[0xC];
	BYTE bDigest[0x14];
	BYTE bData[0x3ec0];
} XE_FCRT_DATA, *PXE_FCRT_DATA;

typedef struct _XE_CERTIFICATE_REVOCATION_DATA
{
	DWORD dwLength;
	DWORD dwVersion;
	DWORD dwCount;
	BYTE bRevokedDigests[0x884]; // each 0x14 is another entry, digest is sha1 of console security cert?
} XE_CERTIFICATE_REVOCATION_DATA, *PXE_CERTIFICATE_REVOCATION_DATA;

typedef struct _XE_CERTIFICATE_REVOCATION_BOX_DATA
{
	BYTE bFileTimestamp[0x8]; // should match crl.bin timestamp/meta in nand
	BYTE bUnknown[0x7];
	BYTE bLockDownValue;
} XE_CERTIFICATE_REVOCATION_BOX_DATA, *PXE_CERTIFICATE_REVOCATION_BOX_DATA;


typedef struct _XE_CRL_DATA
{
	DWORD dwMagic; // CRLP / CRLL
	BYTE bConsoleId[0x5];
	BYTE bPadding[0x3];
	BYTE bDigest[0x14];
	BYTE bSignature[0x100]; // CRLP = XEKEY_CONSTANT_PIRS_KEY, CRLL = XEKEY_CONSTANT_LIVE_KEY
	BYTE bAesNonce[0x10];
	BYTE bAesKey1[0x10]; // decrypt using AesEcb with CPU key
	// decrypt following with AesCbc using bAesNonce
	XE_CERTIFICATE_REVOCATION_BOX_DATA xeBoxData;
	// decrypt with AesCbc and updated IV? TODO: check this shit out
	XE_CERTIFICATE_REVOCATION_DATA xeData;
} XE_CRL_DATA, *PXE_CRL_DATA;


typedef struct _XE_SEC_DATA
{
	BYTE bPairingData[0x3];
	BYTE bPadding[0x3];
	BYTE bSecurityInitialised;
	BYTE bLockDownValue;
	BYTE bFileTimestamp[0x8]; // should match crl.bin timestamp/meta in nand
	BYTE bDetectedViolations;
	ULONGLONG qwSecurityActivated;
	ULONGLONG qwDvdDisconnectedCount;
	ULONGLONG qwLockSystemUpdateCount;
	BYTE WhateverMan[0x4000];
} XE_SEC_DATA, *PXE_SEC_DATA;

typedef struct _XE_EXTENDED_KV_DATA
{
	BYTE WhateverMan[0x4000];
} XE_EXTENDED_KV_DATA, *PXE_EXTENDED_KV_DATA;

typedef struct _XE_DAE_DATA
{
	BYTE WhateverMan[0x4000];
} XE_DAE_DATA, *PXE_DAE_DATA;

class CXeFlashSecuredFiles
{
public:
	XE_FCRT_DATA xeFcrtData;
	XE_SEC_DATA xeSecData;
	XE_EXTENDED_KV_DATA xeExtKVData;
	XE_DAE_DATA xeDaeData;
	XE_CRL_DATA xeCrlData;
	BYTE * pbCPUKey;

};
#endif