#include "TimeDefuser.h"

#ifndef TD_LEGACY
BOOLEAN PatchExGetExpirationDate(void* pExGetExpirationDate) {
	PMDL mdl = NULL;
	void* map = NULL;
	// Create a MDL paging to get over write protection.
	mdl = IoAllocateMdl(pExGetExpirationDate, 8, FALSE, FALSE, NULL);
	if (!mdl) {
		TDPrint("[X] TimeDefuser: IoAllocateMdl failed.\n");
		return FALSE;
	}

	MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
	map = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	if (!map) {
		TDPrint("[X] TimeDefuser: MmMapLockedPagesSpecifyCache failed.\n");
		return FALSE;
	}
	MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);
	// Write to newly created MDL mapping.
	*(int*)map = 0xC3C03148; // xor eax,eax \ ret | This is apparently same for both x86 and x64
	// Unmap the MDL
	MmUnmapLockedPages(map, mdl);
	MmUnlockPages(mdl);
	IoFreeMdl(mdl);
	return TRUE;
}
#endif
#define PDRIVER_OBEJCT void* // Unused by TimeDefuser

NTSTATUS DriverEntry(void* DriverObject, PUNICODE_STRING RegistryPath) {
	LARGE_INTEGER* li = KUSERSystemExpirationDate; // Address of SystemExpirationDate field at KUSER_SHARED_DATA
	unsigned long long TimebombStamp = 0;	// Expiration date stamp
	RTL_PROCESS_MODULES ModuleInfo = { 0 };	// Structure used for getting kernel base address
	unsigned long long* KernelBase = NULL;	// Kernel Base address
	ULONG KernelSize = 0;					// Kernel image size
	HANDLE hKey = OpenRegistryKey(RegistryPath);
#ifndef TD_LEGACY
	unsigned int KernelSize2 = 0;			// Var used in loops as a max value
	PAGESections ps[5] = { 0 };				// PE sections that name starts with "PAGE"
	unsigned char* PotentialTimestamp = NULL;// Potential address of ExNtExpirationDate/a
#endif

	// Unrefence unused variables.
	UNREFERENCED_PARAMETER(DriverObject);

	// Print version info.
	TDPrint("[*] TimeDefuser: version " td_version td_variant" loaded "
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

	// Check whether addresses are cached
	if (hKey) {
		if (CompareKernelVersion(hKey)) {
			// Get cached address offsets for timestamps.
			int Stamp1 = RegReadValue(hKey, L"Stamp1", NULL, 0),
				Stamp2 = RegReadValue(hKey, L"Stamp2", NULL, 0);

			// Zero first timestamp
			if (!Stamp1) {
				// No cached address, assume nothing is cached.
				goto patchBeginning;
			}
			TDPrint("[*] TimeDefuser: Cached addresses are found on registry.\n");
			TDPrint("[+] TimeDefuser: Cached ExpNtExpirationDate address 0x%p is used.\n", (unsigned long long*)((char*)KernelBase + Stamp1));
			*(unsigned long long*)((char*)KernelBase+Stamp1) = 0;
			#ifdef TD_LEGACY
				// On legacy, for some reason, actual timebomb stamp 
				// is the next qword (on XP 2526). We will zero that too.
				*(unsigned long long*)((char*)KernelBase+Stamp1+8) = 0;
			#endif
			
			// Zero second timestamp if available.
			if (Stamp2) {
				TDPrint("[+] TimeDefuser: Cached ExpNtExpirationData address 0x%p is used.\n", (char*)KernelBase + Stamp2);
				#ifdef TD_LEGACY
					RtlZeroMemory((char*)KernelBase + Stamp2, 16);
				#else
					*(unsigned long long*)((char*)KernelBase + Stamp2) = 0;
				#endif
			}

#ifndef TD_LEGACY
			int Function = RegReadValue(hKey, L"Function", NULL, 0);
			TDPrint("[+] TimeDefuser: Cached ExGetExpirationDate function address 0x%p is used.\n", (char*)KernelBase + Function);
			if (!PatchExGetExpirationDate((char*)KernelBase + Function))
				goto patchFail;
#endif
			goto patchOK;
		}
		// Kernel version mismatch.
	}
patchBeginning:
	TDPrint("[*] TimeDefuser: No or mismatching cached addresses are found on registry.\n");
	SaveKernelVersion(hKey);
#ifdef TD_LEGACY
	// Search for timebomb stamp in memory
	KernelSize /= sizeof(unsigned __int64);
	for (unsigned int i = 0; i < KernelSize; i++) {
		if (KernelBase[i] == TimebombStamp) {
			TDPrint("[+] TimeDefuser: ExpNtExpirationDate found at 0x%p\n", &KernelBase[i]);
			KernelBase[i] = 0;
			// For some reason actual timebomb was the next qword on XP 2526, I'll save this and search for it again.
			TimebombStamp = KernelBase[i + 1]; // Save the lower part of stamp.
			KernelBase[i + 1] = 0; // And null where I found it too.
			RegWriteDword(hKey, L"Stamp1", (ULONG)((unsigned char*)&KernelBase[i] - (unsigned char*)KernelBase));
			break;
		}
	}

	// Search for the second stamp, ExpNtExpirationData (and not Date)
	for (unsigned int i = 0; i < KernelSize; i++) {
		if ((int)KernelBase[i] == (int)TimebombStamp) {
			TDPrint("[+] TimeDefuser: ExpNtExpirationData found at 0x%p\n", &KernelBase[i]);
			RtlZeroMemory(&KernelBase[i], 16);
			RegWriteDword(hKey, L"Stamp2", (ULONG)((unsigned char*)&KernelBase[i] - (unsigned char*)KernelBase));
			goto patchOK;
		}
	}
	// That's all for legacy implementation, get out.

#else
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
			KernelSize2 = *(int*)&KernelBase[i + 1]; // Get the section size
			// Get the function RVA and append it to kernel base address.
			int* asd = (int*)&KernelBase[i + 1];
			PotentialTimestamp += asd[1];
			TDPrint("[+] TimeDefuser: PAGEDATA Section found at 0x%p with size %d\n", PotentialTimestamp, *(int*)&KernelBase[i + 1]);
			break;
		}
	}
	if (PotentialTimestamp == (unsigned char*)KernelBase) {
		TDPrint("[X] TimeDefuser: PAGEDATA Section not found!\n");
		goto patchFail;
	}

	// Search for timebomb stamp in memory
	CHAR occurance = FALSE;
	void* pExpNtExpirationDate = NULL;

	TDPrint("[+] TimeDefuser: searching for stamp at 0x%p in %d bytes\n", PotentialTimestamp, KernelSize2);

	KernelSize2;
	for (ULONG i = 0; i < KernelSize2; i++) {
		if (*(unsigned long long*) & PotentialTimestamp[i] == TimebombStamp) {
			TDPrint("[+] TimeDefuser: Timebomb stamp found at 0x%p\n", &PotentialTimestamp[i]);
			*(unsigned long long*)(&PotentialTimestamp[i]) = 0;
			pExpNtExpirationDate = &PotentialTimestamp[i];

			if (occurance) {
				pExpNtExpirationDate = &PotentialTimestamp[i];
				RegWriteDword(hKey, L"Stamp2", (ULONG)(&PotentialTimestamp[i] - (unsigned char*)KernelBase));
				occurance = 2;
				break;
			}
			else { 
				occurance = 1; 
				RegWriteDword(hKey, L"Stamp1", (ULONG)((unsigned char*)&PotentialTimestamp[i] - (unsigned char*)KernelBase));
			}
		}
	}

	// Print the address according to occurrance.
	switch (occurance) {
		case 0:
			TDPrint("[X] TimeDefuser: can't find ExpNtExpirationDate!\n");
			goto patchFail; 
			break;
		case 1:
			TDPrint("[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (first occurrance)\n", pExpNtExpirationDate);
			break;
		case 2:
			TDPrint("[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (second occurrance)\n", pExpNtExpirationDate);
			break;
	}

	// Search for PAGE section at PE sections. This section or one of the next three sections is where the 
	// "ExpTimeRefreshWork" function is located at, which later calls a function named "ExGetExpirationDate".
	// Due to it's variable being, we will search the PAGE section and next three sections.

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectNamePAGELK) { // Check if we found the PAGELK\0\0 section name.
			int* temp = (int*)&KernelBase[i + 1];
			ps[0].size = temp[0]; // Get the section size
			ps[0].RVA = temp[1];  // and RVA
			TDPrint("[+] TimeDefuser: PAGELK Section found at 0x%p with size %d\n", (unsigned char*)KernelBase + temp[1], temp[0]);
			// Get the RVA and size of next three sections.
			for (char j = 1; j < 5; j++) {
				temp += 10;
				ps[j].size = temp[0]; // Get the section size
				ps[j].RVA = temp[1];  // and RVA
			}
			break;
		}
	}

	if (!ps[0].size) {
		TDPrint("[X] TimeDefuser: PAGELK Section not found!\n");
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
					if (PotentialTimeRef[i - j] == 0xe8) { // CALL instruction found.
						TDPrint("[+] TimeDefuser: CALL instruction found at 0x%p\n", &PotentialTimeRef[i - j]);
						unsigned char* pExGetExpirationDate = &PotentialTimeRef[i - j + 5];
						pExGetExpirationDate += *(unsigned int*)&PotentialTimeRef[i - j + 1]; // Next 4 bytes are relative address to our current location.
						RegWriteDword(hKey, L"Function", (ULONG)(pExGetExpirationDate - (unsigned char*)KernelBase));
						// Check if we are running at one of shit builds, refer to ExGetExpirationDateShim.
						if (!MmIsAddressValid(pExGetExpirationDate)) {
							TDPrint("[*] TimeDefuser: Invalid address, skipping this one...\n", pExGetExpirationDate);
							continue;
						}

						TDPrint("[+] TimeDefuser: ExGetExpirationDate found at 0x%p\n", pExGetExpirationDate);
						if (!PatchExGetExpirationDate(pExGetExpirationDate))
							goto patchFail;
						else {
							goto patchOK;
						}


					}
				}
				break;
			}
		}
	}
	// No references found so far so we fail.
	TDPrint("[X] TimeDefuser: could not find ExpTimeRefreshWork!\n");

#endif

patchFail:
	TDPrint("[X] TimeDefuser: Patch failed.\n");
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	// Clear the ExpirationdDate field in SharedData. 
	// Since 1.4, this is the last step so it will stay there in case of failure 
	// and won't cause any false positives anymore.
	li->QuadPart = 0; ZwClose(hKey);
	TDPrint("[*] TimeDefuser: Patch completed successfully.\n");
	return STATUS_SUCCESS;
}
