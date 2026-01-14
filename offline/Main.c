#define TD_OFFLINE
#include "TimeDefuserOffline.h"

const wchar_t* tdGetDupFilePath() {
	// Here are all secure functions so rustfags will shut the fuck up.
	wchar_t* ret = malloc(2048);
	if (!ret) return NULL;
	memset(ret, 0, 2048);

	// Current working directory
	GetCurrentDirectoryW(1024, ret);
	// + \ntoskrnl-patched-
	wcscat_s(ret, 1024, L"\\ntoskrnl-patched-");
	// + a 'random' number to prevent conflicts
	wchar_t randomized[32] = { 0 };
	swprintf_s(randomized, 32, L"%llu", GetTickCount64());
	wcscat_s(ret, 1024, randomized);
	// + .exe
	wcscat_s(ret, 1024, L".exe");

	return ret;
}

void tdError(wchar_t* errorString, int lastError) {
	wchar_t* msg = NULL;

	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		lastError,
		0,
		(LPWSTR)&msg,
		0,
		NULL
	);

	wprintf(errorString, msg ? msg : L"(no description)");
	if (msg) LocalFree(msg);
}

int main(int argc, char* argv[]) {
	bool failed = 0;
	HANDLE f = INVALID_HANDLE_VALUE; // File handle
	HANDLE m = 0; // Memory mapping handle
	char* data = NULL; // Memory mapping
	size_t sz = 0; // File size

	if (argc < 2) {// No arguments.
		printf("Usage: %s (X:\\Path\\to\\ntoskrnl.exe)\n", argv[0]); return -1;
	}

	// Enable proper Unicode output
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "");

	// Duplicate the given file.
	const wchar_t* dupFilePath = tdGetDupFilePath();
	if (!CopyFileW(CommandLineToArgvW(GetCommandLineW(), &argc)[1], dupFilePath, 1)) {
		tdError(L"[-] Error copying the file: %ls\n", GetLastError());
		failed = 1; goto _Return;
	}

	// Open the duplicate.
	f = CreateFileW(dupFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		tdError(L"[-] Error duplicating the input file: %ls\n", GetLastError());
		failed = 1; goto _Return;
	}

	// Get file size.
	sz = GetFileSize(f, 0);

	// Create a memory mapping to duplicate.
	m = CreateFileMappingW(f, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (m == 0) {
		tdError(L"[-] Error creating a file mapping to copied file: %ls\n", GetLastError());
		failed = 1; goto _Return;
	}
	data = (char*)MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, 0);
	if (!data) {
		tdError(L"[-] Error creating a file mapping to copied file: %ls\n", GetLastError());
		failed = 1; goto _Return;
	}
	char* data0 = data;

	// Sanity checks...
	IMAGE_NT_HEADERS* nt = NULL;
	if (!tdSanityCheck(data, &nt)) {
		failed = 1; goto _Return;
	}

	// Determine architecture
	TDMachine* mach;
	switch (nt->FileHeader.Machine) {
		case IMAGE_FILE_MACHINE_I386:	mach = &tdMachineData[MACHINE_X86]; break;
		case IMAGE_FILE_MACHINE_AMD64:	mach = &tdMachineData[MACHINE_AMD64]; break;
		case IMAGE_FILE_MACHINE_ARM:	mach = &tdMachineData[MACHINE_ARM]; break;
		case IMAGE_FILE_MACHINE_ARM64:	mach = &tdMachineData[MACHINE_ARM64]; break;
		case IMAGE_FILE_MACHINE_IA64:	mach = &tdMachineData[MACHINE_IA64]; break;
		default: {
			puts("(Unknown)\n"
				"[-] Unknown machine type.");
			failed = 1; goto _Return;
		}
		break;
	}
	printf("(%s, %d bytes)\n", mach->FriendlyName, sz);

	if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 && nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
		puts("[-] This machine type is not yet supported, please open an issue at https://github.com/NevermindExpress/TimeDefuser/issues.");
		failed = 1; goto _Return;
	}

	// Check version.
	if (nt->OptionalHeader.MajorSubsystemVersion < 6) {
		puts("[-] Windows XP builds are not supported. For those, you have to use the Legacy kernel driver.");
		failed = 1; goto _Return;
	}

	// All check and stuff are done, now actual patching work begins. First we'll need to find where to patch.
	//PAGESections ps[5] = { 0 };
	IMAGE_SECTION_HEADER* PAGELK = tdFindSection("PAGELK\0", nt + 1);
	if (!PAGELK) {
		puts("[-] PAGELK section not found.");
		failed = 1; goto _Return;
	} // Search onwards it until end of the file.
	printf("[+] Searching at 0x%x within 0x%x bytes...\n", PAGELK->PointerToRawData, (int)sz - PAGELK->PointerToRawData);
	data += PAGELK->PointerToRawData;
	for (size_t i = 0; i < sz - PAGELK->PointerToRawData; i++) {
		if (*(__int64*)&data[i] == mach->SharedData) {
			// We found the time refresh work, search backwards for a CALL instruction
			printf("[+] ExGetExpirationDate found at %x\n", &data[i]-data0);
			for (unsigned char k = 0; k < 100; k++) {
				if ((unsigned char)data[i - k] == mach->callOp) { // CALL instruction found.
					printf("[+] CALL instruction found at file: 0x%x ", &data[i - k] - data0);
					char* pCall = &data[i - k] - data0;
					unsigned int Offset = *(unsigned int*)&data[i - k + 1] + 5; // Next 4 bytes are relative address to our current location.
					//pExGetExpirationDate = pExGetExpirationDate - data;
					// Convert file offset to RVA
					IMAGE_SECTION_HEADER* currentSect = tdFindSectionByAddress(pCall, nt + 1);
					if (!currentSect) {
						puts("\n[*] Invalid address, skipping this one...");
						continue;
					}
					int pCallRVA = (unsigned int)(pCall) - currentSect->PointerToRawData + currentSect->VirtualAddress;
					printf("RVA: 0x%x\n", pCallRVA);
					// Add RIP relative to calculated RVA
					pCallRVA += Offset;
					printf("[+]: Potential ExGetExpirationDate at RVA: 0x%x ", pCallRVA);
					// Convert this new RVA back to file offset.
					currentSect = tdFindSectionByRVA(pCallRVA, nt + 1);
					if (!currentSect) {
						puts("\n[*] Invalid address, skipping this one...");
						continue;
					}
					int exGetFile = currentSect->PointerToRawData + (pCallRVA - currentSect->VirtualAddress);
					printf("file: 0x%x\n", exGetFile);
					// Check if it is valid.
					if (exGetFile > sz) {
						puts("[*] Invalid address, skipping this one...");
						continue;
					}
					printf("[+] ExGetExpirationDate found at 0x%x\n", exGetFile);
					char* pExGetExpirationDate = &data0[exGetFile];
					// Patch the function.
					*(int*)pExGetExpirationDate = mach->ShellCode;
					// All is done.
					goto _PatchDone;
				}
			}
			puts("[-] Failed to find ExGetExpirationDate.");
			failed = 1; goto _Return;
		}
	}
	
	puts("[-] Failed to find ExpTimeRefreshWork.");
	failed = 1;
_PatchDone:
	// Code jumps here when patch succeeds.
	// Onwards, we will do checksum recalculation.
	nt->OptionalHeader.CheckSum = 0;
	nt->OptionalHeader.CheckSum = tdCalculateChecksum(data0, sz);

	// Cleanup and exit.
_Return:
	if (data) {
		UnmapViewOfFile(data);
	}
	if (m) {
		CloseHandle(m);
	}
	if (f != INVALID_HANDLE_VALUE) {
		CloseHandle(f);
	}
	
	if(failed) {
		DeleteFileW(dupFilePath);
		free(dupFilePath); return -1;
	}
	else {
		wprintf(L"Patch successfully completed. Patched kernel image is at \"%ws\".\n"
			L"For using the patched kernel, disable integrity checks and replace the C:\\Windows\\System32\\ntoskrnl.exe.\n"
			, dupFilePath);
		free(dupFilePath); return 0;
	}
}