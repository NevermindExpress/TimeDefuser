#include <ntddk.h>
#include <wdf.h>

#define SystemModuleInformation 11

extern __kernel_entry NTSTATUS NTAPI ZwQuerySystemInformation(
	IN int SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
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
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	_Field_size_(NumberOfModules) RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

// fffff78000000000
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD TimeDefuserDeviceAdd;

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT     DriverObject,
	_In_ PUNICODE_STRING    RegistryPath
) {
	// NTSTATUS variable to record success or failure
	NTSTATUS status = STATUS_SUCCESS;

	// Allocate the driver configuration object
	WDF_DRIVER_CONFIG config;
	// Address of SystemExpirationDate field at KUSER_SHARED_DATA
	LARGE_INTEGER* li = (LARGE_INTEGER*)0xfffff780000002c8; 

	// Change SystemExpirationDate
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[*] TimeDefuser:DriverEntry: loaded, currently SystemExpirationDate is: %llu\n", li->QuadPart));
	unsigned long long TimebombStamp = li->QuadPart; li->QuadPart = 0;
	if (!TimebombStamp) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser:DriverEntry: No timebomb found, exiting.")); return STATUS_FAILED_DRIVER_ENTRY;
	}
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: SystemExpirationDate is updated and it is now: %llu\n", li->QuadPart));

	// Get kernel base
	RTL_PROCESS_MODULES ModuleInfo = { 0 };
	status = ZwQuerySystemInformation(SystemModuleInformation, &ModuleInfo, sizeof(ModuleInfo), 0);
	unsigned long long* KernelBase = ModuleInfo.Modules[0].ImageBase;
	ULONG KernelSize = ModuleInfo.Modules[0].ImageSize;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize));
	
	// Search for timebomb stamp in memory
	BOOLEAN occurence1 = FALSE;
	void* pExpNtExpirationDate = NULL;

	KernelSize /= sizeof(unsigned long long);
	for (size_t i = 0; i < KernelSize; i++) {
		if (KernelBase[i] == TimebombStamp) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: Timebomb stamp found at 0x%p\n", &KernelBase[i]));
			KernelBase[i] = 0x7FFFFFFFFFFFFFFF;

			if (occurence1) {
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: ExpNtExpirationDate address is 0x%p\n", &KernelBase[i]));
				pExpNtExpirationDate = &KernelBase[i]; break; 
			}
			else occurence1 = TRUE;
		}
	} 

	//KernelBase += (unsigned long long)KernelBase % 4096;
	//unsigned char* PotentialTimeRef = (unsigned char*)(KernelBase + 0x500000); 
	//int KernelSize2 = KernelSize * 8 - 0x500000;
	////UNICODE_STRING us; us.Buffer = L"ExGetExpirationDate"; us.Length = 19; us.MaximumLength = 19;
	////KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "TimeDefuser:DriverEntry: ExGetExpirationDate found at 0x%p\n", MmGetSystemRoutineAddress(&us)));
	////us.Buffer = L"ExpTimeRefreshWork"; us.Length = 18; us.MaximumLength = 18;
	////KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "TimeDefuser:DriverEntry: ExpTimeRefreshWork found at 0x%p\n", MmGetSystemRoutineAddress(&us)));
	//
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "TimeDefuser:DriverEntry: Adjusted Kernel Base address is 0x%p and size is %lu\n", KernelBase, KernelSize2));

	// Search for PE headers
	const short header = 0x5a4d;
	if (*(short*)KernelBase != header) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser:DriverEntry: PE Header not found!\n"));
		goto patchFail;
	}						
	
	const size_t sectName = 0x00004b4c45474150; // "PAGELK\0\0"
	int KernelSize2 = 0;
	unsigned char* PotentialTimeRef = (unsigned char*)KernelBase;

	// Search for PAGELK section at PE sections. This section is where the 
	// ExpTimeRefreshWork function is located at, which later calls a function named "ExGetExpirationDate"
	// so we are going to use its RVA and size for finding the function location.

	for (size_t i = 0; i < 768; i++) {
		if (KernelBase[i] == sectName) { // Check if we found the PAGELK\0\0 section name.
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: PAGELK Section found at 0x%p with size %d\n",&KernelBase[i], *(int*)&KernelBase[i + 1]));
			KernelSize2 = *(int*)&KernelBase[i + 1]; // Get the section size
			// Get the function RVA and append it to kernel base address.
			int* asd = (int*)&KernelBase[i + 1]; 
			PotentialTimeRef += asd[1];
			break;
		}
	}
	if (!PotentialTimeRef) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser:DriverEntry: PAGELK Section not found!\n"));
		goto patchFail;
	}

	// Search for the ExpTimeRefreshWork function at the address we got from PAGELK.
	// Finding it is easy because it has one of only two references to expiration date address at KUSER
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: searching at 0x%p in %d bytes\n", PotentialTimeRef, KernelSize2));
	for (size_t i = 0; i < KernelSize2; i += 4096) {
		// Check if given address is valid to prevent page faults
		if (!MmIsAddressValid(&PotentialTimeRef[i])) { 
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "- TimeDefuser:DriverEntry: Page 0x%p is not valid.\n", &PotentialTimeRef[i]));
			continue;
		}
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "+ TimeDefuser:DriverEntry: Searching inside page 0x%p\n", &PotentialTimeRef[i]));

		// Search inside page for reference
		for (int k = 0; k < 4096;k++) {
			if (*(unsigned long long*)&PotentialTimeRef[i + k] == (unsigned long long)li) {
				// We found the reference of KUSER expiration date field address.
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: Potential TimeRef found at 0x%p\n", &PotentialTimeRef[i + k]));
				// The call to ExGetExpirationDate is a few instructions before this reference
				// So we search backwards for any CALL instruction (0xe8)
				for (unsigned char j = 0; j < 100; j++) {
					if (*(unsigned char*)&PotentialTimeRef[i + k - j] == 0xe8) { // CALL instruction found.
						unsigned char* pExGetExpirationDate = &PotentialTimeRef[i + k - j + 5];
						pExGetExpirationDate += *(unsigned int*)&PotentialTimeRef[i + k - j + 1]; // Next 4 bytes are relative address to our current location.
						KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: ExGetExpirationDate found at 0x%p\n", pExGetExpirationDate));
						// Create a MDL paging to get over write protection.
						PMDL mdl = IoAllocateMdl(pExGetExpirationDate, 8, FALSE, FALSE, NULL);
						if (!mdl) { 
							KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] TimeDefuser:DriverEntry: IoAllocateMdl failed.\n"));
							goto patchFail;
						}

						MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
						void* map = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
						MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);
						// Write to newly created MDL mapping.
						*(int*)map = 0xC3C03148;
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
patchFail:
	// 0xC3C03148
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[X] TimeDefuser:DriverEntry: Patch failed.\n"));
	return STATUS_FAILED_DRIVER_ENTRY;

patchOK:
	WDF_DRIVER_CONFIG_INIT(&config,
		TimeDefuserDeviceAdd
	);
	
	// Finally, create the driver object
	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		WDF_NO_HANDLE
	); 
	return STATUS_SUCCESS;
}

NTSTATUS
TimeDefuserDeviceAdd(
	_In_    WDFDRIVER       Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
{
	// We're not using the driver object,
	// so we need to mark it as unreferenced
	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;

	// Allocate the device object
	WDFDEVICE hDevice;

	// Print "Hello World" for DriverEntry
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "TimeDefuser:TimeDefuserDeviceAdd: called."));

	// Create the device object
	status = WdfDeviceCreate(&DeviceInit,
		WDF_NO_OBJECT_ATTRIBUTES,
		&hDevice
	);
	return status;
}