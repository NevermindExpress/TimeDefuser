// Simplified header for offline version of TimeDefuser
#pragma once
#ifndef TD_OFFLINE
	#error You are not supposed to use this header in the kernel driver.
#endif // !TD_OFFLINE

#define td_version "1.8.4"

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <locale.h>
#include <Windows.h>

// Structures
typedef struct TD_MACHINE {
	const char* FriendlyName;
	unsigned int ShellCode; // Shell code for "return 0" that will be patched to the function.
	__int64 SharedData; // Where "ExpirationDate" on KUSER_SHARED_DATA is
	bool is64; unsigned char callOp;
} TDMachine;

typedef struct {
	unsigned long RVA;
	unsigned long size;
} PAGESections;

// Definitions
enum tdMachineTypes {
	MACHINE_X86,
	MACHINE_AMD64,
	MACHINE_ARM,
	MACHINE_ARM64,
	MACHINE_IA64,
};

extern TDMachine tdMachineData[];

#ifndef IMAGE_FILE_MACHINE_ARM64
	#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

// TD Functions
extern bool tdSanityCheck(char* data, IMAGE_NT_HEADERS** nt);
extern IMAGE_SECTION_HEADER* tdFindSection(const char* name, IMAGE_SECTION_HEADER* data);
extern IMAGE_SECTION_HEADER* tdFindSectionByAddress(unsigned int addr, IMAGE_SECTION_HEADER* data);
extern IMAGE_SECTION_HEADER* tdFindSectionByRVA(unsigned int addr, IMAGE_SECTION_HEADER* data);
extern DWORD tdCalculateChecksum(BYTE* data, DWORD fileSize);