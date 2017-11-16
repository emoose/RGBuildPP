#include "stdafx.h"
#include "CXeFlashFileSystem.h"
#include "util.h"
__checkReturn errno_t CXeFlashFileSystemRoot::ChainAllocBlock(PWORD pwBlkIdx)
{
	return this->ChainAllocBlocks(1, 0, pwBlkIdx);
}
__checkReturn errno_t CXeFlashFileSystemRoot::ChainAllocBlocks(WORD wBlksNeeded, WORD wMinBlk, PWORD pwBlkIdx)
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
		return 0;
	}
	return 1;
}
void CXeFlashFileSystemRoot::ChainFreeChain(WORD wBlkIdx)
{
	WORD chainlength = 0;
	WORD* chain = this->ChainGetChain(wBlkIdx, &chainlength);
	for(int i = 0; i < chainlength; i++)
	{
		this->pwBlockMap[chain[i]] = 0x1ffe;
	}
	free(chain);
}
WORD * CXeFlashFileSystemRoot::ChainGetFromStart(WORD wStartBlock, PWORD pwChainLength)
{
	WORD startBlk = 0;
	WORD currBlk = wStartBlock;
	do
	{
		startBlk = currBlk;
		currBlk = this->ChainGetPrevious(currBlk);
	} while(currBlk > 0);
	if(startBlk == 0)
		return NULL;
	return this->ChainGetChain(startBlk, pwChainLength);
}

WORD CXeFlashFileSystemRoot::ChainGetPrevious(WORD wBlkIdx)
{
	for(DWORD i = 0; i < this->cbBlockMap; i++)
	{
		if(this->pwBlockMap[i] == wBlkIdx)
			return (WORD)i;
	}
	return 0;
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
	WORD blocksneeded = (WORD)(cbData / this->xepBlkDriver->dwLilBlockLength + ((cbData % this->xepBlkDriver->dwLilBlockLength) > 0 ? 1 : 0));
	DWORD wrote = 0;
	if(chainlength == blocksneeded)
	{
		for(WORD i = 0; i < blocksneeded; i++)
		{
			DWORD towrite = this->xepBlkDriver->dwLilBlockLength;
			if(i+1 == blocksneeded)
				towrite = cbData - wrote;
			this->xepBlkDriver->WriteLilBlock(chain[i] + this->xepBlkDriver->dwFSOffset, pbData + wrote, towrite);
			wrote += towrite;
		}
		free(chain);
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
				Log(3, "CXeFlashFileSystemRoot::ChainSetChainData: failed to allocate block!\n");
				free(chain);
				return 0;
			}
			currentBlk = this->pwBlockMap[currentBlk];
		}
		free(chain);
		return this->ChainSetChainData(wStartBlock, pbData, cbData);
	}
	// deallocation time!
	// free blocks after the amount we need
	this->ChainFreeChain(chain[blocksneeded-1]);

	// set last block to 0x1fff
	this->pwBlockMap[chain[blocksneeded-1]] = 0x1fff;

	free(chain);
	// re-run
	return this->ChainSetChainData(wStartBlock, pbData, cbData);	
}
FLASHFILESYSTEM_ENTRY* CXeFlashFileSystemRoot::FileSearch(PSZ szFileName)
{
	for(DWORD i = 0; i < this->cfsEntries; i++)
	{
		if(!strcmp(this->pfsEntries[i].cFileName, szFileName))
			return &this->pfsEntries[i];
	}
	return NULL;
}
__checkReturn errno_t CXeFlashFileSystemRoot::FileSetData(FLASHFILESYSTEM_ENTRY* fsEntry, BYTE* pbData, DWORD cbData)
{
	if(fsEntry->wBlockNumber == 0) // allocate it a block
		if(!this->ChainAllocBlock(&fsEntry->wBlockNumber))
			return 1;
	if(this->ChainSetChainData(fsEntry->wBlockNumber, pbData, cbData) != cbData)
		return 2;
	fsEntry->dwLength = cbData;
	return 0;
}
BYTE * CXeFlashFileSystemRoot::FileGetData(FLASHFILESYSTEM_ENTRY* fsEntry)
{
	// lets get the chain
	WORD chainlength = 0;
	WORD * chain = this->ChainGetChain(fsEntry->wBlockNumber, &chainlength);
	BYTE* buffer = (BYTE*)malloc(fsEntry->dwLength);
	this->xepBlkDriver->ReadLilBlockChain(chain, chainlength, buffer, fsEntry->dwLength);
	free(chain);
	return buffer;
}
__checkReturn errno_t CXeFlashFileSystemRoot::FileAdd(CHAR* szName, FLASHFILESYSTEM_ENTRY** pfsEntry)
{
	// TODO: CHECK IF WE REACHED MAX ENTRY COUNT
	// AND ALLOCATE ANOTHER BLOCK FOR FS ROOT?
	if(this->FindEntry(szName) != NULL)
		return 1;
	Log(-1, "CXeFlashFileSystemRoot::FileAdd: adding file %s...\n", szName);
	FLASHFILESYSTEM_ENTRY* ent = &this->pfsEntries[this->cfsEntries];
	*pfsEntry = ent;
#ifdef _DEBUG
	ent = ent;
#endif
	memset(ent->cFileName, 0, 22);
	memcpy(ent->cFileName, szName, strlen(szName) % 23);
	this->cfsEntries++;
	if(!this->ChainAllocBlock(&ent->wBlockNumber))
		return 2;
	return 0;
}
FLASHFILESYSTEM_ENTRY* CXeFlashFileSystemRoot::FindEntry(PSZ szName)
{
	for(DWORD i = 0; i < this->cfsEntries; i++)
		if(!strcmp(this->pfsEntries[i].cFileName, szName))
			return &this->pfsEntries[i];

	return NULL;
}
__checkReturn errno_t CXeFlashFileSystemRoot::Save(WORD wBlkIdx)
{
	// TODO: detect if we have too many entries and add a block to the chain if we do
	// TODO: fix up blockmap if we do have too many entries (extend it, copy data, dealloc old one etc)
	WORD chainLength = 0;
	WORD* fschain = this->ChainGetFromStart(wBlkIdx, &chainLength);
	for(int y = 0; y < chainLength; y++)
	{
		BYTE* blkData = (BYTE*)malloc(this->xepBlkDriver->dwLilBlockLength);
		int pgcount = this->xepBlkDriver->dwLilBlockLength / 512;
		DWORD * blkMapPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
		DWORD * fEntryPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
		DWORD blNum = 0;
		DWORD fsNum = 0;
		Log(0, "CXeFlashFileSystemRoot::Save: saving filesystem to block 0x%x...\n", fschain[y]);
		BYTE spare[0x10];
		this->xepBlkDriver->ReadLilBlockSpare(wBlkIdx, spare);
		this->xepBlkDriver->SetSpareSeqField(spare, this->dwVersion);
		this->xepBlkDriver->SetSpareBlockTypeField(spare, 0x30);
		this->xepBlkDriver->WriteLilBlockSpare(wBlkIdx, spare);
		//this->dwVersion = this->xepBlkDriver->GetSpareSeqField(spare);
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

		Log(0, "CXeFlashFileSystemRoot::Save: writing blockmap...\n");
		// save blockmap

		for(DWORD i = 0; i < blNum; i++)
		{
			BYTE* bmapdata = blkData + (blkMapPages[i] * 0x200);
			for(int z = 0; z < 0x100; z++) // 0x100 is max blocks per page
				*(WORD*)(bmapdata + (z * 2)) = bswap16(this->pwBlockMap[z + (i * 0x100) + (y*0x1000)]);
		}

		Log(0, "CXeFlashFileSystemRoot::Save: writing file entries...\n");
		// *deep breath* lets try to write the filenames...
		for(DWORD i = 0; i < fsNum; i++)
		{
			BYTE* fsdata = blkData + (fEntryPages[i] * 0x200);
			for(int z = 0; z < 0x10; z++) // 0x10 is max file entries per page
			{
				this->pfsEntries[(y*0x100) + (i*0x10) + z].dwLength = bswap32(this->pfsEntries[(y*0x100) + (i*0x10) + z].dwLength);
				this->pfsEntries[(y*0x100) + (i*0x10) + z].dwTimeStamp = bswap32(this->pfsEntries[(y*0x100) + (i*0x10) + z].dwTimeStamp);
				this->pfsEntries[(y*0x100) + (i*0x10) + z].wBlockNumber = bswap16(this->pfsEntries[(y*0x100) + (i*0x10) + z].wBlockNumber);
				memcpy((VOID*)(fsdata + (z * sizeof(FLASHFILESYSTEM_ENTRY))), (VOID*)&this->pfsEntries[(y*0x100) + (i*0x10) + z], sizeof(FLASHFILESYSTEM_ENTRY));
				this->pfsEntries[(y*0x100) + (i*0x10) + z].dwLength = bswap32(this->pfsEntries[(y*0x100) + (i*0x10) + z].dwLength);
				this->pfsEntries[(y*0x100) + (i*0x10) + z].dwTimeStamp = bswap32(this->pfsEntries[(y*0x100) + (i*0x10) + z].dwTimeStamp);
				this->pfsEntries[(y*0x100) + (i*0x10) + z].wBlockNumber = bswap16(this->pfsEntries[(y*0x100) + (i*0x10) + z].wBlockNumber);
			}
		}
		free(blkMapPages);
		free(fEntryPages);
		Log(0, "CXeFlashFileSystemRoot::Save: saving block...\n");
		this->xepBlkDriver->WriteLilBlock(fschain[y], blkData, this->xepBlkDriver->dwLilBlockLength);
		free(blkData);
	}
	free(fschain);
	return 0;
}
__checkReturn errno_t CXeFlashFileSystemRoot::Load(WORD wBlkIdx)
{
	// you should be able to call this to add entries from another fs root block
	// this should _ONLY_ be done if the fs blockmap says that the block is in a chain
	if(this->xepCoronaData == 0)
	{
		BYTE spare[0x10];
		this->xepBlkDriver->ReadLilBlockSpare(wBlkIdx, (BYTE*)&spare);
		this->dwVersion = this->xepBlkDriver->GetSpareSeqField(spare);
	}
	else
		this->dwVersion = this->xepCoronaData->dwFSVersion;

	BYTE* blkData = (BYTE*)malloc(this->xepBlkDriver->dwLilBlockLength);
	this->xepBlkDriver->ReadLilBlock(wBlkIdx, blkData, this->xepBlkDriver->dwLilBlockLength);

	int pgcount = this->xepBlkDriver->dwLilBlockLength / 512;
	DWORD * blkMapPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
	DWORD * fEntryPages = (DWORD*)malloc((pgcount / 2) * sizeof(DWORD));
	DWORD blNum = 0;
	DWORD fsNum = 0;
	Log(0, "CXeFlashFileSystemRoot::Load: loading filesystem from block 0x%x...\n", wBlkIdx);
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
	WORD* newmap = (WORD*)malloc(this->cbBlockMap + (blNum * 0x100) * sizeof(WORD));
	//this->pwBlockMap = 
	Log(0, "CXeFlashFileSystemRoot::Load: loading blockmap...\n");
	// read blockmap

	for(DWORD i = 0; i < blNum; i++)
	{
		BYTE* bmapdata = blkData + (blkMapPages[i] * 0x200);
		for(int y = 0; y < 0x100; y++) // 0x100 is max blocks per page
			newmap[y + ((i+this->cbBlockMap)*0x100)] = bswap16(*(WORD*)(bmapdata + (y * 2)));
	}
	if(this->pwBlockMap != 0)
	{
		memcpy((void*)newmap, this->pwBlockMap, this->cbBlockMap * sizeof(WORD));
		free(this->pwBlockMap);
	}
	this->pwBlockMap = newmap;
	this->cbBlockMap += (blNum*0x100);
	free(blkMapPages);
	// alloc our filename array
	FLASHFILESYSTEM_ENTRY* newentries = (FLASHFILESYSTEM_ENTRY*)malloc((this->cfsEntries + (fsNum * 0x10)) * sizeof(FLASHFILESYSTEM_ENTRY));

	//this->pfsEntries = (FLASHFILESYSTEM_ENTRY*)malloc((fsNum * 0x10) * sizeof(FLASHFILESYSTEM_ENTRY));
	
	Log(0, "CXeFlashFileSystemRoot::Load: loading file entries...\n");
	// *deep breath* read filenames
	for(DWORD i = 0; i < fsNum; i++)
	{
		BYTE* fsdata = blkData + (fEntryPages[i] * 0x200);
		for(int y = 0; y < 0x10; y++) // 0x20 is max file entries per page
		{
			memcpy((VOID*)&newentries[this->cfsEntries + y + (i*0x10)], (VOID*)(fsdata + (y * sizeof(FLASHFILESYSTEM_ENTRY))), sizeof(FLASHFILESYSTEM_ENTRY));
			newentries[this->cfsEntries + y + (i*0x10)].dwLength = bswap32(newentries[this->cfsEntries + y + (i*0x10)].dwLength);
			newentries[this->cfsEntries + y + (i*0x10)].dwTimeStamp = bswap32(newentries[this->cfsEntries + y + (i*0x10)].dwTimeStamp);
			newentries[this->cfsEntries + y + (i*0x10)].wBlockNumber = bswap16(newentries[this->cfsEntries + y + (i*0x10)].wBlockNumber);
		}
	}
	if(this->pfsEntries != 0)
	{
		memcpy((void*)newentries, this->pfsEntries, this->cfsEntries * sizeof(FLASHFILESYSTEM_ENTRY));
		free(this->pfsEntries);
	}
	this->pfsEntries = newentries;
	free(fEntryPages);
	Log(0, "CXeFlashFileSystemRoot::Load: validating file entries...\n");
	int numentries = this->cfsEntries;
	for(DWORD i = this->cfsEntries; i < (fsNum * 0x10) + this->cfsEntries; i++)
	{
		if(this->pfsEntries[i].wBlockNumber == 0x0 || this->pfsEntries[i].dwLength == 0x0)
			break;
		if(this->pfsEntries[i].cFileName[0] == 0x05)
			this->pfsEntries[i].cFileName[0] = '_';
		numentries++;
	}
	this->cfsEntries = numentries;
	free(blkData);
	// now check blockmap shit
	WORD bmap = this->pwBlockMap[wBlkIdx];
	if((bmap & 0x7fff) < 0x1ffb)
		return this->Load(bmap);
	return 0;
}