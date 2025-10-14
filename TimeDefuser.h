/// General definitions for TimeDefuser

/// Includes (only one)
#include <ntddk.h>

/// Definitions
#define td_version "1.5.1"
#define SystemModuleInformation 11
#define PEheader 0x5a4d // MZ
#define sectNamePAGEDATA	0x4154414445474150 // PAGEDATA
#define sectNamePAGE		0x0000000045474150 // "PAGE\0\0\0\0"

#if defined(AMD64)
	#define KUSERSystemExpirationDate (LARGE_INTEGER*)0xfffff780000002c8;
	typedef unsigned __int64 ptr_t;
#elif defined(i386)
	#define KUSERSystemExpirationDate (LARGE_INTEGER*)0xffdf02c8;
	typedef unsigned long ptr_t;
#else
	#error Unsupported architecture.
#endif

/// TimeDefuser Structures
typedef struct {
	unsigned long RVA;
	unsigned long size;
} PAGESections;

/// Windows NT Structures
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

/// TimeDefuser macros
#define TDPrint(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, ##__VA_ARGS__);