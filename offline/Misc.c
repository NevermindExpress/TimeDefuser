#define TD_OFFLINE
#include "TimeDefuserOffline.h"

TDMachine tdMachineData[] = {
	{"x86", 0xC3C03148 ,0xffdf02c8, false, 0xE8 },
	{"AMD64", 0xC3C03148 ,0xfffff780000002c8, true, 0xE8 },
	{"ARM", 0 ,0, false, 0x00 },
	{"ARM64", 0 ,0, true, 0x00 },
	{"Itanium", 0 ,0, true, 0x00 }
};

IMAGE_SECTION_HEADER* tdFindSection(const char* name, IMAGE_SECTION_HEADER* data) {
	for (size_t i = 0; i < 36; i++) {
		if (!memcmp(data[i].Name, name, 8)) {
			return &data[i];
		}
	}
	return NULL;
}

IMAGE_SECTION_HEADER* tdFindSectionByAddress(const char* addr, IMAGE_SECTION_HEADER* data) {
	for (size_t i = 0; i < 36; i++) {
		if (addr > data[i].PointerToRawData && addr < data[i].PointerToRawData + data[i].SizeOfRawData) {
			return &data[i];
		}
	}
	return NULL;
}

IMAGE_SECTION_HEADER* tdFindSectionByRVA(const char* addr, IMAGE_SECTION_HEADER* data) {
	for (size_t i = 0; i < 36; i++) {
		if (addr > data[i].VirtualAddress && addr < data[i].VirtualAddress + data[i].Misc.VirtualSize) {
			return &data[i];
		}
	}
	return NULL;
}

DWORD tdCalculateChecksum(BYTE* data, DWORD fileSize) {
	DWORD sum = 0;
	DWORD i = 0;

	while (i + 1 < fileSize) {
		sum += *(WORD*)(data + i);
		sum = (sum & 0xFFFF) + (sum >> 16);
		i += 2;
	}

	// Handle odd file size
	if (i < fileSize) {
		sum += data[i];
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	// Final fold
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum = (sum & 0xFFFF) + (sum >> 16);

	// Add file length
	sum += fileSize;

	return sum;
}