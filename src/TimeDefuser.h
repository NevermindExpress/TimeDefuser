/// General definitions for TimeDefuser

/// Includes
#include "tdwdm/wdm.h"

/// Definitions
#define td_version "1.9"

#define SystemModuleInformation 11
#define IMAGE_DOS_SIGNATURE 0x5a4d // MZ
#define IMAGE_NT_SIGNATURE  0x00004550  // PE00

#if defined(AMD64)
	#define KUSERSystemExpirationDate (LARGE_INTEGER*)0xfffff780000002c8; // c802000080f7ffff
	typedef unsigned __int64 ptr_t;
#elif defined(i386)
	#define KUSERSystemExpirationDate (LARGE_INTEGER*)0xffdf02c8;
	typedef unsigned long ptr_t;
#else
	#error Unsupported architecture.
#endif

/// Section Names
static const unsigned char Sections[][8] = {
	{ 'P','A','G','E', 0,0,0,0 },
	{ 'P','A','G','E','L','K',0,0 },
	{ 'P','A','G','E','K','D',0,0 },
	{ 'P','A','G','E','D','A','T','A' },
	{ 'P','A','G','E','V','R','F','Y' },
	{ 'P','A','G','E','V','R','F','B' },
	{ 'P','A','G','E','B','G','F','X' },
	{ 'P','A','G','E','H','D','L','S' },
	{ '.','t','e','x','t',0,0,0 },
	{ 'I','N','I','T',0,0,0,0 },
};
/// TimeDefuser Structures
typedef struct {
	unsigned long RVA;
	unsigned long size;
} PAGESections;

/// Windows NT Structures
#pragma pack(push, 1)
typedef struct _IMAGE_DOS_HEADER {
	WORD e_magic;
	char  unused[58];
	LONG e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
	WORD    Machine;
	WORD    NumberOfSections;
	DWORD   TimeDateStamp;
	DWORD   PointerToSymbolTable;
	DWORD   NumberOfSymbols;
	WORD    SizeOfOptionalHeader;
	WORD    Characteristics;
} IMAGE_FILE_HEADER;

typedef struct _IMAGE_SECTION_HEADER {
	BYTE    Name[8];
	union {
		DWORD   PhysicalAddress;
		DWORD   VirtualSize;
	} Misc;
	DWORD   VirtualAddress;
	DWORD   SizeOfRawData;
	DWORD   PointerToRawData;
	DWORD   PointerToRelocations;
	DWORD   PointerToLinenumbers;
	WORD    NumberOfRelocations;
	WORD    NumberOfLinenumbers;
	DWORD   Characteristics;
} IMAGE_SECTION_HEADER;
#pragma pack(pop)

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
	PVOID Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

/// Windows NT Functions
extern __kernel_entry NTSTATUS NTAPI ZwQuerySystemInformation(
	int SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength OPTIONAL
);	

/// TimeDefuser Registry Functions
HANDLE OpenRegistryKey(PUNICODE_STRING KeyPath);
NTSTATUS RegWriteDword(HANDLE hKey, PCWSTR ValueName, ULONG Data);
ULONG RegReadValue(_In_ HANDLE KeyHandle, _In_ PCWSTR ValueName, _Out_ PVOID ValueOutput, _In_ ULONG ValueOutputSz);
NTSTATUS SaveKernelVersion(_In_ HANDLE hKey);
BOOLEAN CompareKernelVersion(_In_ HANDLE hKey);

/// TimeDefuser macros
#define TDPrint(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, ##__VA_ARGS__);