#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

// Required by MSVC-compatible linkers for floating point support
int _fltused = 0;

EFI_RUNTIME_SERVICES *gRT = NULL;

// Stubs for RegisterFilterLib (referenced by BaseLib)
BOOLEAN EFIAPI FilterBeforeMsrRead (UINT32 Index, UINT64 *Value) { return TRUE; }
VOID EFIAPI FilterAfterMsrRead (UINT32 Index, UINT64 *Value) {}
BOOLEAN EFIAPI FilterBeforeMsrWrite (UINT32 Index, UINT64 *Value) { return TRUE; }
VOID EFIAPI FilterAfterMsrWrite (UINT32 Index, UINT64 *Value) {}

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;

    return UefiMain(ImageHandle, SystemTable);
}

// Simple implementation of compiler intrinsics to avoid linking against standard library
void *memset(void *s, int c, UINTN n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, UINTN n) {
    char *d = dest;
    const char *s = src;
    while(n--) *d++ = *s++;
    return dest;
}
