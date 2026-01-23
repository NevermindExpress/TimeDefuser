// Lightweight wdm.h polyfill for TimeDefuser
// This file is for removing dependency on having WDK for compiling.
#pragma once
#ifdef _M_IX86
    #define i386
    #define _X86_
#elif defined(_M_AMD64)
    #define AMD64
    #define _AMD64_
#endif

#include <ntdef.h>
#include <ntstatus.h>
#include <bugcodes.h>
#include <winapifamily.h>
#include <ntiologc.h>
#include <excpt.h>
#include <dpfilter.h>

#define PDRIVER_OBEJCT void* // Unused by TimeDefuser

#define NTKERNELAPI __declspec(dllimport)


// Definitions
#define TIMER_TOLERABLE_DELAY_BITS      6
#define TIMER_EXPIRED_INDEX_BITS        6
#define TIMER_PROCESSOR_INDEX_BITS      5

#define PAGE_NOACCESS           0x01    
#define PAGE_READONLY           0x02    
#define PAGE_READWRITE          0x04    
#define PAGE_WRITECOPY          0x08    

#define PAGE_EXECUTE            0x10    
#define PAGE_EXECUTE_READ       0x20    
#define PAGE_EXECUTE_READWRITE  0x40    
#define PAGE_EXECUTE_WRITECOPY  0x80    

#define PAGE_GUARD             0x100    
#define PAGE_NOCACHE           0x200    
#define PAGE_WRITECOMBINE      0x400 

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)


#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))


#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))

#define REG_OPTION_RESERVED         (0x00000000L)   // Parameter is reserved

#define REG_OPTION_NON_VOLATILE     (0x00000000L)   // Key is preserved
                                                    // when system is rebooted

#define REG_OPTION_VOLATILE         (0x00000001L)   // Key is not preserved
                                                    // when system is rebooted

#define REG_OPTION_CREATE_LINK      (0x00000002L)   // Created key is a
                                                    // symbolic link

#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)   // open for backup or restore
                                                    // special access rules
                                                    // privilege required

#define REG_OPTION_OPEN_LINK        (0x00000008L)   // Open symbolic link

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_OPEN_LEGAL_OPTION       \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_BINARY                  ( 3ul ) // Free form binary
#define REG_DWORD                   ( 4ul ) // 32-bit number

#define far
#define near

typedef CCHAR KPROCESSOR_MODE;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT* PFLOAT;
typedef BOOL near* PBOOL;
typedef BOOL far* LPBOOL;
typedef BYTE near* PBYTE;
typedef BYTE far* LPBYTE;
typedef int near* PINT;
typedef int far* LPINT;
typedef WORD near* PWORD;
typedef WORD far* LPWORD;
typedef long far* LPLONG;
typedef DWORD near* PDWORD;
typedef DWORD far* LPDWORD;
typedef void far* LPVOID;
typedef CONST void far* LPCVOID;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int*       PUINT;


typedef enum _MODE {
	KernelMode,
	UserMode,
	MaximumMode
} MODE;

typedef enum _LOCK_OPERATION {
	IoReadAccess,
	IoWriteAccess,
	IoModifyAccess
} LOCK_OPERATION;

typedef enum _IO_ALLOCATION_ACTION {
    KeepObject = 1,
    DeallocateObject,
    DeallocateObjectKeepRegisters
} IO_ALLOCATION_ACTION, * PIO_ALLOCATION_ACTION;

typedef enum _MEMORY_CACHING_TYPE_ORIG {
    MmFrameBufferCached = 2
} MEMORY_CACHING_TYPE_ORIG;

typedef enum _MEMORY_CACHING_TYPE {
    MmNonCached = FALSE,
    MmCached = TRUE,
    MmWriteCombined = MmFrameBufferCached,
    MmHardwareCoherentCached,
    MmNonCachedUnordered,       // IA64
    MmUSWCCached,
    MmMaximumCacheType,
    MmNotMapped = -1
} MEMORY_CACHING_TYPE;

typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;

typedef _Enum_is_bitflag_ enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType,
    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,
    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,

} _Enum_is_bitflag_ POOL_TYPE;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
    MaxKeyValueInfoClass  // MaxKeyValueInfoClass should always be the last enum
} KEY_VALUE_INFORMATION_CLASS;

#define DEVICE_TYPE ULONG

typedef ULONG_PTR KSPIN_LOCK;
typedef KSPIN_LOCK* PKSPIN_LOCK;
typedef PVOID PSECURITY_DESCRIPTOR;

typedef ULONG ACCESS_MASK;
typedef ACCESS_MASK* PACCESS_MASK;


//
// Define types that are not exported.
//

typedef struct _ACCESS_STATE* PACCESS_STATE;
typedef struct _CALLBACK_OBJECT* PCALLBACK_OBJECT;
#if defined(_NTHAL_INCLUDED_)
typedef struct _KPROCESS* PEPROCESS;
typedef struct _ETHREAD* PETHREAD;
#elif defined(_NTIFS_INCLUDED_)
typedef struct _KPROCESS* PEPROCESS;
typedef struct _KTHREAD* PETHREAD;
#else
typedef struct _EPROCESS* PEPROCESS;
typedef struct _ETHREAD* PETHREAD;
#endif
typedef struct _IO_TIMER* PIO_TIMER;
typedef struct _KINTERRUPT* PKINTERRUPT;
typedef struct _KTHREAD* PKTHREAD, * PRKTHREAD;
typedef struct _KPROCESS* PKPROCESS, * PRKPROCESS;
typedef struct _OBJECT_TYPE* POBJECT_TYPE;
typedef struct _SECURITY_QUALITY_OF_SERVICE* PSECURITY_QUALITY_OF_SERVICE;

typedef VOID DRIVER_CANCEL(
    _Inout_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP* Irp
);
typedef DRIVER_CANCEL* PDRIVER_CANCEL;

typedef _Function_class_(DRIVER_CONTROL) _IRQL_requires_same_ IO_ALLOCATION_ACTION DRIVER_CONTROL(
    _In_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ struct _IRP* Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context
);
typedef DRIVER_CONTROL* PDRIVER_CONTROL;

typedef VOID KDEFERRED_ROUTINE(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);
typedef KDEFERRED_ROUTINE* PKDEFERRED_ROUTINE;


// Structures
typedef struct _MDL {
	struct _MDL* Next;
	CSHORT           Size;
	CSHORT           MdlFlags;
	struct _EPROCESS* Process;
	PVOID            MappedSystemVa;
	PVOID            StartVa;
	ULONG            ByteCount;
	ULONG            ByteOffset;
} MDL, *PMDL;

typedef struct _DISPATCHER_HEADER {
    union {
        union {
            volatile LONG Lock;
            LONG LockNV;
        } DUMMYUNIONNAME;

        struct {
            UCHAR Type;
            UCHAR Signalling;
            UCHAR Size;
            UCHAR Reserved1;
        } DUMMYSTRUCTNAME;

        struct { 
            UCHAR TimerType;
            union {
                UCHAR TimerControlFlags;
                struct {
                    UCHAR Absolute : 1;
                    UCHAR Wake : 1;
                    UCHAR EncodedTolerableDelay : TIMER_TOLERABLE_DELAY_BITS;
                } DUMMYSTRUCTNAME;
            };

            UCHAR Hand;
            union {
                UCHAR TimerMiscFlags;
                struct {

#if !defined(KENCODED_TIMER_PROCESSOR)

                    UCHAR Index : TIMER_EXPIRED_INDEX_BITS;

#else

                    UCHAR Index : 1;
                    UCHAR Processor : TIMER_PROCESSOR_INDEX_BITS;

#endif

                    UCHAR Inserted : 1;
                    volatile UCHAR Expired : 1;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;
        } DUMMYSTRUCTNAME2;

        struct {                            // Timer2
            UCHAR Timer2Type;
            union {
                UCHAR Timer2Flags;
                struct {
                    UCHAR Timer2Inserted : 1;
                    UCHAR Timer2Expiring : 1;
                    UCHAR Timer2CancelPending : 1;
                    UCHAR Timer2SetPending : 1;
                    UCHAR Timer2Running : 1;
                    UCHAR Timer2Disabled : 1;
                    UCHAR Timer2ReservedFlags : 2;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            UCHAR Timer2Reserved1;
            UCHAR Timer2Reserved2;
        } DUMMYSTRUCTNAME3;

        struct {                            // Queue
            UCHAR QueueType;
            union {
                UCHAR QueueControlFlags;
                struct {
                    UCHAR Abandoned : 1;
                    UCHAR DisableIncrement : 1;
                    UCHAR QueueReservedControlFlags : 6;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            UCHAR QueueSize;
            UCHAR QueueReserved;
        } DUMMYSTRUCTNAME4;

        struct {                            // Thread
            UCHAR ThreadType;
            UCHAR ThreadReserved;
            union {
                UCHAR ThreadControlFlags;
                struct {
                    UCHAR CycleProfiling : 1;
                    UCHAR CounterProfiling : 1;
                    UCHAR GroupScheduling : 1;
                    UCHAR AffinitySet : 1;
                    UCHAR Tagged : 1;
                    UCHAR EnergyProfiling : 1;

#if !defined(_X86_)

                    UCHAR ThreadReservedControlFlags : 2;

#else

                    UCHAR Instrumented : 1;
                    UCHAR ThreadReservedControlFlags : 1;

#endif

                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            union {
                UCHAR DebugActive;

#if !defined(_X86_)

                struct {
                    BOOLEAN ActiveDR7 : 1;
                    BOOLEAN Instrumented : 1;
                    BOOLEAN Minimal : 1;
                    BOOLEAN Reserved4 : 3;
                    BOOLEAN UmsScheduled : 1;
                    BOOLEAN UmsPrimary : 1;
                } DUMMYSTRUCTNAME;

#endif

            } DUMMYUNIONNAME2;
        } DUMMYSTRUCTNAME5;

        struct {                         // Mutant
            UCHAR MutantType;
            UCHAR MutantSize;
            BOOLEAN DpcActive;
            UCHAR MutantReserved;
        } DUMMYSTRUCTNAME6;
    } DUMMYUNIONNAME;

    LONG SignalState;                   // Object lock
    LIST_ENTRY WaitListHead;            // Object lock
} DISPATCHER_HEADER, * PDISPATCHER_HEADER;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;
typedef VOID(NTAPI* PIO_APC_ROUTINE) (
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    );
#define PIO_APC_ROUTINE_DEFINED

typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, * PKEVENT, * PRKEVENT;

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, * PKDEVICE_QUEUE_ENTRY, * PRKDEVICE_QUEUE_ENTRY;

typedef struct _VPB {
    CSHORT Type;
    CSHORT Size;
    USHORT Flags;
    USHORT VolumeLabelLength; // in bytes
    struct _DEVICE_OBJECT* DeviceObject;
    struct _DEVICE_OBJECT* RealDevice;
    ULONG SerialNumber;
    ULONG ReferenceCount;
    WCHAR VolumeLabel[32];
} VPB, * PVPB;

typedef struct _KDPC {
    union {
        ULONG TargetInfoAsUlong;
        struct {
            UCHAR Type;
            UCHAR Importance;
            volatile USHORT Number;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    SINGLE_LIST_ENTRY DpcListEntry;
    KAFFINITY ProcessorHistory;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    __volatile PVOID DpcData;
} KDPC, * PKDPC, * PRKDPC;

typedef struct _WAIT_CONTEXT_BLOCK {
    union {
        KDEVICE_QUEUE_ENTRY WaitQueueEntry;
        struct {
            LIST_ENTRY DmaWaitEntry;
            ULONG NumberOfChannels;
            ULONG SyncCallback : 1;
            ULONG DmaContext : 1;
            ULONG ZeroMapRegisters : 1;
            ULONG Reserved : 29;
        };
    };
    PDRIVER_CONTROL DeviceRoutine;
    PVOID DeviceContext;
    ULONG NumberOfMapRegisters;
    PVOID DeviceObject;
    PVOID CurrentIrp;
    PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, * PWAIT_CONTEXT_BLOCK;

typedef struct _KDEVICE_QUEUE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    KSPIN_LOCK Lock;

#if defined(_AMD64_)

    union {
        BOOLEAN Busy;
        struct {
            LONG64 Reserved : 8;
            LONG64 Hint : 56;
        };
    };

#else

    BOOLEAN Busy;

#endif

} KDEVICE_QUEUE, * PKDEVICE_QUEUE, * PRKDEVICE_QUEUE;

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    struct _DRIVER_OBJECT* DriverObject;
    struct _DEVICE_OBJECT* NextDevice;
    struct _DEVICE_OBJECT* AttachedDevice;
    struct _IRP* CurrentIrp;
    PIO_TIMER Timer;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
    __volatile PVPB Vpb;
    PVOID DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    union {
        LIST_ENTRY ListEntry;
        WAIT_CONTEXT_BLOCK Wcb;
    } Queue;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    KEVENT DeviceLock;

    USHORT SectorSize;
    USHORT Spare1;

    struct _DEVOBJ_EXTENSION* DeviceObjectExtension;
    PVOID  Reserved;

} DEVICE_OBJECT;
typedef struct _DEVICE_OBJECT* PDEVICE_OBJECT;

typedef struct _SECTION_OBJECT_POINTERS {
    PVOID DataSectionObject;
    PVOID SharedCacheMap;
    PVOID ImageSectionObject;
} SECTION_OBJECT_POINTERS;
typedef SECTION_OBJECT_POINTERS* PSECTION_OBJECT_POINTERS;

typedef struct _IO_COMPLETION_CONTEXT {
    PVOID Port;
    PVOID Key;
} IO_COMPLETION_CONTEXT, * PIO_COMPLETION_CONTEXT;

typedef struct _FILE_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
    PVPB Vpb;
    PVOID FsContext;
    PVOID FsContext2;
    PSECTION_OBJECT_POINTERS SectionObjectPointer;
    PVOID PrivateCacheMap;
    NTSTATUS FinalStatus;
    struct _FILE_OBJECT* RelatedFileObject;
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
    __volatile ULONG Waiters;
    __volatile ULONG Busy;
    PVOID LastLock;
    KEVENT Lock;
    KEVENT Event;
    __volatile PIO_COMPLETION_CONTEXT CompletionContext;
    KSPIN_LOCK IrpListLock;
    LIST_ENTRY IrpList;
    __volatile PVOID FileObjectExtension;
} FILE_OBJECT;
typedef struct _FILE_OBJECT* PFILE_OBJECT;

typedef struct _KAPC {
    UCHAR Type;
    UCHAR SpareByte0;
    UCHAR Size;
    UCHAR SpareByte1;
    ULONG SpareLong0;
    struct _KTHREAD* Thread;
    LIST_ENTRY ApcListEntry;
    PVOID Reserved[3];
    PVOID NormalContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    CCHAR ApcStateIndex;
    KPROCESSOR_MODE ApcMode;
    BOOLEAN Inserted;
} KAPC, * PKAPC, * PRKAPC;

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _IRP {
    CSHORT Type;
    USHORT Size;
    PMDL MdlAddress;
    ULONG Flags;
    union {
        struct _IRP* MasterIrp;
        __volatile LONG IrpCount;
        PVOID SystemBuffer;
    } AssociatedIrp;
    LIST_ENTRY ThreadListEntry;
    IO_STATUS_BLOCK IoStatus;
    KPROCESSOR_MODE RequestorMode;
    BOOLEAN PendingReturned;
    CHAR StackCount;
    CHAR CurrentLocation;
    BOOLEAN Cancel;
    KIRQL CancelIrql;
    CCHAR ApcEnvironment;
    UCHAR AllocationFlags;
    PIO_STATUS_BLOCK UserIosb;
    PKEVENT UserEvent;
    union {
        struct {
            union {
                PIO_APC_ROUTINE UserApcRoutine;
                PVOID IssuingProcess;
            };
            PVOID UserApcContext;
        } AsynchronousParameters;
        LARGE_INTEGER AllocationSize;
    } Overlay;
    __volatile PDRIVER_CANCEL CancelRoutine;
    PVOID UserBuffer;
    union {
        struct {
            union {
                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
                struct {
                    PVOID DriverContext[4];
                };
            };
            PETHREAD Thread;
            PCHAR AuxiliaryBuffer;
            struct {
                LIST_ENTRY ListEntry;

                union {
                    struct _IO_STACK_LOCATION* CurrentStackLocation;
                    ULONG PacketType;
                };
            };
            PFILE_OBJECT OriginalFileObject;
        } Overlay;
        KAPC Apc;
        PVOID CompletionKey;
    } Tail;
} IRP;
typedef IRP* PIRP;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1]; // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, * PKEY_VALUE_PARTIAL_INFORMATION;

// Functions

/// Debug print
NTSYSAPI ULONG NTAPI DbgPrintEx(
	_In_ ULONG ComponentId,
	_In_ ULONG Level,
	_In_ PCSTR Format,
	...
);

/// Memory management
NTKERNELAPI PMDL NTAPI IoAllocateMdl(
	_In_opt_ __drv_aliasesMem PVOID VirtualAddress,
	_In_ ULONG Length,
	_In_ BOOLEAN SecondaryBuffer,
	_In_ BOOLEAN ChargeQuota,
	_Inout_opt_ PIRP Irp
);

VOID NTAPI MmProbeAndLockPages(
	_Inout_   PMDL            MemoryDescriptorList,
	_In_      KPROCESSOR_MODE AccessMode,
	_In_      LOCK_OPERATION  Operation
);

NTKERNELAPI PVOID NTAPI MmMapLockedPages(
    _Inout_ PMDL MemoryDescriptorList,
    _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst)
    KPROCESSOR_MODE AccessMode
);

NTKERNELAPI PVOID NTAPI MmMapLockedPagesSpecifyCache(
    _Inout_ PMDL MemoryDescriptorList,
    _In_ __drv_strictType(KPROCESSOR_MODE / enum _MODE, __drv_typeConst)
    KPROCESSOR_MODE AccessMode,
    _In_ __drv_strictTypeMatch(__drv_typeCond) MEMORY_CACHING_TYPE CacheType,
    _In_opt_ PVOID RequestedAddress,
    _In_     ULONG BugCheckOnFailure,
    _In_     ULONG Priority  // MM_PAGE_PRIORITY logically OR'd with MdlMapping*
);

NTKERNELAPI NTSTATUS NTAPI MmProtectMdlSystemAddress(
    _In_ PMDL MemoryDescriptorList,
    _In_ ULONG NewProtect
);

NTKERNELAPI VOID NTAPI MmUnmapLockedPages(
    _In_ PVOID BaseAddress,
    _Inout_ PMDL MemoryDescriptorList
);

NTKERNELAPI VOID NTAPI MmUnlockPages(
    _Inout_ PMDL MemoryDescriptorList
);

NTKERNELAPI VOID NTAPI IoFreeMdl(
    PMDL Mdl
);

BOOLEAN NTAPI MmIsAddressValid(
    _In_ PVOID VirtualAddress
);

NTKERNELAPI PVOID NTAPI ExAllocatePoolWithTag(
    _In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
);

NTKERNELAPI VOID __stdcall ExFreePoolWithTag(
    _Pre_notnull_ __drv_freesMem(Mem) PVOID P,
    _In_ ULONG Tag
);

/// Registry
NTSYSAPI NTSTATUS NTAPI ZwCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
);

NTSYSAPI NTSTATUS NTAPI ZwSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_opt_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_reads_bytes_opt_(DataSize) PVOID Data,
    _In_ ULONG DataSize
);

NTSYSAPI NTSTATUS NTAPI ZwQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
);

/// Executive general
NTSYSAPI NTSTATUS NTAPI ZwClose(
    _In_ HANDLE Handle
);

/// Runtime
NTSYSAPI VOID NTAPI RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString
);
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))

/// ???
NTKERNELAPI BOOLEAN NTAPI PsGetVersion(
    _Out_opt_ PULONG MajorVersion,
    _Out_opt_ PULONG MinorVersion,
    _Out_opt_ PULONG BuildNumber,
    _Out_opt_ PUNICODE_STRING CSDVersion
);