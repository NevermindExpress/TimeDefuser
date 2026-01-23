#include "TimeDefuser.h"

typedef struct {
	ULONG Major;
	ULONG Minor;
	ULONG Build;
} tdkernelVersion;

HANDLE OpenRegistryKey(PUNICODE_STRING KeyPath){
	HANDLE ret = 0;
	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(
		&oa,
		KeyPath,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	);

	ZwCreateKey(
		&ret,
		KEY_READ | KEY_WRITE,
		&oa,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		NULL
	);

	return ret;
}

// Writes a DWORD value
NTSTATUS RegWriteDword(HANDLE hKey, PCWSTR ValueName, ULONG Data) {
	UNICODE_STRING valName;
	RtlInitUnicodeString(&valName, ValueName);

	return ZwSetValueKey(
		hKey,
		&valName,
		0,
		REG_DWORD,
		&Data,
		sizeof(Data)
		);
}

// Reads a value from registry
// If DWORD, return value is the value.
// Else, value is saved at ValueOutput.
ULONG RegReadValue(_In_ HANDLE KeyHandle, _In_ PCWSTR ValueName, _Out_ PVOID ValueOutput, _In_ ULONG ValueOutputSz) {
	NTSTATUS status;
	PKEY_VALUE_PARTIAL_INFORMATION kvpi;
	ULONG size = sizeof(KEY_VALUE_PARTIAL_INFORMATION)+128;
	ULONG retLen = 0, ret = 0;
	UNICODE_STRING valName;

	// Allocate memory for kvpi
	kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, size, 'rgeR');
	if (!kvpi) return 0;

	RtlInitUnicodeString(&valName, ValueName);

	status = ZwQueryValueKey(
		KeyHandle,
		&valName,
		KeyValuePartialInformation,
		kvpi,
		size,
		&retLen
		);

	if (NT_SUCCESS(status)) {
		if (kvpi->Type == REG_DWORD && kvpi->DataLength == sizeof(ULONG)) {
			ret = *(ULONG*)kvpi->Data;
		}
		else {
			if (kvpi->DataLength < ValueOutputSz) {
				ret = kvpi->DataLength;
				//RtlCopyMemory(ValueOutput, kvpi->Data, ret);
				// ^^^ this shit fucks up sooo here is a poor man's memcpy
				char* Data = kvpi->Data;
				while (kvpi->DataLength >= 8) {
					*(__int64*)ValueOutput = *(__int64*)Data; Data += 8;
					(char*)ValueOutput += 8; kvpi->DataLength -= 8;
				}
				while (kvpi->DataLength) {
					*(char*)ValueOutput = Data[0]; Data++;
					(char*)ValueOutput += 1; kvpi->DataLength--;
				}
			}
			else status = STATUS_INVALID_PARAMETER;
		}
	}

	ExFreePoolWithTag(kvpi, 'rgeR');
	return ret;
}

// Saves kernel build version to a value key named "KernelVersion"
NTSTATUS SaveKernelVersion(_In_ HANDLE hKey) {
	UNICODE_STRING valName;
	NTSTATUS status;
	ULONG major, minor, build;
	tdkernelVersion ver;

	RtlInitUnicodeString(&valName, L"KernelVersion");

	PsGetVersion(&major, &minor, &build, NULL);

	ver.Major = major;
	ver.Minor = minor;
	ver.Build = build;

	status = ZwSetValueKey(
		hKey,
		&valName,
		0,
		REG_BINARY,
		&ver,
		sizeof(ver)
		);

	return status;
}

BOOLEAN CompareKernelVersion(_In_ HANDLE hKey) {
	// Firstly we will read the version of current kernel and make it a string.
	ULONG major, minor, build;
	tdkernelVersion currentVer = { 0 }, regVer = { 0 };
	PsGetVersion(&major, &minor, &build, NULL);

	currentVer.Major = major;
	currentVer.Minor = minor;
	currentVer.Build = build;

	// Then we will get the value from registry.
	if (!RegReadValue(hKey, L"KernelVersion", &regVer, sizeof(regVer))) return FALSE;

	// Lastly we will compare and return.
	return currentVer.Major == regVer.Major &&
		currentVer.Minor == regVer.Minor &&
		currentVer.Build == regVer.Build;
}