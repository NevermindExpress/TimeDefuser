#define NTAPI __stdcall
#define NTKERNELAPI __declspec(dllexport)

typedef void* PVOID;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef long NTSTATUS;
#ifdef _M_IX86 // some weird quirk
typedef unsigned long size_t;
#endif
typedef unsigned short wchar_t;
typedef wchar_t WCHAR;

typedef enum _MODE { KernelMode, UserMode } KPROCESSOR_MODE;
typedef enum _MEMORY_CACHING_TYPE { MmNonCached, MmCached, MmWriteCombined } MEMORY_CACHING_TYPE;

NTKERNELAPI void NTAPI DbgPrintEx(int a, int b, const char* c, ...) {}

NTKERNELAPI PVOID NTAPI IoAllocateMdl(
    PVOID a,
    ULONG b,
    int c,
    int d,
    PVOID e
) {
    return 0;
}

NTKERNELAPI void NTAPI MmProbeAndLockPages(
    PVOID a,
    KPROCESSOR_MODE b,
    int c
) {
}

NTKERNELAPI PVOID NTAPI MmMapLockedPages(
    PVOID a,
    KPROCESSOR_MODE b
) {
    return 0;
}

NTKERNELAPI PVOID NTAPI MmMapLockedPagesSpecifyCache(
    PVOID a,
    KPROCESSOR_MODE b,
    MEMORY_CACHING_TYPE c,
    PVOID d,
    ULONG e,
    ULONG f
) {
    return 0;
}

NTKERNELAPI NTSTATUS NTAPI MmProtectMdlSystemAddress(
    PVOID a,
    ULONG b
) {
    return 0;
}

NTKERNELAPI void NTAPI MmUnmapLockedPages(
    PVOID a,
    PVOID b
) {
}

NTKERNELAPI void NTAPI MmUnlockPages(
    PVOID a
) {
}

NTKERNELAPI void NTAPI IoFreeMdl(
    PVOID a
) {
}

NTKERNELAPI int NTAPI MmIsAddressValid(
    PVOID a
) {
    return 0;
}

NTKERNELAPI PVOID NTAPI ExAllocatePoolWithTag(
    int a,
    ULONG b,
    ULONG c
) {
    return 0;
}

NTKERNELAPI void NTAPI ExFreePoolWithTag(
    PVOID a,
    ULONG b
) {
}

NTKERNELAPI NTSTATUS NTAPI ZwCreateKey(
    PVOID a,
    ULONG b,
    PVOID c,
    ULONG d,
    PVOID e,
    ULONG f,
    PVOID g
) {
    return 0;
}

NTKERNELAPI NTSTATUS NTAPI ZwSetValueKey(
    PVOID a,
    PVOID b,
    ULONG c,
    ULONG d,
    PVOID e,
    ULONG f
) {
    return 0;
}

NTKERNELAPI NTSTATUS NTAPI ZwQueryValueKey(
    PVOID a,
    PVOID b,
    int c,
    PVOID d,
    ULONG e,
    PVOID f
) {
    return 0;
}

NTKERNELAPI NTSTATUS NTAPI ZwClose(
    PVOID a
) {
    return 0;
}

NTKERNELAPI void NTAPI RtlInitUnicodeString(
    PVOID a,
    const WCHAR* b
) {
}

NTKERNELAPI NTSTATUS NTAPI RtlStringCchPrintfW(
    WCHAR* a,
    ULONG b,
    const WCHAR* c,
    ...
) {
    return 0;
}

NTKERNELAPI void NTAPI PsGetVersion(
    ULONG* a,
    ULONG* b,
    ULONG* c,
    ULONG* d
) {
}

NTKERNELAPI NTSTATUS NTAPI ZwQuerySystemInformation(
    int a,
    PVOID b,
    ULONG c,
    ULONG* d
) {
    return 0;
}

//NTKERNELAPI void* __cdecl memcpy(
//    void* a,
//    const void* b,
//    size_t c
//) {
//    return a;
//}
//
//NTKERNELAPI void* __cdecl memset(
//    void* a,
//    int b,
//    size_t c
//) {
//    return a;
//}

NTKERNELAPI int __cdecl swprintf(wchar_t* buffer, size_t count, const wchar_t* format, ...)
{
    return 0;
}
//NTKERNELAPI int __cdecl wcscmp(
//    const WCHAR* a,
//    const WCHAR* b
//) {
//    return 0;
//}

//NTKERNELAPI size_t __cdecl wcslen(
//    const WCHAR* a
//) {
//    return 0;
//}