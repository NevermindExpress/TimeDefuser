#define TD_OFFLINE
#include "TimeDefuserOffline.h",
const wchar_t CompanyKey[]		= L"CompanyName";
const wchar_t CompanyValue[]	= L"Microsoft Corporation";
const wchar_t DescKey[]			= L"FileDescription";
const wchar_t DescValue[]		= L"NT Kernel & System";
const wchar_t VerKey[]			= L"FileVersion";
const wchar_t VersionStart[]	= L"StringFileInfo";

bool tdSanityCheck(char* data, IMAGE_NT_HEADERS** nt) {
	// MZ file?
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)data;

	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		printf("[-] MZ signature check mismatch. (expected MZ).\n");
		return 0;
	}
	if (dos->e_lfanew <= 0 || dos->e_lfanew > 0x100000) {
		printf("[-] Suspicious e_lfanew value.\n");
		return 0;
	}
	// PE file?
	*nt = (IMAGE_NT_HEADERS*)(data + dos->e_lfanew);

	if ((*nt)->Signature != IMAGE_NT_SIGNATURE) {
		printf("[-] PE signature check mismatch. (expected PE\\0\\0).\n");
		return 0;
	}
	// Executable?
	if (!((*nt)->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
		printf("[-] File is not marked as an executable image.\n");
		return 0;
	}
	// NATIVE subsystem?
	if ((*nt)->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) {
		printf("[-] Not a kernel image (subsystem).\n");
		return 0;
	}
	IMAGE_DATA_DIRECTORY* rdir =
		&(*nt)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	// Has resources?
	if (!rdir->VirtualAddress || !rdir->Size) {
		printf("[-] Not a kernel image (lacks resources).\n");
		return 0;
	}
	// Has resources. Now we gotta find where they are in image.
	// 
	// Find the .rsrc section
	IMAGE_SECTION_HEADER* sect = tdFindSection(".rsrc\0\0", *nt + 1);
	if (!sect) {
		printf("[-] Not a kernel image (no resources).\n");
		return 0;
	}
	// We found it, now we gotta check some values at version info...
	// Search for the version info start.
	data += sect->PointerToRawData;
	size_t i;
	for (i = 0; i < sect->SizeOfRawData; i++) {
		if (!memcmp(data + i, VersionStart, sizeof(VersionStart))) {
			data += i + 30 + sizeof(VersionStart); // Skip StringFileInfo block headers
			break;
		}
	}
	if (i == sect->SizeOfRawData) {
		printf("[-] Not a kernel image (no version info).\n");
		return 0;
	}
	// Check company
	if (memcmp(data, CompanyKey, sizeof(CompanyKey))) {
		printf("[-] Not a kernel image (no company).\n"); return 0;
	} data += sizeof(CompanyKey) + 2;
	if (memcmp(data, CompanyValue, sizeof(CompanyValue))) {
		printf("[-] Not a kernel image (company).\n"); return 0;
	} data += sizeof(CompanyValue) + 6;
	// Check description
	if (memcmp(data, DescKey, sizeof(DescKey))) {
		printf("[-] Not a kernel image (no description).\n"); return 0;
	} data += sizeof(DescKey) + 2;
	if (memcmp(data, DescValue, sizeof(DescValue))) {
		printf("[-] Not a kernel image (description).\n"); return 0;
	} data += sizeof(DescValue) + 8;
	// Check existence of version info.
	if (memcmp(data, VerKey, sizeof(VerKey))) {
		printf("[-] Not a kernel image (no version).\n"); return 0;
	} data += sizeof(VerKey) + 2;
	// All OK
	wprintf(L"[+] Valid kernel image %ls ", (wchar_t*)data);
	return 1;
}
