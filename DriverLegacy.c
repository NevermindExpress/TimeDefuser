///
/// TimeDefuser For Legacy Systems
/// 
/// It's written in C89 (ugh) so it can be compiled on at least XP DDK and VS 2010 compiler.
/// 
/// It also targets Windows XP and earlier versions with support for x86 and x64.
/// For Windows Vista and later, use the non-legacy version.
///

#include <ntddk.h>

NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

#define SystemModuleInformation 11

extern NTSTATUS NTAPI ZwQuerySystemInformation(
	int SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength OPTIONAL
);

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

NTSTATUS DriverEntry (PDRIVER_OBJECT  DriverObject, PUNICODE_STRING UniRegistryPath) {
	// FUCK C89 AND ANCIENT MSVC WITH NO C99 ON C MODE
	// ITS HORRIBLE I HATE THIS
	// Anyway, due to muh c89, here are all variables.
	NTSTATUS status = STATUS_SUCCESS; // NTSTATUS variable to record success or failure
	unsigned __int64 TimebombStamp; // Timebomb date stamp for saving
	RTL_PROCESS_MODULES ModuleInfo; // For getting kernel base address.
	unsigned __int64* KernelBase; // Kernel base address.
	ULONG KernelSize; // Kernel image size.
	void* pExpNtExpirationDate; // Address of expiration date stored internally in kernel, asides of kuser.
	size_t i; // Counter for "for" loops.
	
		// Address of SystemExpirationDate field at KUSER_SHARED_DATA
#if defined(AMD64)
	LARGE_INTEGER* li = (LARGE_INTEGER*)0xfffff780000002c8; 
#elif defined(i386)
	LARGE_INTEGER* li = (LARGE_INTEGER*)0xffdf02c8;
#else
#error Unsupported architecture.
#endif

    UNREFERENCED_PARAMETER (UniRegistryPath);
	UNREFERENCED_PARAMETER (DriverObject);
	
	// 
	// Finally start of code.
	//

	// Load and change SystemExpirationDate
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[*] TimeDefuser: version 1.3 (legacy) loaded. | Compiled on "__DATE__" "__TIME__" | https://github.com/NevermindExpress/TimeDefuser\n"));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[*] TimeDefuser: currently SystemExpirationDate is: %llu\n", li->QuadPart));
	TimebombStamp = li->QuadPart; // We will need it later so we save it.
	li->QuadPart = 0; // And then set the KUSER field to 0.
	
	if (!TimebombStamp) { // No timebomb.
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: No timebomb found, exiting.")); 
		return STATUS_FAILED_DRIVER_ENTRY;
	}
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: SystemExpirationDate is updated and it is now: %llu\n", li->QuadPart));
	
	// Get kernel base
	status = ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	KernelBase = ModuleInfo.Modules[0].ImageBase;
	KernelSize = ModuleInfo.Modules[0].ImageSize;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize));
	
	// Search for timebomb stamp in memory
	KernelSize /= sizeof(unsigned __int64); 
	for (i = 0; i < KernelSize; i++) {
		if (KernelBase[i] == TimebombStamp) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: ExpNtExpirationDate found at 0x%p\n", &KernelBase[i]));
			KernelBase[i] = 0; 
			// For some reason actual timebomb was the next qword on XP 2526, I'll save this and search for it again.
			TimebombStamp = KernelBase[i+1]; // Save the lower part of stamp.
			KernelBase[i+1] = 0; // And null where I found it too.
			break;
		}
	} 
	
	// Search for the second stamp, ExpNtExpirationData (and not Date)
	for (i = 0; i < KernelSize; i++) {
		if ((int)KernelBase[i] == (int)TimebombStamp) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: ExpNtExpirationData found at 0x%p\n", &KernelBase[i]));
			RtlZeroMemory(&KernelBase[i],16);
			goto patchOK; 
		}
	} 

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: Patch failed.\n"));
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: Patch completed successfully.\n"));
    return STATUS_SUCCESS;
}