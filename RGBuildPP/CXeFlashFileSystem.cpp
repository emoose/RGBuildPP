#include "stdafx.h"
#include "CXeFlashFileSystem.h"
#include "util.h"
BOOL CXeFlashFileSystemRoot::ChainAllocBlock(PWORD pwBlkIdx)
{
	return this->ChainAllocBlocks(1, 0, pwBlkIdx);
}
BOOL CXeFlashFileSystemRoot::ChainAllocBlocks(WORD wBlksNeeded, WORD wMinBlk, PWORD pwBlkIdx)
{
	for(WORD i = wMinBlk; i < this->xepBlkDriver->dwLilBlockCount; i++)
	{
		BOOL cont = false;
		for(WORD x = 0; x < wBlksNeeded; x++)
			if((this->pwBlockMap[x+i] & 0x7fff) != 0x1ffe)
			{
				cont = true;
				break;
			}
		if(cont)
			continue;

		// allocate cluster and return it
		this->pwBlockMap[i] = 0x1fff;
		*pwBlkIdx = i;
		return true;
	}
	return false;
}
void CXeFlashFileSystemRoot::ChainFreeChain(WORD wBlkIdx)
{
	WORD chainlength = 0;
	WORD* chain = this->ChainGetChain(wBlkIdx, &chainlength);
	for(int i = 0; i < chainlength; i++)
	{
		this->pwBlockMap[chain[i]] = 0x1ffe;
	}
}
WORD * CXeFlashFileSystemRoot::ChainGetChain(WORD wStartBlock, PWORD pwChainLength)
{
	return this->ChainGetChain(wStartBlock, 0x600, pwChainLength);
}
WORD * CXeFlashFileSystemRoot::ChainGetChain(WORD wStartBlock, DWORD limit, PWORD pwChainLength)
{
	// first lets get length of this chain
	int num = 1;
	WORD block = wStartBlock;
	while(true)
	{
		block = this->pwBlockMap[block];
		block = block & 0x7fff;
		if((block & 0x1ffe) == 0x1ffe || num == limit)
			break;
		num++;
	}
	if(num == limit)
		return NULL; // must have broken

	// now lets malloc the chain
	WORD * wChain = (WORD*)malloc(num * sizeof(WORD));
	block = wStartBlock;
	for(int i = 0; i < num; i++)
	{
		wChain[i] = block;
		block = this->pwBlockMap[block];
		block = block & 0x7fff;
		if((block & 0x1ffe) == 0x1ffe || num == limit)
			break;
	}

	*pwChainLength = num;
	return wChain;
}
DWORD CXeFlashFileSystemRoot::ChainSetChainData(WORD wStartBlock, BYTE* pbData, DWORD cbData)
{
	WORD chainlength = 0;
	WORD* chain = this->ChainGetChain(wStartBlock, &chainlength);
	WORD blocksneeded = cbData / this->xepBlkDriver->dwLilBlockLength + ((cbData % this->xepBlkDriver->dwLilBlockLength) > 0 ? 1 : 0);
	DWORD wrote = 0;
	if(chainlength == blocksneeded)
	{
		for(WORD i = 0; i < blocksneeded; i++)
		{
			DWORD towrite = this->xepBlkDriver->dwLilBlockLength;
			if(i+1 == blocksneeded)
				towrite = cbData - wrote;
			this->xepBlkDriver->WriteLilBlock(chain[i], pbData + wrote, towrite);
			wrote += towrite;
		}
		return wrote;
	}
	if(chainlength < blocksneeded)
	{
		// looks like we need to allocate some
		WORD blockstoalloc = blocksneeded - chainlength;
		WORD currentBlk = chain[chainlength-1];
		for(WORD x = 0; x < blockstoalloc; x++)
		{
			if(!this->ChainAllocBlock(&this->pwBlockMap[currentBlk]))
			{
				// errrrror!
			}
			else
			{
				currentBlk = this->pwBlockMap[currentBlk];
			}
		}
		return this->ChainSetChainData(wStartBlock, pbData, cbData);
	}
	// deallocation time!
	// free blocks after the amount we need
	this->ChainFreeChain(chain[blocksneeded-1]);

	// set last block to 0x1fff
	this->pwBlockMap[chain[blocksneeded-1]] = 0x1fff;

	// re-run
	return this->ChainSetChainData(wStartBlock, pbData, cbData);	
}
void CXeFlashFileSystemRoot::FileSetData(FLASHFILESYSTEM_ENTRY* fsEntry, BYTE* pbData, DWORD cbData)
{
	WORD chainlength = 1;
}
BYTE * CXeFlashFileSystemRoot::FileGetData(FLASHFILESYSTEM_ENTRY* fsEntry)
{
	// lets get the chain
	WORD chainlength = 0;
	WORD * chain = this->ChainGetChain(fsEntry->wBlockNumber, &chainlength);
	BYTE* buffer = (BYTE*)malloc(fsEntry->dwLength);
	this->xepBlkDriver->ReadLilBlockChain(chain, chainlength, buffer, fsEntry->dwLength);
	free((void*)chain);
	return buffer;
}
FLASHFILESYSTEM_ENTRY* CXeFlashFileSystemRoot::FindEntry(PSZ szName)
{
	for(DWORD i = 0; i < this->fscbEntries; i++)
		if(!strcmp(this->pfsEntries[i].cFileName, szName))
			return &this->pfsEntries[i];

	return NULL;
}
errno_t CXeFlashFileSystemRoot::Load()
{
	int pgcount = this->cbData / 512;
	DWORD * blkMapPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
	DWORD * fEntryPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
	DWORD blNum = 0;
	DWORD fsNum = 0;
	Log(0, "loading filesystem...\n");
	for(int i = 0; i < pgcount; i++)
	{
		if(i%2 == 0)
		{
			blkMapPages[blNum] = i;
			blNum++;
		}
		else
		{
			fEntryPages[fsNum] = i;
			fsNum++;
		}
	}
	// alloc our blockmap
	this->pwBlockMap = (WORD*)malloc((blNum * 0x100) * sizeof(WORD));
	Log(0, "loading blockmap...\n");
	// read blockmap
	for(DWORD i = 0; i < blNum; i++)
	{
		BYTE* bmapdata = this->pbData + (blkMapPages[i] * 0x200);
		for(int y = 0; y < 0x100; y++) // 0x100 is max blocks per page
			this->pwBlockMap[y + (i*0x100)] = bswap16(*(WORD*)(bmapdata + (y * 2)));
	}
	// alloc our filename tables
	this->pfsEntries = (FLASHFILESYSTEM_ENTRY*)malloc((fsNum * 0x10) * sizeof(FLASHFILESYSTEM_ENTRY));
	
	Log(0, "loading file entries...\n");
	// *deep breath* read filenames
	for(DWORD i = 0; i < fsNum; i++)
	{
		BYTE* bmapdata = this->pbData + (fEntryPages[i] * 0x200);
		for(int y = 0; y < 0x10; y++) // 0x20 is max file entries per page
		{
			memcpy((VOID*)&this->pfsEntries[y + (i*0x10)], (VOID*)(bmapdata + (y * sizeof(FLASHFILESYSTEM_ENTRY))), sizeof(FLASHFILESYSTEM_ENTRY));
			this->pfsEntries[y + (i*0x10)].dwLength = bswap32(this->pfsEntries[y + (i*0x10)].dwLength);
			this->pfsEntries[y + (i*0x10)].dwTimeStamp = bswap32(this->pfsEntries[y + (i*0x10)].dwTimeStamp);
			this->pfsEntries[y + (i*0x10)].wBlockNumber = bswap16(this->pfsEntries[y + (i*0x10)].wBlockNumber);
		}
	}
	
	Log(0, "validating file entries...\n");
	this->fscbEntries = 0;
	for(DWORD i = 0; i < (fsNum * 0x10); i++)
	{
		if(this->pfsEntries[i].wBlockNumber == 0x0 || this->pfsEntries[i].dwLength == 0x0)
			break;
		if(this->pfsEntries[i].cFileName[0] == 0x05)
			this->pfsEntries[i].cFileName[0] = '_';
		this->fscbEntries++;
	}
	return 0;
}