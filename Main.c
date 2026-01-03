#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>
#include  <Protocol/GraphicsOutput.h>
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

// 打开 GOP 图形输出协议
EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;
  
  gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &num_gop_handles,
      &gop_handles);

  if (num_gop_handles == 0) {
    return EFI_UNSUPPORTED;
  }

  gBS->OpenProtocol(
      gop_handles[0],
      &gEfiGraphicsOutputProtocolGuid,
      (VOID**)gop,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->FreePool(gop_handles);

  return EFI_SUCCESS;
}

// 绘制矩形函数
void DrawRect(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop,
              UINT32 x, UINT32 y, UINT32 w, UINT32 h,
              EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = gop->Mode->Info;

  // 如果是 PixelBltOnly 或 PixelBitMask，强制使用 Blt 函数
  if (info->PixelFormat == PixelBltOnly ||
      info->PixelFormat == PixelBitMask) {
    gop->Blt(gop, &color, EfiBltVideoFill, 0, 0, x, y, w, h, 0);
    return;
  }

  // 对于线性帧缓冲模型（RGB/BGR），直接写内存
  UINT32 pixel_size = 4; // 32-bit color
  UINT8* base = (UINT8*)gop->Mode->FrameBufferBase;
  
  for (UINT32 dy = 0; dy < h; ++dy) {
    for (UINT32 dx = 0; dx < w; ++dx) {
      UINT32 index = (y + dy) * info->PixelsPerScanLine + (x + dx);
      UINT8* p = base + (index * pixel_size);
      
      if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
        p[0] = color.Red;
        p[1] = color.Green;
        p[2] = color.Blue;
        p[3] = color.Reserved;
      } else if (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        p[0] = color.Blue;
        p[1] = color.Green;
        p[2] = color.Red;
        p[3] = color.Reserved;
      }
    }
  }
}

// 辅助函数：创建颜色
EFI_GRAPHICS_OUTPUT_BLT_PIXEL MakeColor(UINT8 r, UINT8 g, UINT8 b) {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL c;
  c.Red = r;
  c.Green = g;
  c.Blue = b;
  c.Reserved = 0;
  return c;
}

// ---------------------------------------------------------
// 数学与动画辅助函数
// ---------------------------------------------------------
#define PI 3.14159265359

// 简单的正弦函数近似 (泰勒展开)
double Sin(double x) {
  // 将 x 归约到 -PI ~ PI
  while (x > PI) x -= 2 * PI;
  while (x < -PI) x += 2 * PI;
  
  double res = 0;
  double term = x;
  double x2 = x * x;
  
  res += term;
  term *= -x2 / (2 * 3);
  res += term;
  term *= -x2 / (4 * 5);
  res += term;
  term *= -x2 / (6 * 7);
  res += term;
  
  return res;
}

double Cos(double x) {
  return Sin(x + PI / 2);
}

// 绘制实心圆 (使用 Blt 填充小矩形模拟，效率一般但兼容性好)
void DrawCircleFilled(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop,
                      int cx, int cy, int r,
                      EFI_GRAPHICS_OUTPUT_BLT_PIXEL color) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if (x * x + y * y <= r * r) {
        gop->Blt(gop, &color, EfiBltVideoFill, 0, 0, cx + x, cy + y, 1, 1, 0);
      }
    }
  }
}

// 8x16 高分辨率字体数据
// Y i n x u a n S t d o
UINT16 font_data[][16] = {
  // Y (0)
  {
    0x0000, 0x0000, 0xC300, 0x6600, 0x3C00, 0x1800, 0x1800, 0x1800, 
    0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x0000, 0x0000, 0x0000
  },
  // i (1)
  {
    0x0000, 0x1800, 0x1800, 0x0000, 0x1800, 0x1800, 0x1800, 0x1800, 
    0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x0000, 0x0000, 0x0000
  },
  // n (2)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0xDC00, 0x6600, 0x6600, 0x6600, 
    0x6600, 0x6600, 0x6600, 0x6600, 0x6600, 0x0000, 0x0000, 0x0000
  },
  // x (3)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0xC300, 0x6600, 0x3C00, 0x1800, 
    0x3C00, 0x6600, 0xC300, 0xC300, 0xC300, 0x0000, 0x0000, 0x0000
  },
  // u (4)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0x6600, 0x6600, 0x6600, 0x6600, 
    0x6600, 0x6600, 0x6600, 0x6600, 0x3E00, 0x0000, 0x0000, 0x0000
  },
  // a (5)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0x3C00, 0x0600, 0x3E00, 0x6600, 
    0x6600, 0x6600, 0x6600, 0x6600, 0x3E00, 0x0000, 0x0000, 0x0000
  },
  // S (6)
  {
    0x0000, 0x0000, 0x3C00, 0x6600, 0x6000, 0x3000, 0x1800, 0x0C00, 
    0x0600, 0x0600, 0x6600, 0x3C00, 0x0000, 0x0000, 0x0000, 0x0000
  },
  // t (7)
  {
    0x0000, 0x0000, 0x1800, 0x1800, 0x7E00, 0x1800, 0x1800, 0x1800, 
    0x1800, 0x1800, 0x1800, 0x1800, 0x0E00, 0x0000, 0x0000, 0x0000
  },
  // d (8)
  {
    0x0000, 0x0000, 0x0600, 0x0600, 0x0600, 0x3E00, 0x6600, 0x6600, 
    0x6600, 0x6600, 0x6600, 0x6600, 0x3E00, 0x0000, 0x0000, 0x0000
  },
  // o (9)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0x3C00, 0x6600, 0x6600, 0x6600, 
    0x6600, 0x6600, 0x6600, 0x6600, 0x3C00, 0x0000, 0x0000, 0x0000
  },
  // space (10)
  {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
  }
};

// 字符串映射表: "Yinxuan Studio"
// Y(0) i(1) n(2) x(3) u(4) a(5) n(2) [sp] S(6) t(7) u(4) d(8) i(1) o(9)
int title_indices[] = {0, 1, 2, 3, 4, 5, 2, 10, 6, 7, 4, 8, 1, 9};
int title_len = 14;

void DrawTitle(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop, int cx, int cy) {
  int pixel_scale = 6; // 每个像素放大 6 倍
  int char_width = 8;  // 原始字体宽度
  int char_height = 16; // 原始字体高度
  
  // 计算总宽度
  int total_width = title_len * char_width * pixel_scale;
  
  int start_x = cx - total_width / 2;
  int current_x = start_x;
  
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = {255, 255, 255, 0};
  
  for (int k = 0; k < title_len; k++) {
    int idx = title_indices[k];
    for (int row = 0; row < char_height; row++) {
      UINT16 row_data = font_data[idx][row];
      // 8x16 字体数据通常是左对齐的，我们假设使用高 8 位或整个 16 位
      // 这里数据是 0xC300 -> 1100 0011 0000 0000
      // 我们只取高 8 位作为 8 宽度的字模
      for (int col = 0; col < char_width; col++) {
        // 检查第 (15-col) 位是否为 1 (从左到右)
        if ((row_data >> (15 - col)) & 0x01) {
          DrawRect(gop, 
            current_x + col * pixel_scale, 
            cy + row * pixel_scale, 
            pixel_scale, pixel_scale, white);
        }
      }
    }
    current_x += char_width * pixel_scale;
  }
}

// 绘制桌面背景 (新版)
void DrawDesktop(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop) {
  UINT32 width = gop->Mode->Info->HorizontalResolution;
  UINT32 height = gop->Mode->Info->VerticalResolution;

  // 1. 绘制深红色背景
  // Crimson: RGB(220, 20, 60)
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL bg_color = {60, 20, 220, 0}; // Blue, Green, Red
  DrawRect(gop, 0, 0, width, height, bg_color);

  // 2. 绘制标题 "Yinxuan Studio"
  DrawTitle(gop, width / 2, height / 2 - 50);

  // 3. 转圈动画
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = {255, 255, 255, 0};
  int center_x = width / 2;
  int center_y = height / 2 + 100;
  int radius = 30;
  int num_dots = 8;
  double angle_step = 2 * PI / num_dots;
  
  // 动画循环 (持续约 5 秒)
  // 假设 Stall(50000) = 50ms, 循环 100 次
  for (int frame = 0; frame < 100; frame++) {
    // 清除上一帧 (重绘背景覆盖动画区域)
    DrawRect(gop, center_x - radius - 10, center_y - radius - 10, 
             (radius + 10) * 2, (radius + 10) * 2, bg_color);
    
    // 绘制加载圈
    for (int i = 0; i < num_dots; i++) {
      // 动态角度：基础角度 + 旋转偏移
      double current_angle = i * angle_step + frame * 0.3; // 0.3 是旋转速度
      
      int dx = (int)(radius * Cos(current_angle));
      int dy = (int)(radius * Sin(current_angle));
      
      // 计算大小渐变 (模拟拖尾效果)
      // 使用简单的相位差来决定点的大小
      int dot_r = 3;
      // 简单的视觉效果：让点的大小随位置变化
      
      DrawCircleFilled(gop, center_x + dx, center_y + dy, dot_r, white);
    }
    
    gBS->Stall(50000); // 50ms 延迟
  }
}


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  OpenGOP(image_handle, &gop);
  DrawDesktop(gop);

  Print(L"YinxuanLoader:Wellcome to YinxuanLoader's World!\n");
  Print(L"This UEFI BIOS boot was created by Xi Yinxuan, a student at No.1 High School in Juancheng County, Heze City, Shandong Province, China, and he is just a 14-year-old middle school student!\n");
  Print(L"YinxuanLoader:Preparing to save memory map...\n");
  // 动态分配内存映射缓冲区
  UINTN memmap_buf_size = 4096 * 16;
  VOID* memmap_buf = NULL;
  EFI_STATUS status;
  struct MemoryMap memmap;
  EFI_FILE_PROTOCOL* root_dir = NULL;
  while (1) {
    status = gBS->AllocatePool(EfiLoaderData, memmap_buf_size, &memmap_buf);
    if (EFI_ERROR(status)) {
      Print(L"Failed to allocate memory for memmap_buf\n");
      return status;
    }
    memmap.buffer_size = memmap_buf_size;
    memmap.buffer = memmap_buf;
    memmap.map_size = 0;
    memmap.map_key = 0;
    memmap.descriptor_size = 0;
    memmap.descriptor_version = 0;
    status = GetMemoryMap(&memmap);
    if (status == EFI_BUFFER_TOO_SMALL) {
      if (memmap_buf) gBS->FreePool(memmap_buf);
      memmap_buf_size *= 2;
      continue;
    } else if (EFI_ERROR(status)) {
      Print(L"GetMemoryMap failed: %r\n", status);
      if (memmap_buf) gBS->FreePool(memmap_buf);
      return status;
    }
    //打开根目录，并创建memmap文件
    OpenRootDir(image_handle, &root_dir);

    EFI_FILE_PROTOCOL* memmap_file;
    root_dir->Open(
      root_dir, &memmap_file, L"\\memmap.csv",
      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    SaveMemoryMap(&memmap, memmap_file);
    memmap_file->Close(memmap_file);

    // Don't free memmap_buf here as we need it for ExitBootServices retry
    // gBS->FreePool(memmap_buf); 
    break;
  }
  Print(L"YinxuanLoader: Memory map saved successfully.\n");

  // 绘制图形界面 (在读取内核之前)
  // gop already initialized above
  if (OpenGOP(image_handle, &gop) == EFI_SUCCESS) {
    DrawDesktop(gop);
    // 可选：在界面上打印一些信息
    // Print(L"Graphics initialized. Loading kernel...\n");
  } else {
    Print(L"Failed to open GOP. Continuing in text mode.\n");
  }

  // #@@range_begin(read_kernel)

    EFI_FILE_PROTOCOL* kernel_file;
    status = root_dir->Open(
      root_dir, &kernel_file, L"\\kernel.elf",
      EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
      Print(L"Failed to open kernel.elf: %r\n", status);
      while (1);
    }

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12];
    status = kernel_file->GetInfo(
      kernel_file, &gEfiFileInfoGuid,
      &file_info_size, file_info_buffer);
    if (EFI_ERROR(status)) {
      Print(L"Failed to get kernel info: %r\n", status);
      while (1);
    }

    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x200000;
  status = gBS->AllocatePages(
      AllocateAddress, EfiLoaderData,
      (kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);
  if (EFI_ERROR(status)) {
    Print(L"Failed to allocate pages for kernel: %r\n", status);
    while (1);
  }

  status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  if (EFI_ERROR(status)) {
    Print(L"Failed to read kernel file: %r\n", status);
    while (1);
  }
  Print(L"Kernel Loaded at 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);
  // #@@range_end(read_kernel)

  // #@@range_begin(exit_bs)
  // EFI_STATUS status; // 已在前面声明
  status = gBS->ExitBootServices(image_handle, memmap.map_key);
  if (EFI_ERROR(status)) {
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"YinxuanLoader:failed to get memory map: %r\n", status);
      while (1);
    }
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"YinxuanLoader:Could not exit boot service: %r\n", status);
      while (1);
    }
  }
  // #@@range_end(exit_bs)

  // #@@range_begin(call_kernel)
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  entry_point();
  // #@@range_end(call_kernel)

  Print(L"YinxuanLoader: All done\n");

  while (1);
  return EFI_SUCCESS;
}
