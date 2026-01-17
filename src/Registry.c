#include "TimeDefuser.h"

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
				RtlCopyMemory(ValueOutput, kvpi->Data, ret);
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

	RtlInitUnicodeString(&valName, L"KernelVersion");

	ULONG major, minor, build;
	PsGetVersion(&major, &minor, &build, NULL);

	WCHAR kernelVersionString[64] = L"";
	swprintf(kernelVersionString, 64, L"%u.%u.%u", major, minor, build);
	

	SIZE_T len = (wcslen(kernelVersionString) + 1) * sizeof(WCHAR);

	status = ZwSetValueKey(
		hKey,
		&valName,
		0,
		REG_SZ,
		(PVOID)kernelVersionString,
		(ULONG)len
		);

	return status;
}

BOOLEAN CompareKernelVersion(_In_ HANDLE hKey) {
	// Firstly we will read the version of current kernel and make it a string.
	ULONG major, minor, build;
	PsGetVersion(&major, &minor, &build, NULL);
	WCHAR kernelVersionCurrent[64] = L"";
	swprintf(kernelVersionCurrent, 64, L"%u.%u.%u", major, minor, build);

	// Then we will get the value from registry.
	WCHAR kernelVersionReg[64] = L"";
	if (!RegReadValue(hKey, L"KernelVersion", kernelVersionReg, 64)) return FALSE;

	// Lastly we will compare and return.
	return wcscmp(kernelVersionCurrent, kernelVersionReg) == 0;
}