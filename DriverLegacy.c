///
/// TimeDefuser For Legacy Systems
/// 
/// It's written in C89 (ugh) so it can be compiled on at least XP DDK and VS 2010 compiler.
/// 
/// It also targets Windows XP and earlier versions with support for x86 and x64.
/// For Windows Vista and later, use the non-legacy version.
///

#include <ntddk.h>
#if NTDDI_VERSION < 0x06000000
#define TD_LEGACY
#include "TimeDefuser.h"

NTSTATUS DriverEntry (PDRIVER_OBJECT  DriverObject, PUNICODE_STRING UniRegistryPath); // Required for the pragma below.

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS DriverEntry (PDRIVER_OBJECT  DriverObject, PUNICODE_STRING UniRegistryPath) {
	NTSTATUS status = STATUS_SUCCESS; // NTSTATUS variable to record success or failure
	unsigned __int64 TimebombStamp; // Timebomb date stamp for saving
	RTL_PROCESS_MODULES ModuleInfo; // For getting kernel base address.
	unsigned __int64* KernelBase; // Kernel base address.
	ULONG KernelSize; // Kernel image size.
	void* pExpNtExpirationDate; // Address of expiration date stored internally in kernel, asides of kuser.
	size_t i; // Counter for "for" loops.
	LARGE_INTEGER* li = KUSERSystemExpirationDate	// Address of SystemExpirationDate field at KUSER_SHARED_DATA

    UNREFERENCED_PARAMETER (UniRegistryPath);
	UNREFERENCED_PARAMETER (DriverObject);
	


	// Print version info.
	TDPrint("[*] TimeDefuser: version " td_version " (legacy) loaded "
			"| Compiled on " __DATE__ " " __TIME__ " "
			"| https://github.com/NevermindExpress/TimeDefuser\n");

	// Get SystemExpirationDate
	TimebombStamp = li->QuadPart;
	if (!TimebombStamp) {
		TDPrint("[X] TimeDefuser: No timebomb found, exiting.\n");
		return STATUS_FAILED_DRIVER_ENTRY;
	}
	TDPrint("[+] TimeDefuser: SystemExpirationDate is 0x%x\n", TimebombStamp);
	
	// Get kernel base
	ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	if (ModuleInfo.NumberOfModules == 0) {
		TDPrint("[X] TimeDefuser: Failed to get kernel base address.\n");
		goto patchFail;
	}
	KernelBase = ModuleInfo.Modules[0].ImageBase;
	KernelSize = ModuleInfo.Modules[0].ImageSize;
	TDPrint("[+] TimeDefuser: Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize);
	
	// Search for timebomb stamp in memory
	KernelSize /= sizeof(unsigned __int64); 
	for (i = 0; i < KernelSize; i++) {
		if (KernelBase[i] == TimebombStamp) {
			TDPrint("[+] TimeDefuser: ExpNtExpirationDate found at 0x%p\n", &KernelBase[i]);
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
			TDPrint("[+] TimeDefuser: ExpNtExpirationData found at 0x%p\n", &KernelBase[i]);
			RtlZeroMemory(&KernelBase[i],16);
			goto patchOK; 
		}
	} 

patchFail:
	TDPrint("[X] TimeDefuser: Patch failed.\n");
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	TDPrint("[X] TimeDefuser: Patch completed successfully.\n");
    return STATUS_SUCCESS;
}
#endif