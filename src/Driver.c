#include "TimeDefuser.h"

#pragma function(memset)
void* __cdecl memset(void* dest, int c, size_t count) {
	unsigned char* p = (unsigned char*)dest;
	while (count--) {
		*p++ = (unsigned char)c;
	}
	return dest;
}

BOOLEAN PatchExGetExpirationDate(void* pExGetExpirationDate) {
	PMDL mdl = NULL;
	unsigned char* map = NULL;
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
#ifdef _M_IX86
	*(int*)map = 0x9090C031; // xor eax,eax \ times 2 nop
	map[4] = 0x90; // nop
	if (map[-5] == 0x68) {
		*(int*)(map - 4) = 0x90909090; // times 4 nop
		map[-5] = 0x90; // nop
	}
#elif defined(_M_AMD64)
	*(int*)map = 0xC3C03148; // xor rax,rax \ ret
#endif // _M_IX86
	// Unmap the MDL
	MmUnmapLockedPages(map, mdl);
	MmUnlockPages(mdl);
	IoFreeMdl(mdl);
	return TRUE;
}

NTSTATUS DriverEntry(PDRIVER_OBEJCT DriverObject, PUNICODE_STRING RegistryPath) {
	LARGE_INTEGER* li = KUSERSystemExpirationDate;	// Address of SystemExpirationDate field at KUSER_SHARED_DATA
	unsigned long long TimebombStamp = 0;			// Expiration date stamp
	RTL_PROCESS_MODULES ModuleInfo = { 0 };			// Structure used for getting kernel base address
	unsigned char* KernelBase = NULL;			// Kernel Base address
	ULONG KernelSize = 0;							// Kernel image size
	HANDLE hKey = OpenRegistryKey(RegistryPath);	// Registry Key
	PAGESections ps[10] = { 0 };					// PE sections that we need and will search in
	BOOLEAN Legacy = FALSE;							// Is the current system legacy or not?
	int verMajor = 0;								// Kernel major version
	IMAGE_FILE_HEADER* nt = NULL;					// PE File Header
	IMAGE_SECTION_HEADER* sh = NULL;				// PE Section Header Array

	// Unrefence unused variables.
	UNREFERENCED_PARAMETER(DriverObject);

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

	// Determine if we are running in a legacy system
	{
		PsGetVersion(&verMajor, 0, 0, 0);
		if (verMajor == 5) {
			TDPrint("[*] TimeDefuser: Legacy system detected.\n");
			Legacy = TRUE;
		}
	}

	// Get kernel base
	ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	if (ModuleInfo.NumberOfModules == 0) {
		TDPrint("[X] TimeDefuser: Failed to get kernel base address.\n");
		goto patchFail;
	}
	KernelBase = (unsigned char*)ModuleInfo.Modules[0].ImageBase;
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
			if(Legacy) {
				// On legacy, for some reason, actual timebomb stamp 
				// is the next qword (on XP 2526). We will zero that too.
				*(unsigned long long*)((char*)KernelBase + Stamp1 + 8) = 0;
			}
			
			// Zero second timestamp if available.
			if (Stamp2) {
				TDPrint("[+] TimeDefuser: Cached ExpNtExpirationData address 0x%p is used.\n", (char*)KernelBase + Stamp2);
				*(unsigned long long*)((char*)KernelBase + Stamp2) = 0;
				if (Legacy)
					*(unsigned long long*)((char*)KernelBase + Stamp2 + 8) = 0;
			}

			// ExGetExpirationDate Function if not legacy.
			if (!Legacy) {
				int Function = RegReadValue(hKey, L"Function", NULL, 0);
				TDPrint("[+] TimeDefuser: Cached ExGetExpirationDate function address 0x%p is used.\n", (char*)KernelBase + Function);
				if (!PatchExGetExpirationDate((char*)KernelBase + Function))
					goto patchFail;
			}
			goto patchOK;
		}
		// Kernel version mismatch.
	}
patchBeginning:
	TDPrint("[*] TimeDefuser: No or mismatching cached addresses are found on registry.\n");
	SaveKernelVersion(hKey);

	if (Legacy) {
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
				KernelBase[i] = KernelBase[i + 1] = 0;
				RegWriteDword(hKey, L"Stamp2", (ULONG)((unsigned char*)&KernelBase[i] - (unsigned char*)KernelBase));
				goto patchOK;
			}
		}
		// That's all for legacy implementation, get out.
		goto patchOK;
	}

	// Check for MZ Header existance.
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)KernelBase;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		TDPrint("[X] TimeDefuser: MZ Header not found!\n");
		goto patchFail;
	}

	// Check for PE Header existance.
	if (*(unsigned int*)&KernelBase[dos->e_lfanew] != IMAGE_NT_SIGNATURE) {
		TDPrint("[X] TimeDefuser: PE Header not found!\n");
		goto patchFail;
	}

	// Find where section headers are
	(unsigned char*)nt = (unsigned char*)KernelBase + dos->e_lfanew + 4;
	sh = (IMAGE_SECTION_HEADER*)((unsigned char*)(nt + 1) + nt->SizeOfOptionalHeader);

	// Search for all sections and save the ones we need
	for (size_t i = 0; i < nt->NumberOfSections; i++) {

		for (unsigned wanted = 0; wanted < 9; wanted++) {

			if (*(UINT64*)sh[i].Name != *(UINT64*)Sections[wanted]) continue;

			ps[wanted].RVA =  sh[i].VirtualAddress;
			ps[wanted].size = sh[i].Misc.VirtualSize;

			TDPrint(
				"[+] TimeDefuser: Section %.8s found at 0x%p with size %u\n", 
				sh[i].Name, (unsigned char*)KernelBase + sh[i].VirtualAddress, sh[i].Misc.VirtualSize
			);

			break;
		}
	}

	// Search for timebomb stamp in memory
	{
		CHAR occurance = FALSE;
		void* pExpNtExpirationDate = NULL;
		char* rva = NULL;

			// ps[3] is PAGEDATA
			if (!ps[3].size) {
				TDPrint("[X] TimeDefuser: PAGEDATA Section not found!\n");
				goto patchFail;
			} rva = (char*)KernelBase + ps[3].RVA;

		TDPrint("[+] TimeDefuser: searching for stamps at 0x%p in %d bytes\n", rva, ps[3].size);

		for (ULONG i = 0; i < ps[3].size; i++) {
			if (*(unsigned long long*)&rva[i] == TimebombStamp) {
				TDPrint("[+] TimeDefuser: Timebomb stamp found at 0x%p\n", &rva[i]);
				*(unsigned long long*)(&rva[i]) = 0;
				pExpNtExpirationDate = &rva[i];

				if (occurance) {
					pExpNtExpirationDate = &rva[i];
					RegWriteDword(hKey, L"Stamp2", (ULONG)(&rva[i] - (unsigned char*)KernelBase));
					occurance = 2;
					break;
				}
				else {
					occurance = 1;
					RegWriteDword(hKey, L"Stamp1", (ULONG)((unsigned char*)&rva[i] - (unsigned char*)KernelBase));
				}
			}
		}

		// Print the address according to occurrance.
		switch (occurance) {
			case 0:
				TDPrint("[X] TimeDefuser: can't find ExpNtExpirationDate!\n");
				//goto patchFail; 
				break; // It actually shouldn't be fatal.
			case 1:
				TDPrint("[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (first occurrance)\n", pExpNtExpirationDate);
				break;
			case 2:
				TDPrint("[+] TimeDefuser: ExpNtExpirationDate address is 0x%p (second occurrance)\n", pExpNtExpirationDate);
				break;
		}
	}

	// Search for the ExpTimeRefreshWork function at the addresses we got from sections.
	// Finding it is easy because it has one of only two references to expiration date address at KUSER
	for (char t = 0; t < 10; t++) {
		unsigned char* PotentialTimeRef = (unsigned char*)KernelBase + ps[t].RVA;

		TDPrint("[+] TimeDefuser: searching at 0x%p in %lu bytes\n", PotentialTimeRef, ps[t].size);
		for (size_t i = 0; i < ps[t].size; i++) {

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
						
						// Check if we are running at one of shit builds, refer to ExGetExpirationDateShim.
						if (!MmIsAddressValid(pExGetExpirationDate)) {
							TDPrint("[*] TimeDefuser: Invalid address, skipping this one...\n", pExGetExpirationDate);
							continue;
						}

						TDPrint("[+] TimeDefuser: ExGetExpirationDate found at 0x%p\n", pExGetExpirationDate);
#ifdef _M_IX86
						// Caller is patched on x86
						if (PatchExGetExpirationDate(&PotentialTimeRef[i - j])) {
							RegWriteDword(hKey, L"Function", (ULONG)(&PotentialTimeRef[i - j] - (unsigned char*)KernelBase));
#else
						if (PatchExGetExpirationDate(pExGetExpirationDate)) {
							RegWriteDword(hKey, L"Function", (ULONG)(pExGetExpirationDate - (unsigned char*)KernelBase));
#endif
							goto patchOK;
						}
						else {
							goto patchFail;
						}


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
	li->QuadPart = 0; ZwClose(hKey);
	TDPrint("[+] TimeDefuser: Patch completed successfully.\n");
	return STATUS_SUCCESS;
}
