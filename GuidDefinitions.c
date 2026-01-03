#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DevicePath.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/DriverConfiguration2.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/HiiFont.h>
#include <Guid/EventGroup.h>
#include <Guid/PcAnsi.h>

EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;
EFI_GUID gEfiAcpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
EFI_GUID gEfiAcpi10TableGuid = ACPI_TABLE_GUID;
EFI_GUID gEfiDriverBindingProtocolGuid = EFI_DRIVER_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiDriverConfigurationProtocolGuid = EFI_DRIVER_CONFIGURATION_PROTOCOL_GUID;
EFI_GUID gEfiDriverConfiguration2ProtocolGuid = EFI_DRIVER_CONFIGURATION2_PROTOCOL_GUID;
EFI_GUID gEfiGlobalVariableGuid = EFI_GLOBAL_VARIABLE;
EFI_GUID gEfiSimpleTextOutProtocolGuid = EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiHiiFontProtocolGuid = EFI_HII_FONT_PROTOCOL_GUID;
EFI_GUID gEfiPcAnsiGuid = EFI_PC_ANSI_GUID;

EFI_GUID gEfiVT100Guid = EFI_VT_100_GUID;
EFI_GUID gEfiVT100PlusGuid = EFI_VT_100_PLUS_GUID;
EFI_GUID gEfiVTUTF8Guid = EFI_VT_UTF8_GUID;
EFI_GUID gEfiUartDevicePathGuid = DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL;
EFI_GUID gEfiSasDevicePathGuid = EFI_SAS_DEVICE_PATH_GUID;
EFI_GUID gEfiDebugPortProtocolGuid = { 0xEBA4E8D2, 0x3858, 0x41EC, { 0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0 } };
EFI_GUID gEfiVirtualDiskGuid = { 0x77AB535A, 0x45FC, 0x624B, { 0x55, 0x60, 0xF7, 0xB2, 0x81, 0xD1, 0xF9, 0x6E } };
EFI_GUID gEfiVirtualCdGuid = { 0x3D5ABD30, 0x4175, 0x87CE, { 0x6D, 0x64, 0xD2, 0xAD, 0xE5, 0x23, 0xC4, 0xBB } };
EFI_GUID gEfiPersistentVirtualDiskGuid = { 0x5CEA02C9, 0x4D07, 0x69D3, { 0x26, 0x9F, 0x44, 0x96, 0xFB, 0xE0, 0x96, 0xF9 } };
EFI_GUID gEfiPersistentVirtualCdGuid = { 0x08018188, 0x42CD, 0xBB48, { 0x10, 0x0F, 0x53, 0x87, 0xD5, 0x3D, 0xED, 0x3D } };

// Event Group GUIDs (Standard values)
EFI_GUID gEfiEventLegacyBootGuid = { 0x2a571201, 0x4966, 0x47f6, { 0x8b, 0x86, 0xf3, 0x1e, 0x41, 0xf3, 0x2f, 0x10 } };
EFI_GUID gEfiEventReadyToBootGuid = { 0x7ce88fb3, 0x4bd7, 0x4679, { 0x87, 0xa8, 0xa8, 0xd8, 0x34, 0x17, 0x0a, 0x20 } };
EFI_GUID gEfiEventAfterReadyToBootGuid = { 0x3a2a00af, 0x5358, 0x48f3, { 0x93, 0xc1, 0xa1, 0x8b, 0x87, 0x13, 0x74, 0x13 } };
