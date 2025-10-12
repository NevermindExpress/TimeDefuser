#include <ntddk.h>
#if NTDDI_VERSION >= 0x06000000
//#include <wdf.h>

#define SystemModuleInformation 11

extern "C" __kernel_entry NTSTATUS NTAPI ZwQuerySystemInformation(
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
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct {
	unsigned long RVA;
	unsigned long size;
} PAGESections;

extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	// NTSTATUS variable to record success or failure
	NTSTATUS status = STATUS_SUCCESS;

	// Address of SystemExpirationDate field at KUSER_SHARED_DATA
#if defined(AMD64)
	LARGE_INTEGER* li = (LARGE_INTEGER*)0xfffff780000002c8;
#elif defined(i386)
	LARGE_INTEGER* li = (LARGE_INTEGER*)0xffdf02c8;
#else
#error Unsupported architecture.
#endif

	// Print version info.
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[*] TimeDefuser: version 1.4.1 loaded "
														"| Compiled on " __DATE__ " " __TIME__ " | https://github.com/NevermindExpress/TimeDefuser\n"));

	// Get SystemExpirationDate
	unsigned long long TimebombStamp = li->QuadPart;
	if (!TimebombStamp) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: No timebomb found, exiting.\n"));
		return STATUS_FAILED_DRIVER_ENTRY;
	}
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: SystemExpirationDate is 0x%p\n", TimebombStamp));

	// Get kernel base
	RTL_PROCESS_MODULES ModuleInfo = { 0 };
	status = ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	unsigned long long* KernelBase = (unsigned long long*)ModuleInfo.Modules[0].ImageBase;
#pragma warning(disable:4189)
	ULONG KernelSize = ModuleInfo.Modules[0].ImageSize; 
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize));

	// Check for PE Header existance.
	const short header = 0x5a4d; // MZ
	if (*(short*)KernelBase != header) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: PE Header not found!\n"));
		goto patchFail;
	}

	const __int64 sectNamePAGE = 0x4154414445474150; // "PAGEDATA"
	unsigned int KernelSize2 = 0;
	unsigned char* PotentialTimestamp = (unsigned char*)KernelBase;

	// Search for "PAGEDATA" section at PE sections. This section is where the 
	// ExpNtExpirationDate timestamp variable is located at, so we are going 
	// to use its RVA and size for finding the function location.

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectNamePAGE) { // Check if we found the PAGEDATA section name.
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: PAGEDATA Section found at 0x%p with size %d\n", &KernelBase[i], *(int*)&KernelBase[i + 1]));
			KernelSize2 = *(int*)&KernelBase[i + 1]; // Get the section size
			// Get the function RVA and append it to kernel base address.
			int* asd = (int*)&KernelBase[i + 1];
			PotentialTimestamp += asd[1];
			break;
		}
	}
	if (PotentialTimestamp == (unsigned char*)KernelBase) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: PAGEDATA Section not found!\n"));
		goto patchFail;
	}

	// Search for timebomb stamp in memory
	CHAR occurance = FALSE;
	void* pExpNtExpirationDate = NULL;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: searching for stamp at 0x%p in %d bytes\n", PotentialTimestamp, KernelSize2));

	KernelSize2;
	for (size_t i = 0; i < KernelSize2; i++) {
		if (*(unsigned long long*)&PotentialTimestamp[i] == TimebombStamp) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: Timebomb stamp found at 0x%p\n", &PotentialTimestamp[i]));
			*(unsigned long long*)(&PotentialTimestamp[i]) = 0;
			pExpNtExpirationDate = &PotentialTimestamp[i];

			if (occurance) {
				pExpNtExpirationDate = &PotentialTimestamp[i]; 
				occurance = 2;
				break;
			}
			else occurance = 1;
		}
	}

	// Print the address according to occurrance.
	switch (occurance) {
	case 0:
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: can't find ExpNtExpirationDate!\n"));
		goto patchFail; break;
	case 1:
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (first occurrance)\n", pExpNtExpirationDate));
		break;
	case 2:
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (second occurrance)\n", pExpNtExpirationDate));
		break;
	}

	const __int64 sectNamePAGELK = 0x0000000045474150; // "PAGE\0\0\0\0"

	// Search for PAGE section at PE sections. This section or one of the next three sections is where the 
	// "ExpTimeRefreshWork" function is located at, which later calls a function named "ExGetExpirationDate".
	// Due to it's variable being, we will search the PAGE section and next three sections.

	PAGESections ps[4] = { 0 };

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectNamePAGELK) { // Check if we found the PAGE\0\0\0\0 section name.
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: PAGE Section found at 0x%p with size %d\n", &KernelBase[i], *(int*)&KernelBase[i + 1]));
			int* temp = (int*)&KernelBase[i + 1];
			ps[0].size = temp[0]; // Get the section size
			ps[0].RVA = temp[1];  // and RVA
			// Get the RVA and size of next three sections.
			for (char j = 1; j < 4; j++) {
				temp += 10;
				ps[j].size = temp[0]; // Get the section size
				ps[j].RVA = temp[1];  // and RVA
			}
			break;
		}
	}

	if (!ps[0].size) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: PAGE Section not found!\n"));
		goto patchFail;
	}

	// Search for the ExpTimeRefreshWork function at the address we got from sections.
	// Finding it is easy because it has one of only two references to expiration date address at KUSER
	for (char t = 0; t < 4; t++) {
		unsigned char* PotentialTimeRef = (unsigned char*)KernelBase + ps[t].RVA;
		KernelSize2 = ps[t].size;

		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: searching at 0x%p in %lu bytes\n", PotentialTimeRef, KernelSize2));
		for (size_t i = 0; i < KernelSize2; i++) {

#ifdef _AMD64
			if (*(unsigned long long*)&PotentialTimeRef[i] == (unsigned long long)li) {
#else
			if (*(unsigned long*)&PotentialTimeRef[i] == (unsigned long)li) {
#endif
				// We found the reference of KUSER expiration date field address.
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: Potential TimeRef found at 0x%p\n", &PotentialTimeRef[i]));
				// The call to ExGetExpirationDate is a few instructions before this reference
				// So we search backwards for any CALL instruction (0xe8)
				for (unsigned char j = 0; j < 100; j++) {
					if (*(unsigned char*)&PotentialTimeRef[i - j] == 0xe8) { // CALL instruction found.
						unsigned char* pExGetExpirationDate = &PotentialTimeRef[i - j + 5];
						pExGetExpirationDate += *(unsigned int*)&PotentialTimeRef[i - j + 1]; // Next 4 bytes are relative address to our current location.
						KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser: ExGetExpirationDate found at 0x%p\n", pExGetExpirationDate));
						// Create a MDL paging to get over write protection.
						PMDL mdl = IoAllocateMdl(pExGetExpirationDate, 8, FALSE, FALSE, NULL);
						if (!mdl) {
							KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: IoAllocateMdl failed.\n"));
							goto patchFail;
						}

						MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
						void* map = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
						if (!map) {
							KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: MmMapLockedPagesSpecifyCache failed.\n"));
							goto patchFail;
						}
						MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);
						// Write to newly created MDL mapping.
						*(int*)map = 0xC3C03148; // xor eax,eax \ ret | This is apparently same for both x86 and x64
						// Unmap the MDL
						MmUnmapLockedPages(map, mdl);
						MmUnlockPages(mdl);
						IoFreeMdl(mdl);
						goto patchOK;
					}
				}
			}
		}

	}
	// No references found so far so we fail.
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: PAGE Section not found!\n"));
patchFail:
	// 0xC3C03148
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser: could not find ExpTimeRefreshWork!\n"));
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	li->QuadPart = 0; // Clear the ExpirationdDate field in SharedData. This is the last step so it will stay there in case of failure and won't cause any false positives anymore.
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[*] TimeDefuser: Patch completed successfully.\n"));
	return STATUS_SUCCESS;
}
#endif