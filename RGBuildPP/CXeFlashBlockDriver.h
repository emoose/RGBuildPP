#ifndef _CXeFLASHBLOCKDRIVER_H_
#define _CXeFLASHBLOCKDRIVER_H_

inline void Log(int priority, const char* szFormat, ...);

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
	
	errno_t CreateImage(DWORD dwLength, DWORD dwFlashConfig);
	errno_t InitSpare();

	errno_t OpenImage(PSZ szPath);
	errno_t SaveImage(PSZ szPath);

	errno_t OpenDevice();
	errno_t SaveDevice();

	errno_t OpenContinue(DWORD len, DWORD pagelen);
	errno_t LoadFlashConfig();

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

	BOOL IsSpareBadBlock(BYTE* sparebuff);
	BOOL IsMobileData(BYTE blockType);

	int ReadPageSpare(DWORD pageIdx, BYTE* buffer);
	int ReadBlockSpare(DWORD blockIdx, BYTE* buffer);
	int ReadLilBlockSpare(DWORD blockIdx, BYTE* buffer);

	void WritePageSpare(DWORD pageIdx, BYTE* sparebuff);
	void WriteBlockSpare(DWORD blockIdx, BYTE* sparebuff);
	void WriteLilBlockSpare(DWORD blockIdx, BYTE* sparebuff);

	int ReadBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	int ReadLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	int ReadLilBlockChain(WORD* pwChain, WORD wChainLength, BYTE* pbBuffer, DWORD dwSize);
	int Read(DWORD offset, BYTE* buffer, DWORD length);

	int WriteBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	int WriteLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length);
	int Write(DWORD offset, BYTE* buffer, DWORD length);

	int DetectSpareType();
	void CalculateEDC(UINT* data);
	CXeFlashBlockDriver(){dwFSOffset = 0; dwFlashConfig = 0; dwSpareType = 0; dwBlockCount = 0; dwBlockLength = 0; dwPageLength = 0;};

};

#endif