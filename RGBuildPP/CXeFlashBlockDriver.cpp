#include "stdafx.h"
#include "CXeFlashBlockDriver.h"
#include "util.h"
#ifdef _XBOX
#include "SFCX.h"
#endif
BOOL CXeFlashBlockDriver::IsMobileData(BYTE blockType)
{
	if((blockType & 0x30) != 0x30)
		return FALSE;
	if(blockType >= 0x31 && blockType < 0x3A)
		return TRUE;
	return FALSE;
}
void CXeFlashBlockDriver::SetSpareBadBlock(BYTE* sparebuff, BOOL isBadBlock)
{
	if(dwSpareType == 2)
		*(sparebuff) = (isBadBlock ? 0x0 : 0xFF);
	else
		*(sparebuff + 0x5) = (isBadBlock ? 0x0 : 0xFF);
}
BOOL CXeFlashBlockDriver::IsSpareBadBlock(BYTE* sparebuff)
{
	if(dwSpareType == 2)
		return *(sparebuff) != 0xFF;
	else
		return *(sparebuff + 0x5) != 0xFF;
}
void CXeFlashBlockDriver::SetSparePageCountField(BYTE* sparebuff, BYTE pgCount)
{
	*(sparebuff + 0x9) = pgCount;
}
void CXeFlashBlockDriver::SetSpareBlockTypeField(BYTE* sparebuff, BYTE blkType)
{
	*(sparebuff + 0xC) = blkType;
}
void CXeFlashBlockDriver::SetSpareSeqField(BYTE* sparebuff, DWORD sequence)
{
	BYTE seq0 = sequence & 0xFF;
	BYTE seq1 = (sequence >> 8) & 0xFF;
	BYTE seq2 = (sequence >> 16) & 0xFF;
	BYTE seq3 = (sequence >> 24) & 0xFF;
	switch(dwSpareType)
	{
	case 0:
		*(sparebuff + 0x2) = seq0;
		*(sparebuff + 0x3) = seq1;
		*(sparebuff + 0x4) = seq2;
		*(sparebuff + 0x6) = seq3;
		break;
	case 1:
		*(sparebuff + 0x0) = seq0;
		*(sparebuff + 0x3) = seq1;
		*(sparebuff + 0x4) = seq2;
		*(sparebuff + 0x6) = seq3;
		break;
	case 2:
		*(sparebuff + 0x5) = seq0;
		*(sparebuff + 0x4) = seq1;
		*(sparebuff + 0x3) = seq2;
		break;
	}
}
void CXeFlashBlockDriver::SetSpareIndexField(BYTE* sparebuff, WORD blkIndex)
{
	BYTE idx1 = blkIndex & 0xFF;
	BYTE idx0 = (blkIndex >> 8) & 0xFF;
	switch(dwSpareType)
	{
	case 0:
		*(sparebuff + 0x1) = idx0;
		*(sparebuff + 0x0) = idx1;
		break;
	case 1:
	case 2:
		*(sparebuff + 0x2) = idx0;
		*(sparebuff + 0x1) = idx1;
		break;
	}
}
void CXeFlashBlockDriver::SetSpareSizeField(BYTE * sparebuff, WORD fsSize)
{
	BYTE sz0 = fsSize & 0xFF;
	BYTE sz1 = (fsSize >> 8) & 0xFF;
	*(sparebuff + 0x7) = sz0;
	*(sparebuff + 0x8) = sz1;
}
WORD CXeFlashBlockDriver::GetSpareIndexField(BYTE * sparebuff)
{
	BYTE idx0 = 0;
	BYTE idx1 = 0;
	switch(dwSpareType)
	{
	case 0:
		idx0 = *(sparebuff + 0x1) & 0xF;
		idx1 = *(sparebuff + 0x0);
		break;
	case 1:
	case 2:
		idx0 = *(sparebuff + 0x2) & 0xF;
		idx1 = *(sparebuff + 0x1);
		break;
	}
	return (idx0 << 8) + idx1;
}

WORD CXeFlashBlockDriver::GetSpareSizeField(BYTE* sparebuff)
{
	return (*(sparebuff + 0x8) << 8) + (*(sparebuff + 0x7));
}
BYTE CXeFlashBlockDriver::GetSparePageCountField(BYTE* sparebuff)
{
	return *(sparebuff + 0x9);
}
BYTE CXeFlashBlockDriver::GetSpareBlockTypeField(BYTE* sparebuff)
{
	return *(sparebuff + 0xC);
}
INT CXeFlashBlockDriver::GetSpareSeqField(BYTE* sparebuff)
{
	BYTE seq0 = 0;
	BYTE seq1 = 0;
	BYTE seq2 = 0;
	BYTE seq3 = 0;
	switch(dwSpareType)
	{
	case 0:
		seq0 = *(sparebuff + 0x2);
		seq1 = *(sparebuff + 0x3);
		seq2 = *(sparebuff + 0x4);
		seq3 = *(sparebuff + 0x6);
		break;
	case 1:
		seq0 = *(sparebuff + 0x0);
		seq1 = *(sparebuff + 0x3);
		seq2 = *(sparebuff + 0x4);
		seq3 = *(sparebuff + 0x6);
		break;
	case 2:
		seq0 = *(sparebuff + 0x5);
		seq1 = *(sparebuff + 0x4);
		seq2 = *(sparebuff + 0x3);
		break;
	}
	return (seq3 << 24) + (seq2 << 16) + (seq1 << 8) + seq0;
}

void CXeFlashBlockDriver::WriteBlockSpare(DWORD blockIdx, BYTE* sparebuff)
{
	// should probably loop for every page in the block
	memcpy(pbImageData + (blockIdx * this->dwBlockLengthReal) + 0x200, sparebuff, 0x10);
}

void CXeFlashBlockDriver::WriteLilBlockSpare(DWORD blockIdx, BYTE* sparebuff)
{
	// should probably loop for every page in the block
	memcpy(pbImageData + (blockIdx * this->dwLilBlockLengthReal) + 0x200, sparebuff, 0x10);
}

void CXeFlashBlockDriver::WritePageSpare(DWORD pageIdx, BYTE* sparebuff)
{
	memcpy(pbImageData + (pageIdx * this->dwPageLength) + 0x200, sparebuff, 0x10);
}

int CXeFlashBlockDriver::ReadBlockSpare(DWORD dwBlockIdx, BYTE* buffer)
{
	memcpy(buffer, pbImageData + (dwBlockIdx * this->dwBlockLengthReal) + 0x200, 0x10);
	return 0;
}
int CXeFlashBlockDriver::ReadLilBlockSpare(DWORD dwBlockIdx, BYTE* buffer)
{
	memcpy(buffer, pbImageData + (dwBlockIdx * this->dwLilBlockLengthReal) + 0x200, 0x10);
	return 0;
}
int CXeFlashBlockDriver::ReadPageSpare(DWORD dwPageIdx, BYTE* buffer)
{
	memcpy(buffer, pbImageData + (dwPageIdx * this->dwPageLength) + 0x200, 0x10);
	return 0;
}
int CXeFlashBlockDriver::ReadBlock(DWORD blkIdx, BYTE* buffer, DWORD length)
{
	return this->Read(blkIdx * this->dwBlockLength, buffer, length);
}
int CXeFlashBlockDriver::ReadLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length)
{
	return this->Read(blkIdx * this->dwLilBlockLength, buffer, length);
}
int CXeFlashBlockDriver::ReadLilBlockChain(WORD* pwChain, WORD wChainLength, BYTE* pbBuffer, DWORD dwSize)
{	DWORD i = 0;
	DWORD blockcount = dwSize / this->dwLilBlockLength + (dwSize % this->dwLilBlockLength > 0 ? 1 : 0);
	DWORD toread = dwSize;
	DWORD totalread = 0;
	DWORD lastread = this->dwLilBlockLength;
	WORD currblock = pwChain[i];	
	while(toread > 0)
	{
		DWORD read = toread < this->dwLilBlockLength ? toread : this->dwLilBlockLength;
		read = this->ReadLilBlock(currblock + this->dwFSOffset, pbBuffer + (i * this->dwLilBlockLength), read);
		toread -= read;
		lastread = read;
		totalread += read;
		i++;
		currblock = pwChain[i];

			/*
			int read = toread < 512 ? toread : 512 - offintopage;
		BYTE* copyto = (buffer + (currpage * 512)) - (currpage > 0 ? (offset % 512) : 0);
		BYTE* copyfrom = pbImageData + (pageinimage * this->dwPageLength) + (currpage * this->dwPageLength) + offintopage;
		memcpy(copyto, copyfrom, read);
		offintopage = 0;
		toread -= read;
		lastread = read;
		totalread += read;
		currpage++;
		*/
	}
	return totalread;
}
int CXeFlashBlockDriver::WriteBlock(DWORD blkIdx, BYTE* buffer, DWORD length)
{
	return this->Write(blkIdx * this->dwBlockLength, buffer, length);
}
int CXeFlashBlockDriver::WriteLilBlock(DWORD blkIdx, BYTE* buffer, DWORD length)
{
	return this->Write(blkIdx * this->dwLilBlockLength, buffer, length);
}
int CXeFlashBlockDriver::Write(DWORD offset, BYTE* buffer, DWORD length)
{
	int offintopage = offset % 512;
	int pagecount = (length - offintopage) / 512 + ((length - offintopage) % 512 > 0 ? 1 : 0);
	int currpage = 0;
	int pageinimage = offset / 512;
	int towrite = length;
	int totalwrote = 0;
	int lastwrote = 512;
	// this should do calcs nd shit to make it into real page sizes
	if(offset >= this->dwImageLengthReal || offset + length > this->dwImageLengthReal)
		return 0;

	while(towrite > 0)
	{
		int write = towrite < 512 ? towrite : 512 - offintopage;
		BYTE* copyfrom = (buffer + (currpage * 512)) - (currpage > 0 ? (offset % 512) : 0);
		BYTE* copyto = pbImageData + (pageinimage * this->dwPageLength) + (currpage * this->dwPageLength) + offintopage;
		memcpy(copyto, copyfrom, write);
		offintopage = 0;
		towrite -= write;
		lastwrote = write;
		totalwrote += write;
		currpage++;
	}
	return totalwrote;
}
int CXeFlashBlockDriver::Read(DWORD offset, BYTE* buffer, DWORD length)
{
	int offintopage = offset % 512;
	int pagecount = (length - offintopage) / 512 + ((length - offintopage) % 512 > 0 ? 1 : 0);
	int currpage = 0;
	int pageinimage = offset / 512;
	int toread = length;
	int totalread = 0;
	int lastread = 512;
	// this should do calcs nd shit to make it into real page sizes
	if(offset >= this->dwImageLengthReal || offset + length > this->dwImageLengthReal)
		return 0;

	while(toread > 0)
	{
		int read = toread < 512 ? toread : 512 - offintopage;
		BYTE* copyto = (buffer + (currpage * 512)) - (currpage > 0 ? (offset % 512) : 0);
		BYTE* copyfrom = pbImageData + (pageinimage * this->dwPageLength) + (currpage * this->dwPageLength) + offintopage;
		memcpy(copyto, copyfrom, read);
		offintopage = 0;
		toread -= read;
		lastread = read;
		totalread += read;
		currpage++;
	}
	return totalread;
}
errno_t CXeFlashBlockDriver::InitSpare()
{
	// first lets set all the pages
	BYTE spare[0x10];
	memset((void*)&spare, 0, 0x10);
	this->SetSpareBadBlock((BYTE*)&spare, false);
	for(DWORD i = 0; i < this->dwPageCount; i++)
	{
		this->WritePageSpare(i, (BYTE*)&spare);
	}

	// now lets set all the block numbers
	// this is probably the critical part of big blocks not working
	memset((void*)&spare, 0, 0x10);
	for(DWORD i = 0; i < this->dwBlockCount; i++)
	{
		this->SetSpareBadBlock((BYTE*)&spare, false);
		this->SetSpareIndexField((BYTE*)&spare, (WORD)i);
		this->WriteBlockSpare(i, (BYTE*)&spare);
		memset((void*)&spare, 0, 0x10);
	}
	return 0;
}
errno_t CXeFlashBlockDriver::CreateImage(DWORD dwLength, DWORD dwFlashConfig)
{
	pbImageData = (BYTE*)malloc(dwLength);
	this->dwFlashConfig = dwFlashConfig;
	this->LoadFlashConfig();
	this->InitSpare();
	return 0;
}
void CXeFlashBlockDriver::CalculateEDC(UINT * data)
{
    unsigned int i=0, val=0;
    unsigned char *edc = ((unsigned char*)data) + 0x200;

    unsigned int v=0;

    for (i = 0; i < 0x1066; i++)
    {
        if (!(i & 31))
            v = ~bswap32Xe(*data++);
        val ^= v & 1;
        v>>=1;
        if (val & 1)
            val ^= 0x6954559;
        val >>= 1;
    }

    val = ~val;

    // 26 bit ecc data
    edc[0xC] = ((val << 6) | (edc[0xC] & 0x3F)) & 0xFF;//|= (val << 6) & 0xC0;
    edc[0xD] = (val >> 2) & 0xFF;
    edc[0xE] = (val >> 10) & 0xFF;
    edc[0xF] = (val >> 18) & 0xFF;
}
errno_t CXeFlashBlockDriver::SaveImage(PSZ szPath)
{
	// lets do a quick recalc of the ecc
	for(DWORD i = 0; i < this->dwPageCount; i++)
		this->CalculateEDC((UINT*)(this->pbImageData + (i * 0x210)));
	
	FILE * file;
	errno_t err = fopen_s(&file, szPath, "wb+");
	if(err != 0)
		return err;
	fwrite(this->pbImageData, 1, this->dwImageLengthReal, file);
	fclose(file);
	return 0;
}
errno_t CXeFlashBlockDriver::OpenImage(PSZ szPath)
{
	FILE * file;
	errno_t err = fopen_s(&file, szPath, "rb+");
	if(err != 0)
		return err;
	fseek(file, 0, SEEK_END);
	DWORD len = ftell(file);
	pbImageData = (BYTE*)malloc(len);
	fseek(file, 0, SEEK_SET);
	fread(pbImageData, 1, len, file);
	fclose(file);
	return this->OpenContinue(len, 528);
}
errno_t CXeFlashBlockDriver::SaveDevice()
{
	// we should probably switch to loading from blocks instead of from pages
#ifdef _XBOX
	Log(0, "SFCX init\n");
	int config = sfcx_init();
	if (sfc.initialized != SFCX_INITIALIZED)
	{
		Log(3, "NAND init failed!\n");
		return 1;
	}
	
	Log(0, "   NAND:page_sz         = %08X\n", sfc.page_sz);
	Log(0, "   NAND:meta_sz         = %08X\n", sfc.meta_sz);
	Log(0, "   NAND:page_sz_phys    = %08X\n", sfc.page_sz_phys);

	Log(0, "   NAND:pages_in_block  = %08X\n", sfc.pages_in_block);
	Log(0, "   NAND:block_sz        = %08X\n", sfc.block_sz);
	Log(0, "   NAND:block_sz_phys   = %08X\n", sfc.block_sz_phys);

	Log(0, "   NAND:size_mb         = %dMB\n", sfc.size_mb);
	Log(0, "   NAND:size_bytes      = %08X\n", sfc.size_bytes);
	Log(0, "   NAND:size_bytes_phys = %08X\n", sfc.size_bytes_phys);

	Log(0, "   NAND:size_pages      = %08X\n", sfc.size_pages);
	Log(0, "   NAND:size_blocks     = %08X\n\n", sfc.size_blocks);
	Log(0, "writing NAND...\n");
	DWORD len = sfc.size_bytes_phys;
	// only read 64MB
	if(len > 69206016) len = 69206016;
	for(int i = 0; i < (len / sfc.block_sz_phys); i++)
	{
		sfcx_erase_block(i * sfc.block_sz);
		sfcx_write_block(this->pbImageData + (i*sfc.block_sz_phys), i * sfc.block_sz);
		//sfcx_read_page(this->pbImageData + (i * sfc.page_sz_phys), (i * sfc.page_sz), 1);
		//sfcx_write_page(this->pbImageData + (i*sfc.page_sz_phys), (i * sfc.page_sz));
	}
	sfcx_reset();
	//this->dwFlashConfig = config;
	//return this->OpenContinue(len, sfc.page_sz_phys);
	return 0;
#endif
	return 2;
}
errno_t CXeFlashBlockDriver::OpenDevice()
{
#ifdef _XBOX
	Log(0, "SFCX init\n");
	int config = sfcx_init();
	if (sfc.initialized != SFCX_INITIALIZED)
	{
		Log(3, "NAND init failed!\n");
		return 1;
	}
	
	Log(0, "   NAND:page_sz         = %08X\n", sfc.page_sz);
	Log(0, "   NAND:meta_sz         = %08X\n", sfc.meta_sz);
	Log(0, "   NAND:page_sz_phys    = %08X\n", sfc.page_sz_phys);

	Log(0, "   NAND:pages_in_block  = %08X\n", sfc.pages_in_block);
	Log(0, "   NAND:block_sz        = %08X\n", sfc.block_sz);
	Log(0, "   NAND:block_sz_phys   = %08X\n", sfc.block_sz_phys);

	Log(0, "   NAND:size_mb         = %dMB\n", sfc.size_mb);
	Log(0, "   NAND:size_bytes      = %08X\n", sfc.size_bytes);
	Log(0, "   NAND:size_bytes_phys = %08X\n", sfc.size_bytes_phys);

	Log(0, "   NAND:size_pages      = %08X\n", sfc.size_pages);
	Log(0, "   NAND:size_blocks     = %08X\n\n", sfc.size_blocks);
	Log(0, "reading NAND...\n");
	DWORD len = sfc.size_bytes_phys;
	// only read 64MB
	if(len > 69206016) len = 69206016;
	this->pbImageData = (BYTE*)malloc(len);
	for(int i = 0; i < sfc.size_pages; i++)
	{
		//sfcx_erase_block(i * sfc.block_sz_phys);
		//sfcx_erase_page(
		sfcx_read_page(this->pbImageData + (i * sfc.page_sz_phys), (i * sfc.page_sz), 1);
	}
	sfcx_reset();
	this->dwFlashConfig = config;
	return this->OpenContinue(len, sfc.page_sz_phys);
#endif
	return 2;
}
int CXeFlashBlockDriver::DetectSpareType()
{
	WORD blockIdx = 0;
	// small block
	blockIdx = (this->pbImageData[0x4400 + 0x1] << 8) | (this->pbImageData[0x4400 + 0x0]);
	if (blockIdx == 1 && this->pbImageData[0x4400 + 0x5] == 0xFF)
		return 0;

	// big on small
    blockIdx = (this->pbImageData[0x4400 + 0x2] << 8) | (this->pbImageData[0x4400 + 0x1]);
	if (blockIdx == 1 && this->pbImageData[0x4400 + 0x5] == 0xFF)
		return 1;

	// big block
    blockIdx = (this->pbImageData[0x21200 + 0x2] << 8) | (this->pbImageData[0x21200 + 0x1]);
	if (blockIdx == 1 && this->pbImageData[0x21200 + 0x0] == 0xFF)
		return 2;
    return -1;
}
errno_t CXeFlashBlockDriver::OpenContinue(DWORD len, DWORD pagelen)
{
	this->dwPageLength = pagelen;
	this->dwImageLengthReal = len;
	this->dwSpareType = this->DetectSpareType();
	if(this->dwFlashConfig == 0)
	{
		DWORD reallen = (len / pagelen) * 512;
		// round up 16/64mb nands to nearest size
		if(reallen <= 16777216)
			reallen = 16777216;
		else if (reallen <= 67108864)
			reallen = 67108864;
		switch(reallen)
		{
			case 16777216:
				switch(this->dwSpareType)
				{
					case 0:
						this->dwFlashConfig = 0x01198010;  //All other 16M
						break;
					case 1:
						this->dwFlashConfig = 0x00023010;  //Jasper 16M new SB
						break;
					case 3:
						this->dwFlashConfig = 0x00023010;  //Trinity 16MB
						break;
				}
				break;

			case 67108864:
				switch(this->dwSpareType)
				{
					case 0:
						this->dwFlashConfig = 0x01198030;  //All other 64M
						break;
					case 1:
						this->dwFlashConfig = 0x00023010;  //Jasper 64M new SB
						break;
					case 2:
						this->dwFlashConfig = 0x00AA3020;	//Jasper 256/512 Large Block
						break;
				}
				break;
			case 268435456:
			case 268435456 - 0x80000:
				this->dwFSOffset = 0x2E0;
				this->dwFlashConfig = 0x008A3020;
				break;
			case 536870912:
			case 536870912 - 0x80000:
				this->dwFSOffset = 0xAE0;
				this->dwFlashConfig = 0x00AA3020;
				break;
			default:
				Log(3, "unknown NAND size!\n");
				Log(3, "handling as 16MB!\n");
				this->dwFlashConfig = 0x01198010;
				this->dwBlockLength = 0x4000;
				this->dwBlockCount = 0x400;
		}
	}
	return this->LoadFlashConfig();
}
errno_t CXeFlashBlockDriver::LoadFlashConfig()
{
	
	this->dwPatchSlotLength = 0x10000;
	this->dwReserveBlockIdx = 0x3E0;
	switch(this->dwFlashConfig >> 17 & 3)
	{
	case 0:
		this->dwSpareType = 0;
		switch(this->dwFlashConfig >> 4 & 3)
		{
		case 1:
			this->dwBlockLength = 0x4000;
			this->dwBlockCount = 0x400;
			break;
		case 3:
			this->dwBlockLength = 0x4000;
			this->dwBlockCount = 0x1000;
			this->dwReserveBlockIdx = 0xF80;
			break;
		}
		break;
	case 1:
	case 2:
		switch(this->dwFlashConfig >> 4 & 3)
		{
		case 0:
			this->dwBlockLength = 0x4000;
			this->dwBlockCount = 0x400;
			this->dwSpareType = 1;
			break;
		case 1:
			this->dwBlockLength = 0x4000;
			this->dwBlockCount = 0x1000;
			if(this->dwImageLengthReal == 16777216 || this->dwImageLengthReal == 17301504)
				this->dwBlockCount = 0x400; 
			this->dwSpareType = 1;
			break;
		case 2:
			this->dwBlockLength = 0x20000;
			this->dwBlockCount = this->dwImageLengthReal / ((this->dwBlockLength / 512) * this->dwPageLength);
			this->dwSpareType = 2;
			this->dwPatchSlotLength = 0x20000;
			this->dwReserveBlockIdx = 0x1E0;
		}
		break;
	}
	this->dwConfigBlockIdx = dwReserveBlockIdx - 4;
	this->dwLilBlockLength = 0x4000; // static?
	this->dwLilBlockCount = (this->dwBlockCount * this->dwBlockLength) / 0x4000;
	// calc our real block shit
	if(this->dwPageLength > 512)
	{
		this->dwLilBlockLengthReal = (this->dwLilBlockLength / 512) * 528;
		this->dwBlockLengthReal = (this->dwBlockLength / 512) * 528;
	}
	else
	{
		this->dwLilBlockLengthReal = this->dwLilBlockLength;
		this->dwBlockLengthReal = this->dwBlockLength;
	}
	this->dwPageCount = this->dwLilBlockCount * 0x20;
	Log(0, "page length:\t\t0x%04x\n", this->dwPageLength);
	Log(0, "nand length:\t\t0x%08x\n", this->dwImageLengthReal);
	Log(0, "nand data length:\t0x%08x\n", this->dwBlockCount * this->dwBlockLength);
	Log(0, "block length:\t\t0x%08x\n", this->dwBlockLengthReal);
	Log(0, "block data length:\t0x%08x\n", this->dwBlockLength);
	Log(0, "lilblock length:\t0x%08x\n", this->dwLilBlockLengthReal);
	Log(0, "lilblock data length:\t0x%08x\n", this->dwLilBlockLength);
	Log(0, "block count:\t\t0x%04x\n", this->dwBlockCount);
	Log(0, "lilblock count:\t0x%04x\n", this->dwLilBlockCount);
	Log(0, "page count:\t\t0x%04x\n", this->dwPageCount);
	Log(0, "flashconfig:\t\t0x%08x\n", this->dwFlashConfig);
	Log(0, "spare type:\t\t%d\n", this->dwSpareType);
	Log(0, "config block index:\t0x%x\n", this->dwConfigBlockIdx);
	Log(0, "reserve block index:\t0x%x\n\n", this->dwReserveBlockIdx);
	return 0;
}