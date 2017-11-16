#ifndef _KERNELP_H_
#define _KERNELP_H_

enum XEX_MODULE_FLAG
{ 
	XEX_MODULE_FLAG_TITLE_PROCESS = 0x1,
	XEX_MODULE_FLAG_TITLE_IMPORTS = 0x2,
	XEX_MODULE_FLAG_DEBUGGER = 0x4,
	XEX_MODULE_FLAG_DLL = 0x8,
	XEX_MODULE_FLAG_PATCH = 0x10,
	XEX_MODULE_FLAG_PATCH_FULL = 0x20,
	XEX_MODULE_FLAG_PATCH_DELTA = 0x40,
	
	XEX_MODULE_FLAG_BOUND_PATH = 0x40000000,
	XEX_MODULE_FLAG_SILENT_LOAD = 0x80000000
};

// an intrinsic they left out...
#define __isync() __emit(0x4C00012C)

#define CONSTANT_OBJECT_STRING(s)   { strlen( s ) / sizeof( OCHAR ), (strlen( s ) / sizeof( OCHAR ))+1, s }
#define MAKE_STRING(s)   {(USHORT)(strlen(s)), (USHORT)((strlen(s))+1), (PCHAR)s}

#define EXPORTNUM(x) // Just for documentation, thx XBMC!

#define STATUS_SUCCESS 0
#define NT_EXTRACT_ST(Status)  ((((ULONG)(Status)) >> 30)& 0x3)
#define NT_SUCCESS(Status)    (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)   (NT_EXTRACT_ST(Status) == 1)
#define NT_WARNING(Status)    (NT_EXTRACT_ST(Status) == 2)
#define NT_ERROR(Status)  (NT_EXTRACT_ST(Status) == 3)

#define STATUS_SUCCESS 0
#define FILE_SYNCHRONOUS_IO_NONALERT 0x20
#define OBJ_CASE_INSENSITIVE  0x40

// for KeGetCurrentProcessType()
#define IDLE_PROC 0
#define USER_PROC 1
#define SYSTEM_PROC 2

typedef long NTSTATUS;
typedef ULONG  ACCESS_MASK;

typedef struct _STRING {
 USHORT Length;
 USHORT MaximumLength;
 PCHAR Buffer;
} STRING, ANSI_STRING, *PSTRING, *PANSI_STRING;

typedef struct _CSTRING {
 USHORT Length;
 USHORT MaximumLength;
 CONST char *Buffer;
} CSTRING, *PCSTRING;

typedef struct _UNICODE_STRING {
 USHORT Length;
 USHORT MaximumLength;
 PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef STRING  OBJECT_STRING;
typedef CSTRING  COBJECT_STRING;
typedef PSTRING  POBJECT_STRING;
typedef PCSTRING PCOBJECT_STRING;
typedef STRING  OEM_STRING;
typedef PSTRING  POEM_STRING;
typedef CHAR  OCHAR;
typedef CHAR*  POCHAR;
typedef PSTR  POSTR;
typedef PCSTR  PCOSTR;
typedef CHAR*  PSZ;
typedef CONST CHAR* PCSZ;
typedef STRING  ANSI_STRING;
typedef PSTRING  PANSI_STRING;
typedef CSTRING  CANSI_STRING;
typedef PCSTRING PCANSI_STRING;
#define ANSI_NULL ((CHAR)0)  // winnt
typedef CONST UNICODE_STRING* PCUNICODE_STRING;
#define UNICODE_NULL  ((WCHAR)0) // winnt

#define OTEXT(quote) __OTEXT(quote)


typedef struct _IO_STATUS_BLOCK {
 union {
 NTSTATUS Status;
 PVOID Pointer;
 } st;
 ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef void (NTAPI *PIO_APC_ROUTINE) (
 IN PVOID ApcContext,
 IN PIO_STATUS_BLOCK IoStatusBlock,
 IN ULONG Reserved
 );

typedef struct _OBJECT_ATTRIBUTES {
 HANDLE RootDirectory;
 POBJECT_STRING ObjectName;
 ULONG Attributes;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// void InitializeObjectAttributes(
//  OUT POBJECT_ATTRIBUTES p,
//  IN STRING n,
//  IN ULONG a,
//  IN HANDLE r)
#define InitializeObjectAttributes( p, name, attrib, root){ \
 (p)->RootDirectory = root;    \
 (p)->Attributes = attrib;  \
 (p)->ObjectName = name;    \
}

// returned by a call to 'NtQueryInformationFile' with 0x22 = FileNetworkOpenInformation
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  LARGE_INTEGER  AllocationSize;
  LARGE_INTEGER  EndOfFile;
  ULONG  FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;


typedef struct _XBOX_HARDWARE_INFO {
 DWORD Flags;
 unsigned char NumberOfProcessors;
 unsigned char PCIBridgeRevisionID;
 unsigned char Reserved[6];
 unsigned short BldrMagic;
 unsigned short BldrFlags;
} XBOX_HARDWARE_INFO, *PXBOX_HARDWARE_INFO;
/* description about xex exe headers in memory */
typedef struct _XEX_IMPORT_TABLE_ENT {
DWORD ImportDestAddr;
DWORD ImportStubAddr;
} XEX_IMPORT_TABLE_ENT, *PXEX_IMPORT_TABLE_ENT;

typedef struct _XEX_IMPORT_TABLE {
DWORD TableSize;
 BYTE NextImportDigest[20];
DWORD ModuleNumber;
DWORD Version[2];
 BYTE Unused;
 BYTE ModuleIndex;
 WORD ImportCount;
DWORD ImportStubAddr[1];
} XEX_IMPORT_TABLE, *PXEX_IMPORT_TABLE;

typedef struct _XEX_IMPORT_DESCRIPTOR {
DWORD Size;
DWORD NameTableSize;
DWORD ModuleCount;
 // nametable is here of nametable size
 // followed by modulecount number of xex import tables
} XEX_IMPORT_DESCRIPTOR, *PXEX_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_EXPORT_ADDRESS_TABLE {
DWORD Magic[3]; // 48 00 00 00 00 48 56 45 48 00 00 00
DWORD ModuleNumber[2];
DWORD Version[3];
DWORD ImageBaseAddress; // must be <<16 to be accurate
DWORD Count;
DWORD Base;
DWORD ordOffset[1]; // ordOffset[0]+ (ImageBaseAddress<<8) = function offset of ordinal 1
} IMAGE_EXPORT_ADDRESS_TABLE, *PIMAGE_EXPORT_ADDRESS_TABLE;

typedef struct _XEX_SECURITY_INFO {
 unsigned long Size;
DWORD ImageSize;
 BYTE Signature[256];
DWORD InfoSize;
DWORD ImageFlags;
DWORD LoadAddress;
 BYTE ImageHash[20];
DWORD ImportTableCount;
 BYTE ImportDigest[20];
 BYTE MediaID[16];
 BYTE ImageKey[16];
 PIMAGE_EXPORT_ADDRESS_TABLE ExportTableAddress;
 BYTE HeaderHash[20];
DWORD GameRegion;
DWORD AllowedMediaTypes;
DWORD PageDescriptorCount;
} XEX_SECURITY_INFO, *PXEX_SECURITY_INFO;

// Test
typedef struct _IMAGE_XEX_HEADER2 {
DWORD Magic;
DWORD ModuleFlags;
DWORD SizeOfHeaders;
DWORD SizeOfDiscardableHeaders;
DWORD SecurityInfoOffset;
DWORD HeaderDirectoryEntryCount;
} IMAGE_XEX_HEADER2, *PIMAGE_XEX_HEADER2;
// End Test

typedef struct _IMAGE_XEX_HEADER {
DWORD Magic;
DWORD ModuleFlags;
DWORD SizeOfHeaders;
DWORD SizeOfDiscardableHeaders;
 PXEX_SECURITY_INFO SecurityInfo;
DWORD HeaderDirectoryEntryCount;
} IMAGE_XEX_HEADER, *PIMAGE_XEX_HEADER;

typedef struct _LDR_DATA_TABLE_ENTRY {
 LIST_ENTRY InLoadOrderLinks;
 LIST_ENTRY InClosureOrderLinks;
 LIST_ENTRY InInitializationOrderLinks;
PVOID NtHeadersBase;
PVOID ImageBase;
DWORD SizeOfNtImage;
 UNICODE_STRING FullDllName;
 UNICODE_STRING BaseDllName;
DWORD Flags;
DWORD SizeOfFullImage;
PVOID EntryPoint;
 WORD LoadCount;
 WORD ModuleIndex;
PVOID DllBaseOriginal;
DWORD CheckSum;
DWORD ModuleLoadFlags;
DWORD TimeDateStamp;
PVOID LoadedImports;
PVOID XexHeaderBase;
 union{
 STRING LoadFileName;
 struct {
 PVOID ClosureRoot; // LDR_DATA_TABLE_ENTRY
 PVOID TraversalParent; // LDR_DATA_TABLE_ENTRY
 } asEntry;
 } inf;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _XBOX_KRNL_VERSION{
 USHORT Major; // for 360 this is always 2
 USHORT Minor; // usually 0
 USHORT Build; // current version, for example 9199
 USHORT Qfe;
} XBOX_KRNL_VERSION, *PXBOX_KRNL_VERSION;

typedef enum _POOL_TYPE {
 NonPagedPool   = 0,
 PagedPool   = 1,
 NonPagedPoolMustSucceed  = 2,
 DontUseThisType   = 3,
 NonPagedPoolCacheAligned = 4,
 PagedPoolCacheAligned = 5,
 NonPagedPoolCacheAlignedMustS   = 6
} POOL_TYPE;

typedef struct _DISPATCHER_HEADER
{
     union
     {
          struct
          {
               UCHAR Type;
               union
               {
                    UCHAR Abandoned;
                    UCHAR Absolute;
                    UCHAR NpxIrql;
                    UCHAR Signalling;
               };
               union
               {
                    UCHAR Size;
                    UCHAR Hand;
               };
               union
               {
                    UCHAR Inserted;
                    UCHAR DebugActive;
                    UCHAR DpcActive;
               };
          };
          LONG Lock;
     };
     LONG SignalState;
     LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

typedef struct _KEVENT
{
     DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT;

typedef struct _KSEMAPHORE
{
     DISPATCHER_HEADER Header;
     LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE;

//
// Reader/writer lock.
//

typedef struct _ERWLOCK {
    LONG LockCount;
    ULONG WritersWaitingCount;
    ULONG ReadersWaitingCount;
    ULONG ReadersEntryCount;
    KEVENT WriterEvent;
    KSEMAPHORE ReaderSemaphore;
} ERWLOCK, *PERWLOCK;

typedef struct _OWNER_ENTRY
{
     ULONG OwnerThread;
     union
     {
          LONG OwnerCount;
          ULONG TableSize;
     };
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE
{
     LIST_ENTRY SystemResourcesList;
     POWNER_ENTRY OwnerTable;
     SHORT ActiveCount;
     WORD Flag;
     PKSEMAPHORE SharedWaiters;
     PKEVENT ExclusiveWaiters;
     OWNER_ENTRY OwnerEntry;
     ULONG ActiveEntries;
     ULONG ContentionCount;
     ULONG NumberOfSharedWaiters;
     ULONG NumberOfExclusiveWaiters;
     union
     {
          PVOID Address;
          ULONG CreatorBackTraceIndex;
     };
     ULONG SpinLock;
} ERESOURCE, *PERESOURCE;

typedef struct _OBJECT_TYPE_INITIALIZER
{
     WORD Length;
     UCHAR ObjectTypeFlags;
     ULONG CaseInsensitive: 1;
     ULONG UnnamedObjectsOnly: 1;
     ULONG UseDefaultObject: 1;
     ULONG SecurityRequired: 1;
     ULONG MaintainHandleCount: 1;
     ULONG MaintainTypeList: 1;
     ULONG ObjectTypeCode;
     ULONG InvalidAttributes;
     GENERIC_MAPPING GenericMapping;
     ULONG ValidAccessMask;
     POOL_TYPE PoolType;
     ULONG DefaultPagedPoolCharge;
     ULONG DefaultNonPagedPoolCharge;
     PVOID DumpProcedure;
     LONG * OpenProcedure;
     PVOID CloseProcedure;
     PVOID DeleteProcedure;
     LONG * ParseProcedure;
     LONG * SecurityProcedure;
     LONG * QueryNameProcedure;
     UCHAR * OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;
typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;
typedef struct _EX_PUSH_LOCK
{
     union
     {
          ULONG Locked: 1;
          ULONG Waiting: 1;
          ULONG Waking: 1;
          ULONG MultipleShared: 1;
          ULONG Shared: 28;
          ULONG Value;
          PVOID Ptr;
     };
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef struct _OBJECT_TYPE
{
     ERESOURCE Mutex;
     LIST_ENTRY TypeList;
     UNICODE_STRING Name;
     PVOID DefaultObject;
     ULONG Index;
     ULONG TotalNumberOfObjects;
     ULONG TotalNumberOfHandles;
     ULONG HighWaterNumberOfObjects;
     ULONG HighWaterNumberOfHandles;
     OBJECT_TYPE_INITIALIZER TypeInfo;
     ULONG Key;
     EX_PUSH_LOCK ObjectLocks[32];
} OBJECT_TYPE, *POBJECT_TYPE;

typedef struct _EX_TITLE_TERMINATE_REGISTRATION {
	void*	   NotificationRoutine;
	DWORD	   Priority;
	LIST_ENTRY ListEntry;
} EX_TITLE_TERMINATE_REGISTRATION, *PEX_TITLE_TERMINATE_REGISTRATION;

typedef enum _FILE_INFORMATION_CLASS { 
  FileDirectoryInformation                  = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileIoCompletionNotificationInformation,
  FileIoStatusBlockRangeInformation,
  FileIoPriorityHintInformation,
  FileSfioReserveInformation,
  FileSfioVolumeInformation,
  FileHardLinkInformation,
  FileProcessIdsUsingFileInformation,
  FileNormalizedNameInformation,
  FileNetworkPhysicalNameInformation,
  FileIdGlobalTxDirectoryInformation,
  FileIsRemoteDeviceInformation,
  FileAttributeCacheInformation,
  FileNumaNodeInformation,
  FileStandardLinkInformation,
  FileRemoteProtocolInformation,
  FileMaximumInformation 
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef enum _FS_INFORMATION_CLASS {
    FileFsVolumeInformation=1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _KINTERRUPT_MODE {
  LevelSensitive,
  Latched
} KINTERRUPT_MODE;

typedef enum _KWAIT_REASON
{
         Executive = 0,
         FreePage = 1,
         PageIn = 2,
         PoolAllocation = 3,
         DelayExecution = 4,
         Suspended = 5,
         UserRequest = 6,
         WrExecutive = 7,
         WrFreePage = 8,
         WrPageIn = 9,
         WrPoolAllocation = 10,
         WrDelayExecution = 11,
         WrSuspended = 12,
         WrUserRequest = 13,
         WrEventPair = 14,
         WrQueue = 15,
         WrLpcReceive = 16,
         WrLpcReply = 17,
         WrVirtualMemory = 18,
         WrPageOut = 19,
         WrRendezvous = 20,
         Spare2 = 21,
         Spare3 = 22,
         Spare4 = 23,
         Spare5 = 24,
         WrCalloutStack = 25,
         WrKernel = 26,
         WrResource = 27,
         WrPushLock = 28,
         WrMutex = 29,
         WrQuantumEnd = 30,
         WrDispatchInt = 31,
         WrPreempted = 32,
         WrYieldExecution = 33,
         WrFastMutex = 34,
         WrGuardedMutex = 35,
         WrRundown = 36,
         MaximumWaitReason = 37
} KWAIT_REASON;

typedef enum _WAIT_TYPE
{
         WaitAll = 0,
         WaitAny = 1
} WAIT_TYPE;
typedef DWORD (*EXPTRYTOBOOTMEDIAKERNEL)(PANSI_STRING KernelPath, BOOL IsRemoteFile, BOOL MapRootToCDRom);

//fix up KeBugCheck/KeBugCheckEx
extern "C"
{
// 0x0001 - DbgBreakPoint
NTSYSAPI EXPORTNUM(1) void NTAPI DbgBreakPoint( void );  
// 0x0002 - DbgBreakPointWithStatus
NTSYSAPI EXPORTNUM(2) void NTAPI DbgBreakPointWithStatus( IN ULONG Status );  
// 0x0003 - DbgPrint
//NTSYSAPI EXPORTNUM(3) void NTAPI DbgPrint(const char* s, ...); 
// 0x0004 - DbgPrompt
NTSYSAPI EXPORTNUM(4) ULONG NTAPI DbgPrompt( PCH Prompt, PCH Response, ULONG MaximumResponseLength );  
// 0x0007 - ExAcquireReadWriteLockExclusive
NTSYSAPI EXPORTNUM(7) void NTAPI ExAcquireReadWriteLockExclusive( IN PERWLOCK ReadWriteLock );  
// 0x0008 - ExAcquireReadWriteLockShared
NTSYSAPI EXPORTNUM(8) void NTAPI ExAcquireReadWriteLockShared( IN PERWLOCK ReadWriteLock );  
// 0x0009 - ExAllocatePool
NTSYSAPI EXPORTNUM(9) PVOID NTAPI ExAllocatePool(IN DWORD NumberOfBytes); 
// 0x000A - ExAllocatePoolWithTag
NTSYSAPI EXPORTNUM(10) PVOID NTAPI ExAllocatePoolWithTag(IN DWORD NumberOfBytes, IN DWORD Tag); 
// 0x000B - ExAllocatePoolTypeWithTag
NTSYSAPI EXPORTNUM(11) PVOID NTAPI ExAllocatePoolTypeWithTag(IN  DWORD NumberOfBytes, IN  DWORD Tag, IN  POOL_TYPE PoolType); 
// 0x000C - ExConsoleGameRegion;
NTSYSAPI EXPORTNUM(12) extern PDWORD ExConsoleGameRegion; 
// 0x000D - ExCreateThread
NTSYSAPI EXPORTNUM(13) DWORD NTAPI ExCreateThread(IN PHANDLE pHandle, IN DWORD dwStackSize, IN LPDWORD lpThreadId, IN PVOID apiThreadStartup, IN LPTHREAD_START_ROUTINE lpStartAddress, IN LPVOID lpParameter, IN DWORD dwCreationFlagsMod); 
// 0x000E - ExEventObjectType;
NTSYSAPI EXPORTNUM(14) extern POBJECT_TYPE ExEventObjectType; 
// 0x000F - ExFreePool
NTSYSAPI EXPORTNUM(15) void NTAPI ExFreePool(IN PVOID pPool); 
// 0x0010 - ExGetXConfigSetting
NTSYSAPI EXPORTNUM(16) NTSTATUS NTAPI ExGetXConfigSetting(WORD wCategory, WORD wSetting, PVOID pOutputBuff, DWORD dwOutputBuffSize, PWORD pSettingSize); 
// 0x0011 - ExInitializeReadWriteLock
NTSYSAPI EXPORTNUM(17) void NTAPI ExInitializeReadWriteLock( IN PERWLOCK ReadWriteLock );  
// 0x0012 - ExMutantObjectType;
NTSYSAPI EXPORTNUM(18) extern POBJECT_TYPE ExMutantObjectType; 
// 0x0013 - ExQueryPoolBlockSize
NTSYSAPI EXPORTNUM(19) ULONG NTAPI ExQueryPoolBlockSize( IN PVOID PoolBlock );  
// 0x0015 - ExRegisterTitleTerminateNotification
NTSYSAPI EXPORTNUM(21) void NTAPI ExRegisterTitleTerminateNotification( PEX_TITLE_TERMINATE_REGISTRATION, BOOL);  
// 0x0016 - ExReleaseReadWriteLock
NTSYSAPI EXPORTNUM(22) void NTAPI ExReleaseReadWriteLock( IN PERWLOCK ReadWriteLock );  
// 0x0017 - ExSemaphoreObjectType;
NTSYSAPI EXPORTNUM(23) extern POBJECT_TYPE ExSemaphoreObjectType; 
// 0x0018 - ExSetXConfigSetting
NTSYSAPI EXPORTNUM(24) NTSTATUS NTAPI ExSetXConfigSetting(WORD wCategory, WORD wSetting, PVOID pInputBuff, WORD wSettingSize); 
// 0x001B - ExThreadObjectType;
NTSYSAPI EXPORTNUM(27) extern POBJECT_TYPE ExThreadObjectType; 
// 0x001C - ExTimerObjectType;
NTSYSAPI EXPORTNUM(28) extern POBJECT_TYPE ExTimerObjectType; 
// 0x0025 - HalReadWritePCISpace
NTSYSAPI EXPORTNUM(37) void NTAPI HalReadWritePCISpace( IN ULONG BusNumber, IN ULONG SlotNumber, IN ULONG RegisterNumber, IN PVOID Buffer, IN ULONG Length, IN BOOLEAN WritePCISpace );  
// 0x0028 - HalReturnToFirmware
NTSYSAPI EXPORTNUM(40) void NTAPI HalReturnToFirmware(IN DWORD dwPowerDownMode); 
// 0x002F - IoAllocateIrp
NTSYSAPI EXPORTNUM(47) PVOID NTAPI IoAllocateIrp( IN CCHAR StackSize );  
// 0x0030 - IoBuildAsynchronousFsdRequest
NTSYSAPI EXPORTNUM(48) PVOID NTAPI IoBuildAsynchronousFsdRequest( IN ULONG MajorFunction, IN PVOID DeviceObject, IN OUT PVOID Buffer OPTIONAL, IN ULONG Length OPTIONAL, IN PLARGE_INTEGER StartingOffset OPTIONAL, IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL );  
// 0x0031 - IoBuildDeviceIoControlRequest
NTSYSAPI EXPORTNUM(49) PVOID NTAPI IoBuildDeviceIoControlRequest( IN ULONG IoControlCode, IN PVOID DeviceObject, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, IN BOOLEAN InternalDeviceIoControl, IN PKEVENT Event, OUT PIO_STATUS_BLOCK IoStatusBlock );  
// 0x0032 - IoBuildSynchronousFsdRequest
NTSYSAPI EXPORTNUM(50) PVOID NTAPI IoBuildSynchronousFsdRequest( IN ULONG MajorFunction, IN PVOID DeviceObject, IN OUT PVOID Buffer OPTIONAL, IN ULONG Length OPTIONAL, IN PLARGE_INTEGER StartingOffset OPTIONAL, IN PKEVENT Event, OUT PIO_STATUS_BLOCK IoStatusBlock );  
// 0x0033 - IoCallDriver
NTSYSAPI EXPORTNUM(51) NTSTATUS NTAPI IoCallDriver( IN DWORD DeviceObject, IN OUT PVOID Irp );  
// 0x0034 - IoCheckShareAccess
NTSYSAPI EXPORTNUM(52) NTSTATUS NTAPI IoCheckShareAccess( IN ACCESS_MASK DesiredAccess, IN ULONG DesiredShareAccess, IN OUT PVOID FileObject, IN OUT PVOID ShareAccess, IN BOOLEAN Update );  
// 0x0035 - IoCompleteRequest
NTSYSAPI EXPORTNUM(53) void NTAPI IoCompleteRequest( IN PVOID Irp, IN CCHAR PriorityBoost );  
// 0x0036 - IoCompletionObjectType;
NTSYSAPI EXPORTNUM(54) extern POBJECT_TYPE IoCompletionObjectType; 
// 0x0037 - IoCreateDevice
NTSYSAPI EXPORTNUM(55) NTSTATUS NTAPI IoCreateDevice( IN PVOID DriverObject, IN ULONG DeviceExtensionSize, IN POBJECT_STRING DeviceName OPTIONAL, IN ULONG DeviceType, IN BOOLEAN Exclusive, OUT PVOID *DeviceObject );  
// 0x0038 - IoCreateFile
NTSYSAPI EXPORTNUM(56) NTSTATUS NTAPI IoCreateFile( OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes, IN ULONG ShareAccess, IN ULONG Disposition, IN ULONG CreateOptions, IN ULONG Options );  
// 0x0039 - IoDeleteDevice
NTSYSAPI EXPORTNUM(57) void NTAPI IoDeleteDevice( IN PVOID DeviceObject );  
// 0x003A - IoDeviceObjectType;
NTSYSAPI EXPORTNUM(58) extern POBJECT_TYPE IoDeviceObjectType; 
// 0x003B - IoDismountVolume
NTSYSAPI EXPORTNUM(59) NTSTATUS NTAPI IoDismountVolume( IN PVOID DeviceObject );  
// 0x003C - IoDismountVolumeByFileHandle
//NTSYSAPI EXPORTNUM(60) NTSTATUS NTAPI IoDismountVolume( IN PVOID DeviceObject );  
// 0x003D - IoDismountVolumeByName
NTSYSAPI EXPORTNUM(61) NTSTATUS NTAPI IoDismountVolumeByName( IN POBJECT_STRING DeviceName );  
// 0x003E - IoFileObjectType;
NTSYSAPI EXPORTNUM(62) extern POBJECT_TYPE IoFileObjectType; 
// 0x003F - IoFreeIrp
NTSYSAPI EXPORTNUM(63) void NTAPI IoFreeIrp( IN PVOID Irp );  
// 0x0040 - IoInitializeIrp
NTSYSAPI EXPORTNUM(64) void NTAPI IoInitializeIrp( IN OUT PVOID Irp, IN USHORT PacketSize, IN CCHAR StackSize );  
// 0x0041 - IoInvalidDeviceRequest
NTSYSAPI EXPORTNUM(65) NTSTATUS NTAPI IoInvalidDeviceRequest( IN PVOID DeviceObject, IN PVOID Irp );  
// 0x0043 - IoQueueThreadIrp
NTSYSAPI EXPORTNUM(67) void NTAPI IoQueueThreadIrp( IN PVOID Irp );  
// 0x0045 - IoRemoveShareAccess
NTSYSAPI EXPORTNUM(69) void NTAPI IoRemoveShareAccess( IN PVOID FileObject, IN OUT PVOID ShareAccess );  
// 0x0046 - IoSetIoCompletion
NTSYSAPI EXPORTNUM(70) NTSTATUS NTAPI IoSetIoCompletion( IN PVOID IoCompletion, IN PVOID KeyContext, IN PVOID ApcContext, IN NTSTATUS IoStatus, IN ULONG_PTR IoStatusInformation );  
// 0x0047 - IoSetShareAccess
NTSYSAPI EXPORTNUM(71) void NTAPI IoSetShareAccess( IN ACCESS_MASK DesiredAccess, IN ULONG DesiredShareAccess, IN OUT PVOID FileObject, OUT PVOID ShareAccess );  
// 0x0048 - IoStartNextPacket
NTSYSAPI EXPORTNUM(72) void NTAPI IoStartNextPacket( IN PVOID DeviceObject );  
// 0x0049 - IoStartNextPacketByKey
NTSYSAPI EXPORTNUM(73) void NTAPI IoStartNextPacketByKey( IN PVOID DeviceObject, IN ULONG Key );  
// 0x004A - IoStartPacket
NTSYSAPI EXPORTNUM(74) void NTAPI IoStartPacket( IN PVOID DeviceObject, IN PVOID Irp, IN PULONG Key OPTIONAL );  
// 0x004B - IoSynchronousDeviceIoControlRequest
NTSYSAPI EXPORTNUM(75) NTSTATUS NTAPI IoSynchronousDeviceIoControlRequest( IN ULONG IoControlCode, IN PVOID DeviceObject, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength, OUT PULONG ReturnedOutputBufferLength OPTIONAL, IN BOOLEAN InternalDeviceIoControl );  
// 0x004C - IoSynchronousFsdRequest
NTSYSAPI EXPORTNUM(76) NTSTATUS NTAPI IoSynchronousFsdRequest( IN ULONG MajorFunction, IN PVOID DeviceObject, IN OUT PVOID Buffer OPTIONAL, IN ULONG Length OPTIONAL, IN PLARGE_INTEGER StartingOffset OPTIONAL );  
// 0x004E - KeAlertResumeThread
NTSYSAPI EXPORTNUM(78) ULONG NTAPI KeAlertResumeThread( IN PVOID Thread );  
// 0x004F - KeAlertThread
NTSYSAPI EXPORTNUM(79) BOOLEAN NTAPI KeAlertThread( IN PVOID Thread, IN CCHAR ProcessorMode );  
// 0x0051 - KeBoostPriorityThread
NTSYSAPI EXPORTNUM(81) void NTAPI KeBoostPriorityThread( IN PVOID Thread, IN LONG Increment );  
// 0x0052 - KeBugCheck
NTSYSAPI EXPORTNUM(82) DECLSPEC_NORETURN void NTAPI KeBugCheck( IN ULONG BugCheckCode );
// 0x0053 - KeBugCheckEx
NTSYSAPI EXPORTNUM(83) DECLSPEC_NORETURN void NTAPI KeBugCheckEx( IN ULONG BugCheckCode, IN ULONG_PTR BugCheckParameter1, IN ULONG_PTR BugCheckParameter2, IN ULONG_PTR BugCheckParameter3, IN ULONG_PTR BugCheckParameter4 );  
// 0x0054 - KeCancelTimer
NTSYSAPI EXPORTNUM(84) BOOLEAN NTAPI KeCancelTimer( IN PVOID );  
// 0x0055 - KeConnectInterrupt
NTSYSAPI EXPORTNUM(85) BOOLEAN NTAPI KeConnectInterrupt( IN PVOID Interrupt );  
// 0x0056 - KeContextFromKframes
NTSYSAPI EXPORTNUM(86) void NTAPI KeContextFromKframes( IN PVOID TrapFrame, IN OUT PCONTEXT ContextFrame );  
// 0x0057 - KeContextToKframes
NTSYSAPI EXPORTNUM(87) void NTAPI KeContextToKframes( IN OUT PVOID TrapFrame, IN PCONTEXT ContextFrame, IN ULONG ContextFlags );  
// 0x005A - KeDelayExecutionThread
NTSYSAPI EXPORTNUM(90) NTSTATUS NTAPI KeDelayExecutionThread( IN CCHAR WaitMode, IN BOOLEAN Alertable, IN PLARGE_INTEGER Interval );  
// 0x005C - KeDisconnectInterrupt
NTSYSAPI EXPORTNUM(92) BOOLEAN NTAPI KeDisconnectInterrupt( IN PVOID Interrupt );  
// 0x005F - KeEnterCriticalRegion
NTSYSAPI EXPORTNUM(95) void NTAPI KeEnterCriticalRegion( void );  
// 0x0066 - KeGetCurrentProcessType
NTSYSAPI EXPORTNUM(102) UCHAR NTAPI KeGetCurrentProcessType(void); 
// 0x006D - KeInitializeApc
NTSYSAPI EXPORTNUM(109) void NTAPI KeInitializeApc(IN PVOID Apc, IN PVOID Thread, IN PVOID KernelRoutine, IN PVOID RundownRoutine OPTIONAL, IN PVOID NormalRoutine OPTIONAL, IN CCHAR ProcessorMode OPTIONAL, IN PVOID NormalContext OPTIONAL); 
// 0x006E - KeInitializeDeviceQueue
NTSYSAPI EXPORTNUM(110) void NTAPI KeInitializeDeviceQueue(IN PVOID DeviceQueue); 
// 0x006F - KeInitializeDpc
NTSYSAPI EXPORTNUM(111) void NTAPI KeInitializeDpc(IN PVOID Dpc, IN PVOID DeferredRoutine, IN PVOID DeferredContext); 
// 0x0070 - KeInitializeEvent
NTSYSAPI EXPORTNUM(112) void NTAPI KeInitializeEvent( IN PVOID Event, IN EVENT_TYPE Type, IN BOOLEAN State );  
// 0x0071 - KeInitializeInterrupt
NTSYSAPI EXPORTNUM(113) void NTAPI KeInitializeInterrupt( IN PVOID Interrupt, IN PVOID ServiceRoutine, IN PVOID ServiceContext, IN ULONG Vector, IN UCHAR Irql, IN KINTERRUPT_MODE InterruptMode, IN BOOLEAN ShareVector );  
// 0x0072 - KeInitializeMutant
NTSYSAPI EXPORTNUM(114) void NTAPI KeInitializeMutant( IN PVOID Mutant, IN BOOLEAN InitialOwner );  
// 0x0073 - KeInitializeQueue
NTSYSAPI EXPORTNUM(115) void NTAPI KeInitializeQueue(IN PVOID Queue, IN ULONG qwCount OPTIONAL); 
// 0x0074 - KeInitializeSemaphore
NTSYSAPI EXPORTNUM(116) void NTAPI KeInitializeSemaphore( IN PVOID Semaphore, IN ULONG Count, IN ULONG Limit );  
// 0x0075 - KeInitializeTimerEx
//NTSYSAPI EXPORTNUM(117) void NTAPI KeInitializeTimerEx( IN PVOID Timer, IN clock_t Type );  
// 0x0076 - KeInsertByKeyDeviceQueue
NTSYSAPI EXPORTNUM(118) BOOLEAN NTAPI KeInsertByKeyDeviceQueue(IN PVOID DeviceQueue, IN PVOID DeviceQueueEntry, IN ULONG SortKey); 
// 0x0077 - KeInsertDeviceQueue
NTSYSAPI EXPORTNUM(119) BOOLEAN NTAPI KeInsertDeviceQueue(IN PVOID DeviceQueue, IN PVOID DeviceQueueEntry); 
// 0x0078 - KeInsertHeadQueue
NTSYSAPI EXPORTNUM(120) ULONG NTAPI KeInsertHeadQueue( IN PVOID Queue, IN PLIST_ENTRY Entry );  
// 0x0079 - KeInsertQueue
NTSYSAPI EXPORTNUM(121) ULONG NTAPI KeInsertQueue( IN PVOID Queue, IN PLIST_ENTRY Entry );  
// 0x007A - KeInsertQueueApc
NTSYSAPI EXPORTNUM(122) BOOLEAN NTAPI KeInsertQueueApc(IN PVOID Apc, IN PVOID SystemArgument1, IN PVOID SystemArgument2, IN LONG Increment); 
// 0x007B - KeInsertQueueDpc
NTSYSAPI EXPORTNUM(123) BOOLEAN NTAPI KeInsertQueueDpc(IN PVOID Dpc, IN PVOID SystemArgument1, IN PVOID SystemArgument2); 
// 0x007D - KeLeaveCriticalRegion
NTSYSAPI EXPORTNUM(125) void NTAPI KeLeaveCriticalRegion();  
// 0x007F - KePulseEvent
NTSYSAPI EXPORTNUM(127) ULONG NTAPI KePulseEvent( IN PVOID Event, IN LONG Increment, IN BOOLEAN Wait );  
// 0x0081 - KeQueryBasePriorityThread
NTSYSAPI EXPORTNUM(129) ULONG NTAPI KeQueryBasePriorityThread( IN PVOID Thread );  
// 0x0082 - KeQueryInterruptTime
NTSYSAPI EXPORTNUM(130) ULONGLONG NTAPI KeQueryInterruptTime();  
// 0x0084 - KeQuerySystemTime
NTSYSAPI EXPORTNUM(132) void NTAPI KeQuerySystemTime( OUT PLARGE_INTEGER CurrentTime );  
// 0x0085 - KeRaiseIrqlToDpcLevel
NTSYSAPI EXPORTNUM(133) VOID NTAPI KeRaiseIrqlToDpcLevel();  
// 0x0087 - KeReleaseMutant
NTSYSAPI EXPORTNUM(135) ULONG NTAPI KeReleaseMutant( IN PVOID Mutant, IN LONG Increment, IN BOOLEAN Abandoned, IN BOOLEAN Wait );  
// 0x0088 - KeReleaseSemaphore
NTSYSAPI EXPORTNUM(136) ULONG NTAPI KeReleaseSemaphore( IN PVOID Semaphore, IN LONG Increment, IN ULONG Adjustment, IN BOOLEAN Wait );  
// 0x0089 - KeReleaseSpinLockFromRaisedIrql
NTSYSAPI EXPORTNUM(137) void NTAPI KeReleaseSpinLockFromRaisedIrql( IN OUT	PDWORD spinVar );  
// 0x008A - KeRemoveByKeyDeviceQueue
NTSYSAPI EXPORTNUM(138) PVOID NTAPI KeRemoveByKeyDeviceQueue(IN PVOID DeviceQueue, IN ULONG SortKey); 
// 0x008B - KeRemoveDeviceQueue
NTSYSAPI EXPORTNUM(139) PVOID NTAPI KeRemoveDeviceQueue(IN PVOID DeviceQueue); 
// 0x008C - KeRemoveEntryDeviceQueue
NTSYSAPI EXPORTNUM(140) BOOLEAN NTAPI KeRemoveEntryDeviceQueue(IN PVOID DeviceQueue, IN PVOID DeviceQueueEntry); 
// 0x008D - KeRemoveQueue
NTSYSAPI EXPORTNUM(141) PLIST_ENTRY NTAPI KeRemoveQueue( IN PVOID Queue, IN CCHAR WaitMode, IN PLARGE_INTEGER Timeout OPTIONAL );  
// 0x008E - KeRemoveQueueDpc
NTSYSAPI EXPORTNUM(142) BOOLEAN NTAPI KeRemoveQueueDpc(IN PVOID Dpc); 
// 0x008F - KeResetEvent
NTSYSAPI EXPORTNUM(143) ULONG NTAPI KeResetEvent( IN PVOID Event );  
// 0x0092 - KeResumeThread
NTSYSAPI EXPORTNUM(146) ULONG NTAPI KeResumeThread( IN PVOID Thread );  
// 0x0094 - KeRundownQueue
NTSYSAPI EXPORTNUM(148) PLIST_ENTRY NTAPI KeRundownQueue( IN PVOID Queue );  
// 0x0099 - KeSetBasePriorityThread
NTSYSAPI EXPORTNUM(153) ULONG NTAPI KeSetBasePriorityThread( IN PVOID Thread, IN ULONG Increment );  
// 0x009A - KeSetCurrentProcessType
NTSYSAPI EXPORTNUM(154) void NTAPI KeSetCurrentProcessType(DWORD dwProcessType); 
// 0x009C - KeSetDisableBoostThread
//NTSYSAPI EXPORTNUM(156) LOGICAL NTAPI KeSetDisableBoostThread( IN PVOID Thread, IN LOGICAL Disable );  
// 0x009D - KeSetEvent
NTSYSAPI EXPORTNUM(157) ULONG NTAPI KeSetEvent( IN PVOID Event, IN LONG Increment, IN BOOLEAN Wait );  
// 0x009E - KeSetEventBoostPriority
NTSYSAPI EXPORTNUM(158) void NTAPI KeSetEventBoostPriority( IN PVOID Event, IN PVOID *Thread OPTIONAL );  
// 0x00A3 - KeSetPriorityThread
NTSYSAPI EXPORTNUM(163) LONG NTAPI KeSetPriorityThread( IN PVOID Thread, IN LONG Priority );  
// 0x00A6 - KeSetTimer
NTSYSAPI EXPORTNUM(166) BOOLEAN NTAPI KeSetTimer( IN PVOID Timer, IN LARGE_INTEGER DueTime, IN PVOID Dpc OPTIONAL );  
// 0x00A7 - KeSetTimerEx
NTSYSAPI EXPORTNUM(167) BOOLEAN NTAPI KeSetTimerEx( IN PVOID Timer, IN LARGE_INTEGER DueTime, IN ULONG Period OPTIONAL, IN PVOID Dpc OPTIONAL );  
// 0x00A8 - KeStallExecutionProcessor
NTSYSAPI EXPORTNUM(168) BYTE NTAPI KeStallExecutionProcessor( IN		DWORD period );  
// 0x00A9 - KeSuspendThread
NTSYSAPI EXPORTNUM(169) ULONG NTAPI KeSuspendThread( IN PVOID Thread );  
// 0x00AC - KeTestAlertThread
NTSYSAPI EXPORTNUM(172) BOOLEAN NTAPI KeTestAlertThread( IN CCHAR ProcessorMode );  
// 0x00AF - KeWaitForMultipleObjects
NTSYSAPI EXPORTNUM(175) NTSTATUS NTAPI KeWaitForMultipleObjects( IN ULONG Count, IN PVOID Object[], IN WAIT_TYPE WaitType, IN KWAIT_REASON WaitReason, IN CCHAR WaitMode, IN BOOLEAN Alertable, IN PLARGE_INTEGER Timeout OPTIONAL, IN PVOID WaitBlockArray );  
// 0x00B0 - KeWaitForSingleObject
NTSYSAPI EXPORTNUM(176) NTSTATUS NTAPI KeWaitForSingleObject( IN PVOID Object, IN KWAIT_REASON WaitReason, IN CCHAR WaitMode, IN BOOLEAN Alertable, IN PLARGE_INTEGER Timeout OPTIONAL );  
// 0x00B1 - KfAcquireSpinLock
NTSYSAPI EXPORTNUM(177) BYTE NTAPI KfAcquireSpinLock( IN OUT	PDWORD spinVar );  
// 0x00B2 - KfRaiseIrql
NTSYSAPI EXPORTNUM(178) BYTE NTAPI KfRaiseIrql( IN		BYTE irql );  
// 0x00B3 - KfLowerIrql
NTSYSAPI EXPORTNUM(179) void NTAPI KfLowerIrql( IN		BYTE irql );  
// 0x00B4 - KfReleaseSpinLock
NTSYSAPI EXPORTNUM(180) void NTAPI KfReleaseSpinLock( IN OUT	PDWORD spinVar, IN		BYTE oldIrql );  
// 0x00B5 - KiBugCheckData;
NTSYSAPI EXPORTNUM(181) extern PULONG KiBugCheckData; 
// 0x00B6 - LDICreateDecompression
NTSYSAPI EXPORTNUM(182) DWORD NTAPI LDICreateDecompression(PDWORD pcbDataBlockMax, PVOID pvConfiguration, DWORD pfnma, DWORD pfnmf, PVOID pcbSrcBufferMin, PDWORD unknow, PDWORD pcbDecompressed); 
// 0x00B7 - LDIDecompress
NTSYSAPI EXPORTNUM(183) DWORD NTAPI LDIDecompress(DWORD dwContext, PVOID pbSrc, WORD cbSrc, PVOID pdDst, PDWORD pcbDecompressed); 
// 0x00B8 - LDIDestroyDecompression
NTSYSAPI EXPORTNUM(184) DWORD NTAPI LDIDestroyDecompression(DWORD dwContext); 
// 0x00BE - MmGetPhysicalAddress
NTSYSAPI EXPORTNUM(190) ULONGLONG NTAPI MmGetPhysicalAddress(IN PVOID pBaseAddress); 
// 0x00BF - MmIsAddressValid
NTSYSAPI EXPORTNUM(191) BOOLEAN NTAPI MmIsAddressValid(IN ULONGLONG qwAddress); 
// 0x00CF - NtClose
NTSYSAPI EXPORTNUM(207) NTSTATUS NTAPI NtClose(IN HANDLE hHandle); 
// 0x00D2 - NtCreateFile
NTSYSAPI EXPORTNUM(210) NTSTATUS NTAPI NtCreateFile( OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes, IN ULONG ShareAccess, IN ULONG CreateDisposition, IN ULONG CreateOptions );  
// 0x00D3 - NtCreateIoCompletion
NTSYSAPI EXPORTNUM(211) NTSTATUS NTAPI NtCreateIoCompletion( OUT PHANDLE IoCompletionHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN ULONG Count OPTIONAL );  
// 0x00D8 - NtDeleteFile
NTSYSAPI EXPORTNUM(216) NTSTATUS NTAPI NtDeleteFile( IN POBJECT_ATTRIBUTES ObjectAttributes );  
// 0x00D9 - NtDeviceIoControlFile
NTSYSAPI EXPORTNUM(217) NTSTATUS NTAPI NtDeviceIoControlFile(IN HANDLE hFileHandle, IN HANDLE hEvent OPTIONAL, IN PIO_APC_ROUTINE pApcRoutine OPTIONAL, IN PVOID pApcContext OPTIONAL, OUT PIO_STATUS_BLOCK pIoStatusBlock, IN ULONG qwIoControlCode, IN PVOID pInputBuffer OPTIONAL, IN ULONG qwInputBufferLength, OUT PVOID pOutputBuffer OPTIONAL, IN ULONG qwOutputBufferLength); 
// 0x00DB - NtFlushBuffersFile
NTSYSAPI EXPORTNUM(219) NTSTATUS NTAPI NtFlushBuffersFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock );  
// 0x00DF - NtOpenFile
NTSYSAPI EXPORTNUM(223) NTSTATUS NTAPI NtOpenFile(OUT PHANDLE hFileHandle, IN ACCESS_MASK dwDesiredAccess, IN POBJECT_ATTRIBUTES pObjectAttributes, OUT PIO_STATUS_BLOCK pIoStatusBlock, IN ULONG qwShareAccess, IN ULONG qwOpenOptions); 
// 0x00E0 - NtOpenSymbolicLinkObject
NTSYSAPI EXPORTNUM(224) NTSTATUS NTAPI NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle, IN POBJECT_ATTRIBUTES ObjectAttributes); 
// 0x00E4 - NtQueryDirectoryFile
NTSYSAPI EXPORTNUM(228) NTSTATUS NTAPI NtQueryDirectoryFile( IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN FILE_INFORMATION_CLASS FileInformationClass, IN POBJECT_STRING FileName OPTIONAL, IN BOOLEAN RestartScan );  
// 0x00E7 - NtQueryFullAttributesFile
NTSYSAPI EXPORTNUM(231) NTSTATUS NTAPI NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PFILE_NETWORK_OPEN_INFORMATION Attributes); 
// 0x00E8 - NtQueryInformationFile
NTSYSAPI EXPORTNUM(232) NTSTATUS NTAPI NtQueryInformationFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN FILE_INFORMATION_CLASS FileInformationClass );  
// 0x00E9 - NtQueryIoCompletion
NTSYSAPI EXPORTNUM(233) NTSTATUS NTAPI NtQueryIoCompletion( IN HANDLE IoCompletionHandle, OUT PVOID IoCompletionInformation );  
// 0x00EC - NtQuerySymbolicLinkObject
NTSYSAPI EXPORTNUM(236) NTSTATUS NTAPI NtQuerySymbolicLinkObject(IN HANDLE LinkHandle, IN OUT PSTRING LinkTarget, OUT PULONG ReturnedLength OPTIONAL); 
// 0x00EF - NtQueryVolumeInformationFile
NTSYSAPI EXPORTNUM(239) NTSTATUS NTAPI NtQueryVolumeInformationFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID FsInformation, IN ULONG Length, IN FS_INFORMATION_CLASS FsInformationClass );  
// 0x00F0 - NtReadFile
NTSYSAPI EXPORTNUM(240) NTSTATUS NTAPI NtReadFile( IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL );  
// 0x00F1 - NtReadFileScatter
NTSYSAPI EXPORTNUM(241) NTSTATUS NTAPI NtReadFileScatter( IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PFILE_SEGMENT_ELEMENT SegmentArray, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL );  
// 0x00F4 - NtRemoveIoCompletion
NTSYSAPI EXPORTNUM(244) NTSTATUS NTAPI NtRemoveIoCompletion( IN HANDLE IoCompletionHandle, OUT PVOID *KeyContext, OUT PVOID *ApcContext, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PLARGE_INTEGER Timeout );  
// 0x00F7 - NtSetInformationFile
NTSYSAPI EXPORTNUM(247) NTSTATUS NTAPI NtSetInformationFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PVOID FileInformation, IN ULONG Length, IN FILE_INFORMATION_CLASS FileInformationClass );  
// 0x00F8 - NtSetIoCompletion
NTSYSAPI EXPORTNUM(248) NTSTATUS NTAPI NtSetIoCompletion( IN HANDLE IoCompletionHandle, IN PVOID KeyContext, IN PVOID ApcContext, IN NTSTATUS IoStatus, IN ULONG_PTR IoStatusInformation );  
// 0x00F9 - NtSetSystemTime
NTSYSAPI EXPORTNUM(249) DWORD NTAPI NtSetSystemTime(ULONG qwNewTime, ULONG qwOldTime OPTIONAL); 
// 0x00FF - NtWriteFile
NTSYSAPI EXPORTNUM(255) NTSTATUS NTAPI NtWriteFile( IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL );  
// 0x0100 - NtWriteFileGather
NTSYSAPI EXPORTNUM(256) NTSTATUS NTAPI NtWriteFileGather( IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, IN PFILE_SEGMENT_ELEMENT SegmentArray, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL );  
// 0x0102 - ObCreateObject
NTSYSAPI EXPORTNUM(258) NTSTATUS NTAPI ObCreateObject( IN POBJECT_TYPE ObjectType, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN ULONG ObjectBodySize, OUT PVOID *Object );  
// 0x0103 - ObCreateSymbolicLink
NTSYSAPI EXPORTNUM(259) HRESULT NTAPI ObCreateSymbolicLink(IN PSTRING SymbolicLinkName, IN PSTRING DeviceName); 
// 0x0104 - ObDeleteSymbolicLink
NTSYSAPI EXPORTNUM(260) HRESULT NTAPI ObDeleteSymbolicLink(IN PSTRING SymbolicLinkName); 
// 0x0105 - ObDereferenceObject
NTSYSAPI EXPORTNUM(261) void NTAPI ObDereferenceObject( IN PVOID Object );  
// 0x0106 - ObDirectoryObjectType;
NTSYSAPI EXPORTNUM(262) extern POBJECT_TYPE ObDirectoryObjectType; 
// 0x0108 - ObInsertObject
NTSYSAPI EXPORTNUM(264) NTSTATUS NTAPI ObInsertObject( IN PVOID Object, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN ULONG ObjectPointerBias, OUT PHANDLE Handle );  
// 0x010C - ObMakeTemporaryObject
NTSYSAPI EXPORTNUM(268) void NTAPI ObMakeTemporaryObject( IN PVOID Object );  
// 0x010D - ObOpenObjectByName
NTSYSAPI EXPORTNUM(269) NTSTATUS NTAPI ObOpenObjectByName( IN POBJECT_ATTRIBUTES ObjectAttributes, IN POBJECT_TYPE ObjectType, IN OUT PVOID ParseContext OPTIONAL, OUT PHANDLE Handle );  
// 0x010E - ObOpenObjectByPointer
NTSYSAPI EXPORTNUM(270) NTSTATUS NTAPI ObOpenObjectByPointer( IN PVOID Object, IN POBJECT_TYPE ObjectType, OUT PHANDLE Handle );  
// 0x010F - ObReferenceObject
NTSYSAPI EXPORTNUM(271) void NTAPI ObReferenceObject( IN PVOID Object );  
// 0x0110 - ObReferenceObjectByHandle
NTSYSAPI EXPORTNUM(272) NTSTATUS NTAPI ObReferenceObjectByHandle( IN HANDLE Handle, IN POBJECT_TYPE ObjectType OPTIONAL, OUT PVOID *Object );  
// 0x0111 - ObReferenceObjectByName
NTSYSAPI EXPORTNUM(273) NTSTATUS NTAPI ObReferenceObjectByName( IN POBJECT_STRING ObjectName, IN ULONG Attributes, IN POBJECT_TYPE ObjectType, IN OUT PVOID ParseContext OPTIONAL, OUT PVOID *Object );  
// 0x0112 - ObSymbolicLinkObjectType;
NTSYSAPI EXPORTNUM(274) extern POBJECT_TYPE ObSymbolicLinkObjectType; 
// 0x0114 - RtlAnsiStringToUnicodeString
NTSYSAPI EXPORTNUM(276) NTSTATUS NTAPI RtlAnsiStringToUnicodeString( PUNICODE_STRING DestinationString, PANSI_STRING SourceString, BOOLEAN AllocateDestinationString );  
// 0x0115 - RtlAppendStringToString
NTSYSAPI EXPORTNUM(277) NTSTATUS NTAPI RtlAppendStringToString( PSTRING Destination, PSTRING Source );  
// 0x0116 - RtlAppendUnicodeStringToString
NTSYSAPI EXPORTNUM(278) NTSTATUS NTAPI RtlAppendUnicodeStringToString( PUNICODE_STRING Destination, PUNICODE_STRING Source );  
// 0x0117 - RtlAppendUnicodeToString
NTSYSAPI EXPORTNUM(279) NTSTATUS NTAPI RtlAppendUnicodeToString( PUNICODE_STRING Destination, PCWSTR Source );  
// 0x0118 - RtlAssert
NTSYSAPI EXPORTNUM(280) void NTAPI RtlAssert( PVOID FailedAssertion, PVOID FileName, ULONG LineNumber, PCHAR Message );  
// 0x011A - RtlCompareMemory
NTSYSAPI EXPORTNUM(282) SIZE_T NTAPI RtlCompareMemory( const void *Source1, const void *Source2, SIZE_T Length );  
// 0x011B - RtlCompareMemoryUlong
NTSYSAPI EXPORTNUM(283) SIZE_T NTAPI RtlCompareMemoryUlong( PVOID Source, SIZE_T Length, ULONG Pattern );  
// 0x011C - RtlCompareString
NTSYSAPI EXPORTNUM(284) ULONG NTAPI RtlCompareString( PSTRING String1, PSTRING String2, BOOLEAN CaseInSensitive );  
// 0x011E - RtlCompareUnicodeString
NTSYSAPI EXPORTNUM(286) ULONG NTAPI RtlCompareUnicodeString( PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive );  
// 0x0121 - RtlCopyString
NTSYSAPI EXPORTNUM(289) void NTAPI RtlCopyString( PSTRING DestinationString, PSTRING SourceString );  
// 0x0122 - RtlCopyUnicodeString
NTSYSAPI EXPORTNUM(290) void NTAPI RtlCopyUnicodeString( PUNICODE_STRING DestinationString, PUNICODE_STRING SourceString );  
// 0x0123 - RtlCreateUnicodeString
NTSYSAPI EXPORTNUM(291) BOOLEAN NTAPI RtlCreateUnicodeString( OUT PUNICODE_STRING DestinationString, IN PCWSTR SourceString );  
// 0x0126 - RtlFillMemoryUlong
//NTSYSAPI EXPORTNUM(294) void NTAPI RtlFillMemory(PVOID Destination, SIZE_T Length, UCHAR Fill );  
// 0x0127 - RtlFreeAnsiString
NTSYSAPI EXPORTNUM(295) void NTAPI RtlFreeAnsiString( PANSI_STRING AnsiString );  
// 0x0129 - RtlGetCallersAddress
NTSYSAPI EXPORTNUM(297) void NTAPI RtlGetCallersAddress( OUT PVOID *CallersAddress, OUT PVOID *CallersCaller );  
// 0x012A - RtlGetStackLimits
NTSYSAPI EXPORTNUM(298) void NTAPI RtlGetStackLimits(PDWORD pdwLimitA, PDWORD pdwLimitB); 
// 0x012B - RtlImageXexHeaderField
NTSYSAPI EXPORTNUM(299) PVOID NTAPI RtlImageXexHeaderField(IN PVOID XexHeaderBase, IN DWORD ImageField); 
// 0x012C - RtlInitAnsiString
NTSYSAPI EXPORTNUM(300) void NTAPI RtlInitAnsiString(IN OUT PANSI_STRING paszDestString, IN PCSZ pszSrcString); 
// 0x012D - RtlInitUnicodeString
NTSYSAPI EXPORTNUM(301) void NTAPI RtlInitUnicodeString( PUNICODE_STRING DestinationString, PCWSTR SourceString );  
// 0x0133 - RtlMultiByteToUnicodeN
NTSYSAPI EXPORTNUM(307) NTSTATUS NTAPI RtlMultiByteToUnicodeN( PWSTR UnicodeString, ULONG MaxBytesInUnicodeString, PULONG BytesInUnicodeString, PCHAR MultiByteString, ULONG BytesInMultiByteString );  
// 0x0134 - RtlMultiByteToUnicodeSize
NTSYSAPI EXPORTNUM(308) NTSTATUS NTAPI RtlMultiByteToUnicodeSize( PULONG BytesInUnicodeString, PCHAR MultiByteString, ULONG BytesInMultiByteString );  
// 0x0135 - RtlNtStatusToDosError
NTSYSAPI EXPORTNUM(309) ULONG NTAPI RtlNtStatusToDosError( NTSTATUS Status );  
// 0x013F - RtlTimeFieldsToTime
NTSYSAPI EXPORTNUM(319) BOOLEAN NTAPI RtlTimeFieldsToTime( PVOID TimeFields, PLARGE_INTEGER Time );  
// 0x0140 - RtlTimeToTimeFields
NTSYSAPI EXPORTNUM(320) void NTAPI RtlTimeToTimeFields( PLARGE_INTEGER Time, PVOID TimeFields );  
// 0x0142 - RtlUnicodeStringToAnsiString
NTSYSAPI EXPORTNUM(322) NTSTATUS NTAPI RtlUnicodeStringToAnsiString( PANSI_STRING DestinationString, PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString );  
// 0x0143 - RtlUnicodeToMultiByteN
NTSYSAPI EXPORTNUM(323) NTSTATUS NTAPI RtlUnicodeToMultiByteN( PCHAR MultiByteString, ULONG MaxBytesInMultiByteString, PULONG BytesInMultiByteString, PWSTR UnicodeString, ULONG BytesInUnicodeString );  
// 0x0144 - RtlUnicodeToMultiByteSize
NTSYSAPI EXPORTNUM(324) NTSTATUS NTAPI RtlUnicodeToMultiByteSize( PULONG BytesInMultiByteString, PWSTR UnicodeString, ULONG BytesInUnicodeString );  
// 0x0149 - RtlUpcaseUnicodeChar
NTSYSAPI EXPORTNUM(329) WCHAR NTAPI RtlUpcaseUnicodeChar( WCHAR SourceCharacter );  
// 0x014A - RtlUpperChar
NTSYSAPI EXPORTNUM(330) CHAR NTAPI RtlUpperChar( CHAR Character );  
// 0x0156 - XboxHardwareInfo;
NTSYSAPI EXPORTNUM(342) extern PXBOX_HARDWARE_INFO XboxHardwareInfo; 
// 0x0157 - XboxKrnlBaseVersion;
NTSYSAPI EXPORTNUM(343) extern PXBOX_KRNL_VERSION XboxKrnlBaseVersion; 
// 0x0158 - XboxKrnlVersion;
NTSYSAPI EXPORTNUM(344) extern PXBOX_KRNL_VERSION XboxKrnlVersion; 
// 0x0193 - XexExecutableModuleHandle;
NTSYSAPI EXPORTNUM(403) extern PLDR_DATA_TABLE_ENTRY* XexExecutableModuleHandle; 
// 0x0194 - XexCheckExecutable
NTSYSAPI EXPORTNUM(404) BOOLEAN NTAPI XexCheckExecutablePrivilege(IN DWORD dwPrivilege); 
// 0x0195 - XexGetModuleHandle
NTSYSAPI EXPORTNUM(405) DWORD NTAPI XexGetModuleHandle(IN PSZ szModuleName, IN OUT PHANDLE dwHandle); 
// 0x0197 - XexGetProcedureAddress
NTSYSAPI EXPORTNUM(407) DWORD NTAPI XexGetProcedureAddress(IN UINT32 dwHandle, IN DWORD dwOrdinal, IN PVOID pAddress); 
// 0x0198 - XexLoadExecutable
NTSYSAPI EXPORTNUM(408) DWORD NTAPI XexLoadExecutable(IN PSZ szModulePath, IN OUT PHANDLE dwHandle, IN DWORD dwTypeInfo, IN DWORD dwMinVersion); 
// 0x0199 - XexLoadImage
NTSYSAPI EXPORTNUM(409) DWORD NTAPI XexLoadImage(IN PSZ szModulePath, IN DWORD dwTypeInfo, IN DWORD dwMinVersion, IN OUT PHANDLE dwHandle); 
// 0x019C - XexPcToFileHeader
NTSYSAPI EXPORTNUM(412) PVOID NTAPI XexPcToFileHeader( IN		PVOID address, OUT		PLDR_DATA_TABLE_ENTRY* ldatOut );  
// 0x01A6 - DbgLoadImageSymbols
NTSYSAPI EXPORTNUM(422) void NTAPI DbgLoadImageSymbols( PSTRING FileName, PVOID ImageBase, ULONG_PTR ProcessId );  
// 0x01A7 - DbgUnLoadImageSymbols
NTSYSAPI EXPORTNUM(423) void NTAPI DbgUnLoadImageSymbols( PSTRING FileName, PVOID ImageBase, ULONG_PTR ProcessId );  
// 0x01A8 - RtlImageDirectoryEntryToData
NTSYSAPI EXPORTNUM(424) PVOID NTAPI RtlImageDirectoryEntryToData(PVOID baseAddr, BOOLEAN bMappedAsImg, WORD wDirEntry, PDWORD pdwSize); 
// 0x01A9 - RtlImageNtHeader
NTSYSAPI EXPORTNUM(425) PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader( PVOID Base );  
// 0x01AF - ExLoadedImageName;
NTSYSAPI EXPORTNUM(431) extern PCHAR* ExLoadedImageName; 
// 0x01BA - VdGetCurrentDisplayInformation
NTSYSAPI EXPORTNUM(442) DWORD NTAPI VdGetCurrentDisplayInformation(IN PVOID pDisplayInfo); 
// 0x01E6 - XInputdReadState
NTSYSAPI EXPORTNUM(486) NTSTATUS NTAPI XInputdReadState(IN PDWORD pdwDeviceContext, OUT PDWORD pdwPacketNumber, OUT PXINPUT_GAMEPAD pInputData); 
// 0x0241 - XeKeysGetKeyProperties
NTSYSAPI EXPORTNUM(577) DWORD NTAPI XeKeysGetKeyProperties(DWORD dwKeyIdx); 
// 0x0242 - XeKeysSetKey
NTSYSAPI EXPORTNUM(578) NTSTATUS NTAPI XeKeysSetKey(DWORD dwKeyIdx, PVOID pInputBuffer, DWORD dwInputBufferSize); 
// 0x0244 - XeKeysGetKey
NTSYSAPI EXPORTNUM(580) NTSTATUS NTAPI XeKeysGetKey(DWORD dwKeyIdx, PVOID pOutputBuffer, PDWORD pOutputBufferSize); 
// 0x0247 - XeKeysGetConsoleType
NTSYSAPI EXPORTNUM(583) NTSTATUS NTAPI XeKeysGetConsoleType(PWORD pConsoleType); 
// 0x0254 - XeKeysObfuscate
NTSYSAPI EXPORTNUM(596) NTSTATUS NTAPI XeKeysObfuscate(DWORD dwKeyIdx, PVOID pInputBuffer, DWORD dwInputBufferLength, PVOID pOutputBuffer, PDWORD pOutputBufferLength); 
// 0x0255 - XeKeysUnobfuscate
NTSYSAPI EXPORTNUM(597) NTSTATUS NTAPI XeKeysUnobfuscate(DWORD dwKeyIdx, PVOID pInputBuffer, DWORD dwInputBufferLength, PVOID pOutputBuffer, PDWORD pOutputBufferLength); 
// 0x0263 - AniTerminateAnimation
NTSYSAPI EXPORTNUM(611) void NTAPI AniTerminateAnimation(void); 
// 0x026C - HalGetPowerUpCause
NTSYSAPI EXPORTNUM(620) void NTAPI HalGetPowerUpCause( PBYTE reply );  
// 0x02B8 - NtCancelIoFile
NTSYSAPI EXPORTNUM(696) NTSTATUS NTAPI NtCancelIoFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock );  
// 0x02B9 - NtCancelIoFileEx
NTSYSAPI EXPORTNUM(697) NTSTATUS NTAPI NtCancelIoFile( IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock );  
// 0x02CD - RtlCaptureStackBackTrace
NTSYSAPI EXPORTNUM(717) USHORT NTAPI RtlCaptureStackBackTrace( IN ULONG FramesToSkip, IN ULONG FramesToCapture, OUT PVOID *BackTrace, OUT PULONG BackTraceHash );  
// 0x02E2 - LDIResetDecompression
NTSYSAPI EXPORTNUM(738) NTSTATUS NTAPI LDIResetDecompression(DWORD dwContext);
// 0x02F0 - KeRemoveQueueApc
NTSYSAPI EXPORTNUM(752) BOOLEAN NTAPI KeRemoveQueueApc(IN PVOID Apc); 


// 0x26E : RgcMonitorReportBuffersEnd(
// 0x2B1 : MicDeviceRequest(
// 0x2C7 : SataCdRomRecordReset(
// 0x2CC : SvodCreateDevice(
// 0x2E1 : RmcDeviceRequest(
// 0x2F5 : missing?(
// 0x2F8 : NomnilGetExtension(
// 0x2F9 : NomnilStartCloseDevice(
// 0x306 : NomnilSetLed(
// 0x30D : PsCamDeviceRequest(
// 0x30E : McaDeviceRequest(
// 0x30F : DetroitDeviceRequest(
// 0x329 : DevAuthGetStatistics(
// 0x359 : DevAuthShouldAlwaysEnforce(
// 0x32A : NullCableRequest(
// 0x345 : XboxKrnlVersion4Digit(
// 0x362 : EmaExecute(
// 0x377 : TidDeviceRequest(

// 0x005 : DumpGetRawDumpInfo(
// 0x006 : DumpWriteDump(
// 0x274 : DumpSetCollectionFacility(
// 0x304 : DumpXitThread(
// 0x32C : DumpUpdateDumpSettings(
// 0x33C : DumpRegisterDedicatedDataBlock(

// 0x04D : KeAcquireSpinLockAtRaisedIrql(
// 0x050 : DWORD KeBlowFuses(DWORD var);
// 0x058 : KeCreateUserMode(
// 0x059 : extern PVOID KeDebugMonitorData;
// 0x05B : KeDeleteUserMode(
// 0x05D : KeEnableFpuExceptions(
// 0x05E : KeEnablePPUPerformanceMonitor(
// 0x060 : KeEnterUserMode(
// 0x061 : void KeFlushCacheRange(LPVOID, DWORD, DWORD);
// 0x062 : VOID KeFlushCurrentEntireTb(IN BOOLEAN ZeroEntireTb);
// 0x063 : VOID KeFlushEntireTb(IN BOOLEAN Invalid, IN BOOLEAN AllProcessors);
// 0x064 : KeFlushUserModeCurrentTb(
// 0x065 : KeFlushUserModeTb(
// 0x067 : KeGetPMWRegister(
// 0x068 : KeGetPRVRegister(
// 0x069 : KeGetSocRegister(
// 0x06A : KeGetSpecialPurposeRegister(
// 0x06B : KeLockL2(
// 0x06C : KeUnlockL2(
// 0x07C : ULONG_PTR KeIpiGenericCall(IN PKIPI_BROADCAST_WORKER BroadcastFunction, IN ULONG_PTR Context);
// 0x07E : KeLeaveUserMode(
// 0x080 : KeQueryBackgroundProcessors(
// 0x083 : DWORD KeQueryPerformanceFrequency();
// 0x086 : KeRegisterDriverNotification(
// 0x090 : NTSTATUS KeRestoreFloatingPointState(IN PKFLOATING_SAVE FloatSave);
// 0x091 : KeRestoreVectorUnitState(
// 0x093 : VOID KeRetireDpcList(IN PKPRCB Prcb);
// 0x095 : NTSTATUS KeSaveFloatingPointState(OUT PKFLOATING_SAVE FloatSave);
// 0x096 : KeSaveVectorUnitState(
// 0x097 : KAFFINITY KeSetAffinityThread(IN PVOID Thread, IN KAFFINITY Affinity);
// 0x098 : KeSetBackgroundProcessors(
// 0x09B : KeSetCurrentStackPointers(
// 0x09F : KeSetPMWRegister(
// 0x0A0 : KeSetPowerMode(
// 0x0A1 : KeSetPRVRegister(
// 0x0A2 : KeSetPriorityClassThread(
// 0x0A4 : KeSetSocRegister(
// 0x0A5 : KeSetSpecialPurposeRegister(
// 0x0AA : VOID KeSweepDcacheRange(IN BOOLEAN AllProcessors, IN PVOID BaseAddress, IN ULONG Length);
// 0x0AB : VOID KeSweepIcacheRange(IN BOOLEAN AllProcessors, IN PVOID BaseAddress, IN ULONG Length);
// 0x0AD : extern DWORD* KeTimeStampBundle;
// 0x0AE : KeTryToAcquireSpinLockAtRaisedIrql(
// 0x152 : DWORD KeTlsAlloc(void);
// 0x153 : BOOL KeTlsFree(IN DWORD dwTlsIndex);
// 0x154 : LPVOID KeTlsGetValue(IN DWORD dwTlsIndex);
// 0x155 : BOOL KeTlsSetValue(IN DWORD dwTlsIndex, IN LPVOID lpTlsValue OPTIONAL);
// 0x1DF : KiApcNormalRoutineNop(
// 0x266 : extern PVOID KeCertMonitorData;
// 0x29D : KeEnablePFMInterrupt(
// 0x29E : KeDisablePFMInterrupt(
// 0x29F : VOID KeSetProfilerISR(PVOID RoutineAddr);
// 0x2A4 : KeGetImagePageTableEntry(
// 0x2B4 : KeGetVidInfo(
// 0x2D8 : VOID KeFlushMultipleTb(DWORD Number, PVOID Virtual, PVOID Invalid, BOOL AllProcessors, PVOID PtePointer OPTIONAL, DWORD PteValue OPTIONAL);
// 0x35D : KeExecuteOnProtectedStack(
// 0x375 : KeCallAndBlockOnDpcRoutine(
// 0x376 : KeCallAndWaitForDpcRoutine(

// 0x01D : MmDoubleMapMemory(
// 0x01E : MmUnmapMemory(
// 0x0B9 : PVOID MmAllocatePhysicalMemory(ULONG bufferSize, PHYSICAL_ADDRESS unk, DWORD mode, OUT PPHYSICAL_ADDRESS physAddr);
// 0x0BA : PVOID MmAllocatePhysicalMemoryEx(
// 0x0BB : PVOID MmCreateKernelStack(BOOLEAN GuiStack, UCHAR Node);
// 0x0BC : VOID MmDeleteKernelStack(PVOID Stack, BOOLEAN GuiStack);
// 0x0BD : VOID MmFreePhysicalMemory(DWORD unk2, PVOID pMem);
// 0x0C0 : MmLockAndMapSegmentArray(
// 0x0C1 : VOID MmLockUnlockBufferPages(PVOID BaseAddress, DWORD NumBytes, BOOLEAN UnlockPages);
// 0x0C2 : PVOID MmMapIoSpace(PHYSICAL_ADDRESS PhysAddress, DWORD NumBytes, MEMORY_CACHING_TYPE CacheType);
// 0x0C3 : MmPersistPhysicalMemoryAllocation(
// 0x0C4 : DWORD MmQueryAddressProtect(PVOID VirtualAddress);
// 0x0C5 : DWORD MmQueryAllocationSize(PVOID BaseAddress);
// 0x0C6 : NTSTATUS MmQueryStatistics(PMM_STATISTICS MemoryStatistics);
// 0x0C7 : DWORD MmSetAddressProtect(PVOID BaseAddress, DWORD NumberOfBytes, DWORD NewProtect);
// 0x0C8 : MmSplitPhysicalMemoryAllocation(
// 0x0C9 : MmUnlockAndUnmapSegmentArray(
// 0x0CA : MmUnmapIoSpace(
// 0x1AB : PVOID MmDbgReadCheck( PVOID VirtualAddress );
// 0x1AC : VOID MmDbgReleaseAddress( PVOID VirtualAddress, PHARDWARE_PTE Opaque);
// 0x1AD : PVOID MmDbgWriteCheck( PVOID VirtualAddress, PHARDWARE_PTE Opaque);
// 0x30A : MmGetPoolPagesType(

// 0x0CB : Nls844UnicodeCaseTable(
// 0x0CC : NTSTATUS NtAllocateVirtualMemory(IN HANDLE ProcessHandle, IN OUT  PVOID *BaseAddress, IN     ULONG_PTR ZeroBits, IN OUT  PSIZE_T RegionSize, IN     ULONG AllocationType, IN     ULONG Protect);
// 0x0CD : NTSTATUS NtCancelTimer(IN HANDLE TimerHandle, OUT PBOOLEAN CurrentState OPTIONAL);
// 0x0CE : NTSTATUS NtClearEvent(IN HANDLE EventHandle);
// 0x0D0 : NTSTATUS NtCreateDirectoryObject(OUT PHANDLE DirectoryHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes);
// 0x0D1 : NTSTATUS NtCreateEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN EVENT_TYPE EventType, IN BOOLEAN InitialState);
// 0x0D4 : NTSTATUS NtCreateMutant(OUT PHANDLE MutantHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN BOOLEAN InitialOwner);
// 0x0D5 : NTSTATUS NtCreateSemaphore(OUT PHANDLE 	SemaphoreHandle, IN ACCESS_MASK 	DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes 	OPTIONAL, IN LONG 	InitialCount, IN LONG 	MaximumCount );
// 0x0D6 : NTSTATUS NtCreateSymbolicLinkObject(OUT PHANDLE pHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN PUNICODE_STRING DestName);
// 0x0D7 : NTSTATUS NtCreateTimer(OUT PHANDLE TimerHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, IN TIMER_TYPE TimerType);
// 0x0DA : NTSTATUS NtDuplicateObject(IN HANDLE SrcProcessHandle, IN PHANDLE SrcHandle, IN HANDLE TargetProcessHandle, OUT PHANDLE TargetHandle, IN ACCESS_MASK DesiredAccess OPTIONAL, IN BOOLEAN InheritHandle, IN ULONG Options);
// 0x0DC : NTSTATUS NtFreeVirtualMemory(IN HANDLE ProcessHandle, IN PVOID *BaseAddress, IN OUT PULONG RegionSize, IN ULONG FreeType);
// 0x0DD : NTSTATUS NtMakeTemporaryObject(IN HANDLE ObjectHandle);
// 0x0DE : NTSTATUS NtOpenDirectoryObject(OUT PHANDLE DirectoryHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes);
// 0x0E1 : NTSTATUS NtProtectVirtualMemory(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN OUT PULONG NumberOfBytesToProtect, IN ULONG NewAccessProtection, OUT PULONG OldAccessProtection );
// 0x0E2 : NTSTATUS NtPulseEvent(IN HANDLE EventHandle, OUT PLONG PreviousState OPTIONAL);
// 0x0E3 : NTSTATUS NtQueueApcThread(IN HANDLE ThreadHandle, IN PIO_APC_ROUTINE ApcRoutine, IN PVOID ApcRoutineContext OPTIONAL, IN PIO_STATUS_BLOCK ApcStatusBlock OPTIONAL, IN ULONG ApcReserved OPTIONAL );
// 0x0E5 : NTSTATUS NtQueryDirectoryObject(IN HANDLE DirectoryObjectHandle, OUT POBJDIR_INFORMATION DirObjInformation, IN ULONG BufferLength, IN BOOLEAN GetNextIndex, IN BOOLEAN IgnoreInputIndex, IN OUT PULONG ObjectIndex, OUT PULONG DataWritten OPTIONAL );
// 0x0E6 : NTSTATUS NtQueryEvent(IN HANDLE EventHandle, IN EVENT_INFORMATION_CLASS EventInformationClass, OUT PVOID EventInformation, IN ULONG EventInformationLength, OUT PULONG ReturnLength OPTIONAL );
// 0x0EA : NTSTATUS NtQueryMutant(IN HANDLE MutantHandle, IN MUTANT_INFORMATION_CLASS MutantInformationClass, OUT PVOID MutantInformation, IN ULONG MutantInformationLength, OUT PULONG ResultLength OPTIONAL );
// 0x0EB : NTSTATUS NtQuerySemaphore(IN HANDLE SemaphoreHandle, IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass, OUT PVOID SemaphoreInformation, IN ULONG SemaphoreInformationLength, OUT PULONG ReturnLength OPTIONAL );
// 0x0ED : NTSTATUS NtQueryTimer(IN HANDLE TimerHandle, IN TIMER_INFORMATION_CLASS TimerInformationClass,  IN ULONG TimerInformationLength, OUT PULONG ReturnLength OPTIONAL );

// 0x0EE : NTSTATUS NtQueryVirtualMemory(IN HANDLE ProcessHandle, IN PVOID BaseAddress, IN MEMORY_INFORMATION_CLASS MemoryInformationClass, OUT PVOID Buffer, IN ULONG Length, OUT PULONG ResultLength OPTIONAL );
// 0x0F2 : NTSTATUS NtReleaseMutant(IN HANDLE MutantHandle, OUT PLONG PreviousCount OPTIONAL );
// 0x0F3 : NTSTATUS NtReleaseSemaphore(IN HANDLE SemaphoreHandle, IN ULONG ReleaseCount, OUT PULONG PreviousCount OPTIONAL );
// 0x0F5 : NTSTATUS NtResumeThread(IN HANDLE ThreadHandle, OUT PULONG SuspendCount OPTIONAL );
// 0x0F6 : NTSTATUS NtSetEvent(IN HANDLE EventHandle, OUT PLONG PreviousState OPTIONAL );
// 0x0FA : NTSTATUS NtSetTimerEx(IN HANDLE TimerHandle, IN TIMER_INFORMATION_CLASS TimerSetInformationClass, IN OUT PVOID TimerSetInformation, IN ULONG Length);
// 0x0FB : NtSignalAndWaitForSingleObjectEx(
// 0x0FC : NTSTATUS NtSuspendThread(IN HANDLE ThreadHandle, OUT PULONG PreviousSuspendCount OPTIONAL);
// 0x0FD : NtWaitForSingleObjectEx(
// 0x0FE : NtWaitForMultipleObjectsEx(
// 0x101 : NTSTATUS NtYieldExecution();
// 0x28A : NtAllocateEncryptedMemory(
// 0x28B : NtFreeEncryptedMemory(

// 0x107 : ObGetWaitableObject(
// 0x109 : ObIsTitleObject(
// 0x10A : ObLookupAnyThreadByThreadId(
// 0x10B : ObLookupThreadByThreadId(
// 0x113 : ObTranslateSymbolicLink(

// 0x14C : int _vscprintf( const char *format, va_list argptr );
// 0x14D : int _vsnprintf( char *buffer, size_t count, const char *format, va_list argptr );
// 0x14E : int vsprintf (char * str, const char * format, va_list arg );
// 0x14F : int _vscwprintf(const wchar_t *format, va_list argptr);
// 0x150 : int _vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list argptr );
// 0x151 : int _vswprintf(wchar_t *buffer, const wchar_t *format, va_list argptr);
// 0x19D : KiApcNormalRoutineNop(
// 0x1A5 : extern EXCEPTION_ROUTINE __C_specific_handler;

// 0x1EA : HidGetCapabilities(
// 0x1EB : HidReadKeys(
// 0x1F1 : HidGetLastInputTime(
// 0x273 : HidReadMouseChanges(

// 0x211 : MtpdBeginTransaction(
// 0x212 : MtpdCancelTransaction(
// 0x213 : MtpdEndTransaction(
// 0x214 : MtpdGetCurrentDevices(
// 0x215 : MtpdReadData(
// 0x216 : MtpdReadEvent(
// 0x217 : MtpdResetDevice(
// 0x218 : MtpdSendData(
// 0x219 : MtpdVerifyProximity(

// 0x262 : AniBlockOnAnimation(
// 0x265 : AniSetLogo(
// 0x2A6 : AniStartBootAnimation(

// 0x020 : FscGetCacheElementCount(
// 0x021 : FscSetCacheElementCount(
// 0x02B : InterlockedFlushSList(
// 0x02C : InterlockedPopEntrySList(
// 0x02D : InterlockedPushEntrySList(
// 0x02E : IoAcquireDeviceObjectLock(
// 0x044 : IoReleaseDeviceObjectLock(
// 0x2B6 : IoAcquireCancelSpinLock(
// 0x2B7 : IoReleaseCancelSpinLock(

// 0x2C3 : IptvSetBoundaryKey(
// 0x2C4 : IptvSetSessionKey(
// 0x2C5 : IptvVerifyOmac1Signature(
// 0x2C6 : IptvGetAesCtrTransform(
// 0x2D5 : IptvGetSessionKeyHash(

// 0x21A : XUsbcamSetCaptureMode(
// 0x21B : XUsbcamGetConfig(
// 0x21C : XUsbcamSetConfig(
// 0x21D : XUsbcamGetState(
// 0x21E : XUsbcamReadFrame(
// 0x21F : XUsbcamSnapshot(
// 0x220 : XUsbcamSetView(
// 0x221 : XUsbcamGetView(
// 0x222 : XUsbcamCreate(
// 0x223 : XUsbcamDestroy(
// 0x264 : XUsbcamReset(

// 0x224 : XMACreateContext(
// 0x225 : XMAInitializeContext(
// 0x226 : XMAReleaseContext(
// 0x227 : XMAEnableContext(
// 0x228 : XMADisableContext(
// 0x229 : XMAGetOutputBufferWriteOffset(
// 0x22A : XMASetOutputBufferReadOffset(
// 0x22B : XMAGetOutputBufferReadOffset(
// 0x22C : XMASetOutputBufferValid(
// 0x22D : XMAIsOutputBufferValid(
// 0x22E : XMASetInputBuffer0Valid(
// 0x22F : XMAIsInputBuffer0Valid(
// 0x230 : XMASetInputBuffer1Valid(
// 0x231 : XMAIsInputBuffer1Valid(
// 0x232 : XMASetInputBuffer0(
// 0x233 : XMASetInputBuffer1(
// 0x234 : XMAGetPacketMetadata(
// 0x235 : XMABlockWhileInUse(
// 0x236 : XMASetLoopData(
// 0x237 : XMASetInputBufferReadOffset(
// 0x238 : XMAGetInputBufferReadOffset(

// 0x20C : DrvSetSysReqCallback(
// 0x20D : DrvSetUserBindingCallback(
// 0x20E : DrvSetContentStorageCallback(
// 0x20F : DrvSetAutobind(
// 0x210 : DrvGetContentStorageNotification(
// 0x278 : DrvXenonButtonPressed(
// 0x279 : DrvBindToUser(
// 0x28F : DrvSetDeviceConfigChangeCallback(
// 0x290 : DrvDeviceConfigChange(
// 0x328 : DrvSetMicArrayStartCallback(

// 0x259 : StfsCreateDevice(
// 0x25A : StfsControlDevice(
// 0x2DC : StfsDeviceErrorEvent(

// 0x1E5 : XInputdGetCapabilities(
// 0x1E7 : XInputdWriteState(
// 0x1E8 : XInputdNotify(
// 0x1E9 : XInputdRawState(
// 0x1EC : XInputdGetDeviceStats(
// 0x1ED : XInputdResetDevice(
// 0x1EE : XInputdSetRingOfLight(
// 0x1EF : XInputdSetRFPowerMode(
// 0x1F0 : XInputdSetRadioFrequency(
// 0x261 : XInputdPowerDownDevice(
// 0x277 : XInputdReadTextKeystroke(
// 0x27F : XInputdSendStayAliveRequest(
// 0x282 : XInputdFFGetDeviceInfo(
// 0x283 : XInputdFFSetEffect(
// 0x284 : XInputdFFUpdateEffect(
// 0x285 : XInputdFFEffectOperation(
// 0x286 : XInputdFFDeviceControl(
// 0x287 : XInputdFFSetDeviceGain(
// 0x288 : XInputdFFCancelIo(
// 0x289 : XInputdFFSetRumble(
// 0x2AD : XInputdGetLastTextInputTime(
// 0x2B0 : XInputdSetTextMessengerIndicator(
// 0x2C8 : XInputdSetTextDeviceKeyLocks(
// 0x2C9 : XInputdGetTextDeviceKeyLocks(
// 0x2E0 : XInputdControl(
// 0x316 : XInputdGetDevicePid(

// 0x022 : HalGetCurrentAVPack(
// 0x023 : HalGpioControl(
// 0x024 : HalOpenCloseODDTray(  #dvdtray
// 0x026 : HalRegisterPowerDownNotification(
// 0x027 : HalRegisterSMCNotification(
// 0x029 : HalSendSMCMessage(
// 0x02A : HalSetAudioEnable(
// 0x25C : HalFsbInterruptCount(
// 0x267 : HalIsExecutingPowerDownDpc(
// 0x27D : HalRegisterPowerDownCallback(
// 0x291 : HalRegisterHdDvdRomNotification(
// 0x2A5 : HalRegisterBackgroundModeTransitionCallback(
// 0x2A7 : HalClampUnclampOutputDACs(
// 0x2A8 : HalPowerDownToBackgroundMode(
// 0x2A9 : HalNotifyAddRemoveBackgroundTask(
// 0x2AA : HalCallBackgroundModeNotificationRoutines(
// 0x2AB : HalFsbResetCount(
// 0x2B5 : HalNotifyBackgroundModeTransitionComplete(
// 0x2BA : HalFinalizePowerLossRecovery(
// 0x2BB : HalSetPowerLossRecovery(
// 0x2BD : HalRegisterXamPowerDownCallback(
// 0x317 : HalGetNotedArgonErrors(
// 0x319 : HalReadArgonEeprom(
// 0x31A : HalWriteArgonEeprom(

// 0x014 : ExRegisterThreadNotification(
// 0x019 : ExTerminateThread(
// 0x01A : ExTerminateTitleProcess(
// 0x042 : ExSetBetaFeaturesEnabled(
// 0x1AA : ExDebugMonitorService(
// 0x1AE : ExLoadedCommandLine(
// 0x239 : ExIsBetaFeatureEnabled(
// 0x2BC : ExReadModifyWriteXConfigSettingUlong(
// 0x2BE : ExCancelAlarm(
// 0x2BF : ExInitializeAlarm(
// 0x2C0 : ExSetAlarm(
// 0x2D0 : ExRegisterXConfigNotification(
// 0x2DD : ExTryToAcquireReadWriteLockExclusive(
// 0x2DE : ExTryToAcquireReadWriteLockShared(
// 0x30B : ExExpansionInstall(
// 0x30C : ExExpansionCall(
// 0x364 : ExFreeDebugPool(

// 0x201 : NicSetUnicastAddress(
// 0x202 : NicAttach(
// 0x203 : NicDetach(
// 0x204 : NicXmit(
// 0x205 : NicUpdateMcastMembership(
// 0x206 : NicFlushXmitQueue(
// 0x207 : NicShutdown(
// 0x208 : NicGetLinkState(
// 0x209 : NicGetStats(
// 0x20A : NicGetOpt(
// 0x20B : NicSetOpt(
// 0x2E3 : NicRegisterDevice(
// 0x2FA : WifiBeginAuthentication(
// 0x2FB : WifiCheckCounterMeasures(
// 0x2FC : WifiChooseAuthenCipherSetFromBSSID(
// 0x2FD : WifiCompleteAuthentication(
// 0x2FE : WifiGetAssociationIE(
// 0x2FF : WifiOnMICError(
// 0x300 : WifiPrepareAuthenticationContext(
// 0x301 : WifiRecvEAPOLPacket(
// 0x302 : WifiDeduceNetworkType(
// 0x303 : NicUnregisterDevice(
// 0x305 : XInputdSetWifiChannel(
// 0x307 : WifiCalculateRegulatoryDomain(
// 0x308 : WifiSelectAdHocChannel(
// 0x309 : WifiChannelToFrequency(

// 0x32D : EtxConsumerDisableEventType(
// 0x32E : EtxConsumerEnableEventType(
// 0x32F : EtxConsumerProcessLogs(
// 0x330 : EtxConsumerRegister(
// 0x331 : EtxConsumerUnregister(
// 0x332 : EtxProducerLog(
// 0x333 : EtxProducerLogV(
// 0x334 : EtxProducerRegister(
// 0x335 : EtxProducerUnregister(
// 0x336 : EtxConsumerFlushBuffers(
// 0x337 : EtxProducerLogXwpp(
// 0x338 : EtxProducerLogXwppV(
// 0x33A : EtxBufferRegister(
// 0x33B : EtxBufferUnregister(

// 0x1E1 : XVoicedHeadsetPresent(
// 0x1E2 : XVoicedSubmitPacket(
// 0x1E3 : XVoicedClose(
// 0x1E4 : XVoicedActivate(
// 0x280 : XVoicedSendVPort(
// 0x281 : XVoicedGetBatteryStatus(

// 0x1F2 : XAudioRenderDriverInitialize(
// 0x1F3 : XAudioRegisterRenderDriverClient(
// 0x1F4 : XAudioUnregisterRenderDriverClient(
// 0x1F5 : XAudioSubmitRenderDriverFrame(
// 0x1F6 : XAudioRenderDriverLock(
// 0x1F7 : XAudioGetVoiceCategoryVolumeChangeMask(
// 0x1F8 : XAudioGetVoiceCategoryVolume(
// 0x1F9 : XAudioSetVoiceCategoryVolume(
// 0x1FA : XAudioBeginDigitalBypassMode(
// 0x1FB : XAudioEndDigitalBypassMode(
// 0x1FC : XAudioSubmitDigitalPacket(
// 0x1FD : XAudioQueryDriverPerformance(
// 0x1FE : XAudioGetRenderDriverThread(
// 0x1FF : XAudioGetSpeakerConfig(
// 0x200 : XAudioSetSpeakerConfig(
// 0x276 : XAudioOverrideSpeakerConfig(
// 0x2D4 : XAudioSuspendRenderDriverClients(
// 0x320 : XAudioRegisterRenderDriverMECClient(
// 0x321 : XAudioUnregisterRenderDriverMECClient(
// 0x322 : XAudioCaptureRenderDriverFrame(
// 0x327 : XVoicedGetDirectionalData(
// 0x34C : XAudioGetRenderDriverTic(
// 0x34D : XAudioEnableDucker(
// 0x34E : XAudioSetDuckerLevel(
// 0x34F : XAudioIsDuckerEnabled(
// 0x350 : XAudioGetDuckerLevel(
// 0x351 : XAudioGetDuckerThreshold(
// 0x352 : XAudioSetDuckerThreshold(
// 0x353 : XAudioGetDuckerAttackTime(
// 0x354 : XAudioSetDuckerAttackTime(
// 0x355 : XAudioGetDuckerReleaseTime(
// 0x356 : XAudioSetDuckerReleaseTime(
// 0x357 : XAudioGetDuckerHoldTime(
// 0x358 : XAudioSetDuckerHoldTime(
// 0x35A : XAudioGetUnderrunCount(
// 0x35C : XVoicedIsActiveProcess(
// 0x36F : XAudioSetProcessFrameCallback(

// 0x119 : void RtlCaptureContext(OUT PCONTEXT ContextRecord);
// 0x11D : RtlCompareStringN(
// 0x11F : RtlCompareUnicodeStringN(
// 0x120 : RtlCompareUtf8ToUnicode(
// 0x124 : WCHAR RtlDowncaseUnicodeChar(IN WCHAR SourceCharacter);
// 0x125 : NTSTATUS RtlEnterCriticalSection(RTL_CRITICAL_SECTION* crit);
// 0x128 : void RtlFreeAnsiString(IN PANSI_STRING AnsiString);
// 0x12E : NTSTATUS RtlInitializeCriticalSection(RTL_CRITICAL_SECTION* crit);
// 0x12F : NTSTATUS RtlInitializeCriticalSectionAndSpinCount(RTL_CRITICAL_SECTION* crit, ULONG spincount);
// 0x130 : NTSTATUS RtlLeaveCriticalSection(RTL_CRITICAL_SECTION* crit);
// 0x131 : PVOID RtlLookupFunctionEntry(IN ULONG ControlPC, OUT PULONG ImageBase, OUT PUNWIND_HISTORY_TABLE HistoryTable);
// 0x132 : CHAR RtlLowerChar(CHAR Character);
// 0x136 : VOID RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord);
// 0x137 : VOID RtlRaiseStatus(IN NTSTATUS Status);
// 0x138 : RtlRip(
// 0x139 : int _scprintf(const char *format [, argument] ... );
// 0x13A : int _snprintf(char *buffer, size_t count, const char *format [, argument] ... );
// 0x13B : int sprintf (char * str, const char * format, ... );
// 0x13C : int _scwprintf(const wchar_t *format [, argument] ... );
// 0x13D : int _snwprintf(wchar_t *buffer, size_t count, const wchar_t *format [, argument] ... );
// 0x13E : int _swprintf(wchar_t *buffer, const wchar_t *format [, argument] ... );
// 0x141 : BOOL RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION* crit);
// 0x145 : RtlUnicodeToUtf8(
// 0x146 : RtlUnicodeToUtf8Size(
// 0x147 : void RtlUnwind(IN PVOID TargetFrame OPTIONAL, IN PVOID TargetIP OPTIONAL, IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL, IN PVOID ReturnValue);
// 0x148 : void RtlUnwind2(IN FRAME_POINTERS TargetFrame OPTIONAL, IN PVOID TargetIP OPTIONAL, IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL, IN PVOID ReturnValue, IN PCONTEXT OriginalContext);
// 0x14B : PEXCEPTION_ROUTINE RtlVirtualUnwind(IN HandlerType, IN ImageBase, IN ControlPC, IN FunctionEntry, IN OUT ContextRecord, OUT InFunction, OUT EstablisherFrame, IN OUT ContextPointers OPTIONAL);
// 0x27B : DWORD RtlComputeCrc32(DWORD dwInitial, const BYTE* pData, INT iLen);
// 0x36D : RtlSetVectoredExceptionHandler(
// 0x36E : RtlClearVectoredExceptionHandler(

// 0x196 : XexGetModuleSection(
// 0x19A : XexLoadImageFromMemory(
// 0x19B : XexLoadImageHeaders(
// 0x19E : XexRegisterPatchDescriptor(
// 0x19F : XexSendDeferredNotifications(
// 0x1A0 : HRESULT XexStartExecutable(FARPROC InitThreadProc);
// 0x1A1 : VOID XexUnloadImage(HANDLE moduleHandle);
// 0x1A2 : VOID XexUnloadImageAndExitThread(HANDLE moduleHandle, HANDLE threadHandle);
// 0x1A3 : XexUnloadTitleModules(
// 0x1A4 : HRESULT XexVerifyImageHeaders(IMAGE_XEX_HEADER* XexHeader, DWORD HeaderSize)
// 0x275 : XexTransformImageKey(
// 0x27A : XexGetModuleImportVersions(
// 0x2C1 : XexActivationGetNonce(
// 0x2C2 : XexActivationSetLicense(
// 0x2CA : XexActivationVerifyOwnership(
// 0x2CB : XexDisableVerboseDbgPrint(
// 0x2CF : XexImportTraceEnable(
// 0x2D3 : XexSetExecutablePrivilege(
// 0x2DF : XexSetLastKdcTime(
// 0x33F : XexShimDisable(
// 0x340 : XexShimEnable(
// 0x341 : XexShimEntryDisable(
// 0x342 : XexShimEntryEnable(
// 0x343 : XexShimEntryRegister(
// 0x344 : XexShimLock(
// 0x348 : XexTitleHash(
// 0x349 : XexTitleHashClose(
// 0x34A : XexTitleHashContinue(
// 0x34B : XexTitleHashOpen(
// 0x36B : XexReserveCodeBuffer(
// 0x36C : XexCommitCodeBuffer(

// 0x1B0 : VdBlockUntilGUIIdle(
// 0x1B1 : VdCallGraphicsNotificationRoutines(
// 0x1B2 : VdDisplayFatalError(
// 0x1B3 : VdEnableClosedCaption(
// 0x1B4 : VdEnableDisableClockGating(
// 0x1B5 : VdEnableDisablePowerSavingMode(
// 0x1B6 : VdEnableRingBufferRPtrWriteBack(
// 0x1B7 : VdGenerateGPUCSCCoefficients(
// 0x1B8 : VdGetClosedCaptionReadyStatus(
// 0x1B9 : VdGetCurrentDisplayGamma(
// 0x1BB : _VdGetDisplayModeOverride__YAKPAK0PAM0PAU_VD_DISPLAY_MODE_OVERRIDE_INFO___Z(
// 0x1BC : VdGetGraphicsAsicID(
// 0x1BD : VdGetSystemCommandBuffer(
// 0x1BE : VdGlobalDevice(
// 0x1BF : VdGlobalXamDevice(
// 0x1C0 : VdGpuClockInMHz(
// 0x1C1 : VdHSIOCalibrationLock(
// 0x1C2 : VdInitializeEngines(
// 0x1C3 : VdInitializeRingBuffer(
// 0x1C4 : VdInitializeScaler(
// 0x1C5 : VdInitializeScalerCommandBuffer(
// 0x1C6 : VdIsHSIOTrainingSucceeded(
// 0x1C7 : VdPersistDisplay(
// 0x1C8 : VdQuerySystemCommandBuffer(
// 0x1C9 : VdQueryVideoFlags(
// 0x1CA : VdQueryVideoMode(
// 0x1CB : VdReadDVERegisterUlong(
// 0x1CC : VdReadWriteHSIOCalibrationFlag(
// 0x1CD : VdRegisterGraphicsNotification(
// 0x1CE : VdRegisterXamGraphicsNotification(
// 0x1CF : VdSendClosedCaptionData(
// 0x1D0 : VdSetCGMSOption(
// 0x1D1 : VdSetColorProfileAdjustment(
// 0x1D2 : VdSetCscMatricesOverride(
// 0x1D3 : VdSetDisplayMode(
// 0x1D4 : VdSetDisplayModeOverride(
// 0x1D5 : VdSetGraphicsInterruptCallback(
// 0x1D6 : _VdSetHDCPOption__YAKK_Z(
// 0x1D7 : VdSetMacrovisionOption(
// 0x1D8 : VdSetSystemCommandBuffer(
// 0x1D9 : VdSetSystemCommandBufferGpuIdentifierAddress(
// 0x1DA : VdSetWSSData(
// 0x1DB : VdSetWSSOption(
// 0x1DC : VdShutdownEngines(
// 0x1DD : VdTurnDisplayOff(
// 0x1DE : VdTurnDisplayOn(
// 0x1E0 : VdWriteDVERegisterUlong(
// 0x25B : VdSwap(
// 0x268 : VdInitializeEDRAM(
// 0x269 : VdRetrainEDRAM(
// 0x26A : VdRetrainEDRAMWorker(
// 0x26B : VdHSIOTrainCount(
// 0x26D : VdHSIOTrainingStatus(
// 0x26F : VdReadEEDIDBlock(
// 0x270 : VdEnumerateVideoModes(
// 0x271 : VdEnableHDCP(
// 0x272 : VdRegisterHDCPNotification(
// 0x27E : VdGetDisplayDiscoveryData(
// 0x2A0 : VdStartDisplayDiscovery(
// 0x2A1 : BOOL VdSetHDCPRevocationList(PVOID pbSRM, DWORD cbSRM);
// 0x2AE : VdEnableWMAProOverHDMI(
// 0x2D2 : VdQueryRealVideoMode(
// 0x2D6 : VdSetCGMSState(
// 0x2D7 : VdSetSCMSState(
// 0x2D9 : VdGetOption(
// 0x2DA : VdSetOption(
// 0x365 : VdQueryVideoCapabilities(
// 0x367 : VdGet3dVideoFormat(
// 0x368 : VdGetWSS2Data(
// 0x369 : VdSet3dVideoFormat(
// 0x36A : VdSetWSS2Data(

// 0x2DB : UsbdBootEnumerationDoneEvent(
// 0x339 : UsbdEnableDisableRootHubPort(
// 0x2E4 : UsbdAddDeviceComplete(
// 0x2E5 : UsbdCancelAsyncTransfer(
// 0x2E6 : UsbdGetDeviceSpeed(
// 0x2E7 : UsbdGetDeviceTopology(
// 0x2E8 : UsbdGetEndpointDescriptor(
// 0x2E9 : UsbdIsDeviceAuthenticated(
// 0x2EA : UsbdOpenDefaultEndpoint(
// 0x2EB : UsbdOpenEndpoint(
// 0x2EC : UsbdQueueAsyncTransfer(
// 0x2ED : UsbdQueueCloseDefaultEndpoint(
// 0x2EE : UsbdQueueCloseEndpoint(
// 0x2EF : UsbdRemoveDeviceComplete(
// 0x2F1 : UsbdDriverLoadRequiredEvent-(
// 0x2F2 : UsbdGetRequiredDrivers(
// 0x2F3 : UsbdRegisterDriverObject(
// 0x2F4 : UsbdUnregisterDriverObject(
// 0x2F6 : UsbdResetDevice(
// 0x2F7 : UsbdGetDeviceDescriptor(
// 0x366 : UsbdGetDeviceRootPortType(
// 0x370 : UsbdGetRootHubDeviceNode(
// 0x371 : UsbdGetPortDeviceNode(
// 0x372 : UsbdGetNatalHub(
// 0x373 : UsbdGetNatalHardwareVersion(
// 0x374 : UsbdNatalHubRegisterNotificationCallback(


// 0x326 : XeCryptBnQwNeCompare(
// 0x310 : XeCryptSha256Init(
// 0x311 : XeCryptSha256Update(
// 0x312 : XeCryptSha256Final
// 0x313 : XeCryptSha256(
// 0x314 : XeCryptSha384Init(
// 0x315 : XeCryptSha384Update(
// 0x318 : XeCryptSha384Final(
// 0x31E : XeCryptSha384(
// 0x31F : XeCryptSha512Init(
// 0x323 : XeCryptSha512Update(
// 0x324 : XeCryptSha512Final(
// 0x325 : XeCryptSha512(
// 0x35F : XeCryptAesCtr(
// 0x360 : XeCryptAesCbcMac(
// 0x361 : XeCryptAesDmMac(

// 0x01F : NTSTATUS XeKeysGetConsoleCertificate(PXE_CONSOLE_CERTIFICATE pCertificate);  #Console identification
// 0x23A : XeKeysGetFactoryChallenge(
// 0x23B : XeKeysSetFactoryResponse(
// 0x23C : XeKeysInitializeFuses(
// 0x23D : XeKeysSaveBootLoader(
// 0x23E : XeKeysSaveKeyVault(
// 0x23F : void XeKeysGetStatus(PDWORD pStatus);
// 0x240 : NTSTATUS XeKeysGeneratePrivateKey(int dwKeyNum, OUT PVOID pPublicKey);
// 0x243 : XeKeysGenerateRandomKey(
// 0x245 : XeKeysGetDigest(
// 0x246 : NTSTATUS XeKeysGetConsoleID(PVOID pConsoleID);
// 0x248 : XeKeysQwNeRsaPrvCrypt(
// 0x249 : VOID XeKeysHmacSha(DWORD dwKeyIdx, const BYTE* pbInp1, DWORD cbInp1, const BYTE* pbInp2, DWORD cbInp2, const BYTE* pbInp3, DWORD cbInp3, BYTE* pbOut, DWORD cbOut);
// 0x24B : XeKeysAesCbc(
// 0x24C : XeKeysDes2Cbc(
// 0x24D : XeKeysDesCbc(
// 0x24E : XeKeysObscureKey(
// 0x24F : XeKeysHmacShaUsingKey(
// 0x250 : XeKeysSaveBootLoaderEx(
// 0x251 : XeKeysAesCbcUsingKey(
// 0x252 : XeKeysDes2CbcUsingKey(
// 0x253 : XeKeysDesCbcUsingKey(
// 0x256 : XeKeysConsolePrivateKeySign(
// 0x257 : XeKeysConsoleSignatureVerification(
// 0x258 : BOOL XeKeysVerifyRSASignature(int dwKeyIdx, PVOID pHash, PVOID pSignature);
// 0x25D : XeKeysSaveSystemUpdate(
// 0x25E : XeKeysLockSystemUpdate(
// 0x25F : DWORD XeKeysExecute(PBYTE pbChalData, DWORD cbChalSize, PHYSICAL_ADDRESS pbBlobParamR4, PHYSICAL_ADDRESS pbBlobParamR5, PHYSICAL_ADDRESS pbBlobParamR6, PHYSICAL_ADDRESS pbBlobParamR7);
// 0x260 : XeKeysGetVersions(
// 0x27C : XeKeysSetRevocationList(
// 0x28C : XeKeysExSaveKeyVault(
// 0x28D : XeKeysExSetKey(
// 0x28E : XeKeysExGetKey(
// 0x292 : XeKeysSecurityInitialize(
// 0x293 : XeKeysSecurityLoadSettings(
// 0x294 : XeKeysSecuritySaveSettings(
// 0x295 : XeKeysSecuritySetDetected(
// 0x296 : XeKeysSecurityGetDetected(
// 0x297 : XeKeysSecuritySetActivated(
// 0x298 : XeKeysSecurityGetActivated(
// 0x299 : XeKeysReserved665() (formerly XeKeysDvdAuthAP25InstallTable)(
// 0x29A : XeKeysReserved666() (formerly XeKeysDvdAuthAP25GetTableVersion)(
// 0x29B : XeKeysGetProtectedFlag(
// 0x29C : XeKeysSetProtectedFlag(
// 0x2A2 : XeKeysGetUpdateSequence(
// 0x2A3 : XeKeysDvdAuthAP25GetChallengeHistory(
// 0x2AF : XeKeysRevokeSaveSettings(
// 0x2B2 : XeKeysGetMediaID(
// 0x2B3 : XeKeysReserved691 (formerly XeKeysLoadKeyVault)(
// 0x2CE : XeKeysRevokeUpdateDynamic(
// 0x2D1 : XeKeysSecuritySetStat(
// 0x31B : XeKeysFcrtLoad(
// 0x31C : XeKeysFcrtSave(
// 0x31D : XeKeysFcrtSet(
// 0x32B : XeKeysRevokeIsDeviceRevoked(
// 0x33D : XeKeysDvdAuthExSave(
// 0x33E : XeKeysDvdAuthExInstall(
// 0x346 : XeKeysObfuscateEx(
// 0x347 : XeKeysUnObfuscateEx(
// 0x35E : XeKeysVerifyPIRSSignature(
// 0x363 : XeKeysGetTruncatedSecondaryConsoleId(
}

#endif