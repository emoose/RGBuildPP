#ifndef _CXeFLASHBLOCKDRIVER_H_
#define _CXeFLASHBLOCKDRIVER_H_

extern void Log(int priority, const char* szFormat, ...);

class CXeFlashBlockDriver
{
public:
	BYTE * pbImageData; // move back to private after tests

	DWORD dwSpareType;
	DWORD dwBlockCount;
	DWORD dwBlockLength;
	DWORD dwBlockLengthReal;
	DWORD dwLilBlockCount;
	DWORD dwLilBlockLength;
	DWORD dwLilBlockLengthReal;
	DWORD dwFlashConfig;
	DWORD dwPageLength;
	DWORD dwPageCount;
	DWORD dwImageLengthReal;
	DWORD dwFSOffset;
	DWORD dwReserveBlockIdx;
	DWORD dwConfigBlockIdx;
	DWORD dwPatchSlotLength;
	
	__checkReturn errno_t CreateImage(DWORD dwLength, DWORD dwFlashConfig);
	__checkReturn errno_t InitSpare();

	__checkReturn errno_t OpenImage(PSZ szPath);
	__checkReturn errno_t SaveImage(PSZ szPath);

	__checkReturn errno_t OpenDevice();
	__checkReturn errno_t SaveDevice();

	__checkReturn errno_t OpenContinue(DWORD len, DWORD pagelen);
	__checkReturn errno_t LoadFlashConfig();

	INT GetSpareSeqField(BYTE * sparebuff);
	WORD GetSpareIndexField(BYTE * sparebuff);
	BYTE GetSpareBlockTypeField(BYTE * sparebuff);
	WORD GetSpareSizeField(BYTE* sparebuff);
	BYTE GetSparePageCountField(BYTE* sparebuff);

	void SetSparePageCountField(BYTE* sparebuff, BYTE pgCount);
	void SetSpareBlockTypeField(BYTE* sparebuff, BYTE blkType);
	void SetSpareSeqField(BYTE* sparebuff, DWORD sequence);
	void SetSpareIndexField(BYTE* sparebuff, WORD blkIndex);
	void SetSpareSizeField(BYTE* sparebuff, WORD fsSize);
	void SetSpareBadBlock(BYTE* sparebuff, BOOL isBadBlock);

	__checkReturn BOOL IsSpareBadBlock(BYTE* sparebuff);
	__checkReturn BOOL IsMobileData(BYTE blockType);

	__checkReturn int ReadPageSpare(DWORD pageIdx, BYTE* buffer);
	__checkReturn int ReadBlockSpare(DWORD blockIdx, BYTE* buffer);
	__checkReturn int ReadLilBlockSpare(DWORD blockIdx, BYTE* buffer);

	void WritePageSpare(DWORD pageIdx, BYTE* sparebuff);
	void WriteBlockSpare(DWORD blockIdx, BYTE* sparebuff);
	void WriteLilBlockSpare(DWORD blockIdx, BYTE* sparebuff);

	__checkReturn int ReadBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	__checkReturn int ReadLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	__checkReturn int ReadLilBlockChain(WORD* pwChain, WORD wChainLength, BYTE* pbBuffer, DWORD dwSize);
	__checkReturn int Read(DWORD offset, BYTE* buffer, DWORD length);

	__checkReturn int WriteBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	__checkReturn int WriteLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	__checkReturn int Write(DWORD offset, BYTE* buffer, DWORD length);

	__checkReturn errno_t CreateDefaults(DWORD imgLen, DWORD pageLen, DWORD spareType, DWORD flashConfig, DWORD fsOffset);

	__checkReturn int DetectSpareType();
	void CalculateEDC(UINT* data);
	CXeFlashBlockDriver(){dwFSOffset = 0; dwFlashConfig = 0; dwSpareType = 0; dwBlockCount = 0; dwBlockLength = 0; dwPageLength = 0;};

};

#endif