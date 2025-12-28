#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

//MemoryMap结构体定义
// #@@range_begin(struct_memory_map)
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};
// #@@range_end(struct_memory_map)

//获取内存映射函数
// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
      &map->map_size,
      (EFI_MEMORY_DESCRIPTOR*)map->buffer,
      &map->map_key,
      &map->descriptor_size,
      &map->descriptor_version);
}
// #@@range_end(get_memory_map)

//获取内存类型对应的字符串函数
// #@@range_begin(get_memory_type)
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}
// #@@range_end(get_memory_type)

//保存内存映射到文件函数
// #@@range_begin(save_memory_map)
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
      map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    len = AsciiSPrint(
        buf, sizeof(buf),
        "%u, %x, %-ls, %08lx, %lx, %lx\n",
        i, desc->Type, GetMemoryTypeUnicode(desc->Type),
        desc->PhysicalStart, desc->NumberOfPages,
        desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}
// #@@range_end(save_memory_map)

//打开根目录函数
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  Print(L"YinxuanLoader:Wellcome to YinxuanLoader's World!\n");
  Print(L"This UEFI BIOS boot was created by Xi Yinxuan, a student at No.1 High School in Juancheng County, Heze City, Shandong Province, China, and he is just a 14-year-old middle school student!\n");
  Print(L"Preparing to save memory map...\n");
  // 动态分配内存映射缓冲区
  UINTN memmap_buf_size = 4096 * 16;
  VOID* memmap_buf = NULL;
  EFI_STATUS status;
  while (1) {
    // 动态分配内存映射缓冲区
    status = gBS->AllocatePool(EfiLoaderData, memmap_buf_size, &memmap_buf);
    if (EFI_ERROR(status)) {
      Print(L"分配内存映射缓冲区失败\n");
      return status;
    }
    struct MemoryMap memmap = {memmap_buf_size, memmap_buf, 0, 0, 0, 0};
    status = GetMemoryMap(&memmap);
    if (status == EFI_BUFFER_TOO_SMALL) {
      gBS->FreePool(memmap_buf);
      memmap_buf_size *= 2;
      continue;
    } else if (EFI_ERROR(status)) {
      Print(L"获取内存映射失败: %r\n", status);
      gBS->FreePool(memmap_buf);
      return status;
    }
    // 打开根目录，并创建memmap.csv文件
    EFI_FILE_PROTOCOL* root_dir;
    OpenRootDir(image_handle, &root_dir);

    EFI_FILE_PROTOCOL* memmap_file;
    root_dir->Open(
        root_dir, &memmap_file, L"\\memmap.csv",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    SaveMemoryMap(&memmap, memmap_file);
    memmap_file->Close(memmap_file);

    // ===== 新增功能：加载kernel.elf并跳转 =====
    // 1. 打开kernel.elf文件
    EFI_FILE_PROTOCOL* kernel_file;
    status = root_dir->Open(
        root_dir, &kernel_file, L"\\kernel.elf",
        EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
      Print(L"未找到kernel.elf，跳过内核加载。\n");
      gBS->FreePool(memmap_buf);
      break;
    }

    // 2. 获取kernel.elf文件大小
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    status = kernel_file->GetInfo(
        kernel_file, &gEfiFileInfoGuid,
        &file_info_size, file_info_buffer);
    if (EFI_ERROR(status)) {
      Print(L"获取kernel.elf文件信息失败: %r\n", status);
      kernel_file->Close(kernel_file);
      gBS->FreePool(memmap_buf);
      break;
    }
    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    // 3. 分配内存并读取kernel.elf内容
    EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000; // 1MB
    status = gBS->AllocatePages(
        AllocateAddress, EfiLoaderData,
        (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
    if (EFI_ERROR(status)) {
      Print(L"分配内存给kernel.elf失败: %r\n", status);
      kernel_file->Close(kernel_file);
      gBS->FreePool(memmap_buf);
      break;
    }
    status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
    if (EFI_ERROR(status)) {
      Print(L"读取kernel.elf失败: %r\n", status);
      kernel_file->Close(kernel_file);
      gBS->FreePages(kernel_base_addr, (kernel_file_size + 0xfff) / 0x1000);
      gBS->FreePool(memmap_buf);
      break;
    }
    Print(L"内核已加载: 地址=0x%0lx, 大小=%lu字节\n", kernel_base_addr, kernel_file_size);
    kernel_file->Close(kernel_file);

    // 4. 退出BootServices，重试机制
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"第一次ExitBootServices失败，尝试刷新内存映射并重试...\n");
      status = GetMemoryMap(&memmap);
      if (EFI_ERROR(status)) {
        Print(L"刷新内存映射失败: %r\n", status);
        gBS->FreePages(kernel_base_addr, (kernel_file_size + 0xfff) / 0x1000);
        gBS->FreePool(memmap_buf);
        break;
      }
      status = gBS->ExitBootServices(image_handle, memmap.map_key);
      if (EFI_ERROR(status)) {
        Print(L"再次ExitBootServices失败: %r\n", status);
        gBS->FreePages(kernel_base_addr, (kernel_file_size + 0xfff) / 0x1000);
        gBS->FreePool(memmap_buf);
        break;
      }
    }

    // 5. 跳转到kernel.elf入口地址
    UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24); // ELF64 header e_entry
    Print(L"跳转到内核入口: 0x%0lx\n", entry_addr);
    typedef void EntryPointType(void);
    EntryPointType* entry_point = (EntryPointType*)entry_addr;
    entry_point();

    // 理论上不会返回，若返回则释放内存
    gBS->FreePages(kernel_base_addr, (kernel_file_size + 0xfff) / 0x1000);
    gBS->FreePool(memmap_buf);
    break;
  }

  Print(L"YinxuanLoader: All done\n");

  while (1);
  return EFI_SUCCESS;
}
