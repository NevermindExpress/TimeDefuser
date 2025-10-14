#if NTDDI_VERSION >= 0x06000000
#include "TimeDefuser.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	LARGE_INTEGER* li = KUSERSystemExpirationDate; // Address of SystemExpirationDate field at KUSER_SHARED_DATA
	unsigned long long TimebombStamp = 0;	// Expiration date stamp
	RTL_PROCESS_MODULES ModuleInfo = { 0 };	// Structure used for getting kernel base address
	unsigned long long* KernelBase = NULL;	// Kernel Base address
	ULONG KernelSize = 0;					// Kernel image size
	unsigned int KernelSize2 = 0;			// Var used in loops as a max value
	PAGESections ps[4] = { 0 };				// PE sections that name starts with "PAGE"
	unsigned char* PotentialTimestamp;		// Potential address of ExNtExpirationDate/a

	// Unrefence unused variables.
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	// Print version info.
	TDPrint("[*] TimeDefuser: version " td_version " loaded "
			"| Compiled on " __DATE__ " " __TIME__ " "
			"| https://github.com/NevermindExpress/TimeDefuser\n");

	// Get SystemExpirationDate
	TimebombStamp = li->QuadPart;
	if (!TimebombStamp) {
		TDPrint("[X] TimeDefuser: No timebomb found, exiting.\n");
		return STATUS_FAILED_DRIVER_ENTRY;
	}
	TDPrint("[+] TimeDefuser: SystemExpirationDate is 0x%llx\n", TimebombStamp);

	// Get kernel base
	ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	if (ModuleInfo.NumberOfModules == 0) {
		TDPrint("[X] TimeDefuser: Failed to get kernel base address.\n");
		goto patchFail;
	}
	KernelBase = (unsigned long long*)ModuleInfo.Modules[0].ImageBase;
	KernelSize = ModuleInfo.Modules[0].ImageSize; 
	TDPrint("[+] TimeDefuser: Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize);

	// Check for PE Header existance.
	if (*(short*)KernelBase != PEheader) {
		TDPrint("[X] TimeDefuser: PE Header not found!\n");
		goto patchFail;
	}

	PotentialTimestamp = (unsigned char*)KernelBase;

	// Search for "PAGEDATA" section at PE sections. This section is where the 
	// ExpNtExpirationDate timestamp variable is located at, so we are going 
	// to use its RVA and size for finding the function location.

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectNamePAGEDATA) { // Check if we found the PAGEDATA section name.
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
		if (*(unsigned long long*) & PotentialTimestamp[i] == TimebombStamp) {
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

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectNamePAGE) { // Check if we found the PAGE\0\0\0\0 section name.
			TDPrint("[+] TimeDefuser: PAGE Section found at 0x%p with size %d\n", &KernelBase[i], *(int*)&KernelBase[i + 1]);
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
		TDPrint("[X] TimeDefuser: PAGE Section not found!\n");
		goto patchFail;
	}

	// Search for the ExpTimeRefreshWork function at the address we got from sections.
	// Finding it is easy because it has one of only two references to expiration date address at KUSER
	for (char t = 0; t < 4; t++) {
		unsigned char* PotentialTimeRef = (unsigned char*)KernelBase + ps[t].RVA;
		KernelSize2 = ps[t].size;

		TDPrint("[+] TimeDefuser: searching at 0x%p in %lu bytes\n", PotentialTimeRef, KernelSize2);
		for (size_t i = 0; i < KernelSize2; i++) {

			if (*(ptr_t*)&PotentialTimeRef[i] == (ptr_t)li) {
				// We found the reference of KUSER expiration date field address.
				TDPrint("[+] TimeDefuser: Potential TimeRef found at 0x%p\n", &PotentialTimeRef[i]);
				// The call to ExGetExpirationDate is a few instructions before this reference
				// So we search backwards for any CALL instruction (0xe8)
				for (unsigned char j = 0; j < 100; j++) {
					if (*(unsigned char*)&PotentialTimeRef[i - j] == 0xe8) { // CALL instruction found.
						unsigned char* pExGetExpirationDate = &PotentialTimeRef[i - j + 5];
						PMDL mdl = NULL;
						void* map = NULL;
						pExGetExpirationDate += *(unsigned int*)&PotentialTimeRef[i - j + 1]; // Next 4 bytes are relative address to our current location.
						TDPrint("[+] TimeDefuser: ExGetExpirationDate found at 0x%p\n", pExGetExpirationDate);
						// Create a MDL paging to get over write protection.
						mdl = IoAllocateMdl(pExGetExpirationDate, 8, FALSE, FALSE, NULL);
						if (!mdl) {
							TDPrint("[X] TimeDefuser: IoAllocateMdl failed.\n");
							goto patchFail;
						}

						MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
						map = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
						if (!map) {
							TDPrint("[X] TimeDefuser: MmMapLockedPagesSpecifyCache failed.\n");
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
				break;
			}
		}
	}
	// No references found so far so we fail.
	TDPrint("[X] TimeDefuser: could not find ExpTimeRefreshWork!\n");
patchFail:
	TDPrint("[X] TimeDefuser: Patch failed.\n");
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	// Clear the ExpirationdDate field in SharedData. 
	// Since 1.4, this is the last step so it will stay there in case of failure 
	// and won't cause any false positives anymore.
	li->QuadPart = 0;
	TDPrint("[*] TimeDefuser: Patch completed successfully.\n");
	return STATUS_SUCCESS;
}
#endif
