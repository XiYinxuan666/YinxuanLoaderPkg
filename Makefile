CC = clang
LD = lld-link

# Adjust these paths as needed
EDK2_INC = edk2_inc
EDK2_LIB = edk2_lib

# EDK2 includes
INCLUDES = -I . \
           -I $(EDK2_INC) \
           -I $(EDK2_INC)/X64 \
           -I $(EDK2_INC)/Library \
           -I $(EDK2_LIB)/BaseLib

# Compilation flags
# -target x86_64-pc-win32-coff: Generate COFF object files for UEFI
# -fno-stack-protector: Disable stack protection (UEFI doesn't support it standardly)
# -fshort-wchar: UEFI uses 16-bit wchar_t
# -mno-red-zone: UEFI ABI doesn't use red zone
CFLAGS = -target x86_64-pc-win32-coff \
         -fno-stack-protector -fshort-wchar -mno-red-zone \
         -nostdlibinc \
         $(INCLUDES) \
         -D MDE_PKG_URN= \
         -Wno-incompatible-library-redeclaration \
         -Wno-unused-function -Wno-unused-variable \
         -O2 \
         -include AutoGen.h

# Linker flags
# -subsystem:efi_application: Set subsystem to EFI Application
# -entry:EfiMain: Entry point function
LDFLAGS = -subsystem:efi_application -nodefaultlib -dll -entry:EfiMain

# Source files
# Main application
SRCS = Main.c EntryPoint.c GuidDefinitions.c

# Libraries
SRCS += $(wildcard $(EDK2_LIB)/BasePrintLib/*.c)
SRCS += $(wildcard $(EDK2_LIB)/UefiLib/*.c)
SRCS += $(wildcard $(EDK2_LIB)/UefiMemoryAllocationLib/*.c)
SRCS += $(wildcard $(EDK2_LIB)/BaseMemoryLib/*.c)

BASELIB_SRCS = $(wildcard $(EDK2_LIB)/BaseLib/*.c)
BASELIB_SRCS := $(filter-out %/UnitTestHost.c %/X86UnitTestHost.c %/X86MemoryFence.c %/LongJump.c %/SetJump.c %/SwitchStack.c %/X86DisablePaging32.c %/X86DisablePaging64.c %/X86EnablePaging32.c %/X86EnablePaging64.c %/X86RdRand.c %/X86Thunk.c, $(BASELIB_SRCS))
SRCS += $(BASELIB_SRCS)
SRCS += $(EDK2_LIB)/BaseLib/X64/GccInline.c
SRCS += $(EDK2_LIB)/BaseLib/X64/GccInlinePriv.c

SRCS += $(wildcard $(EDK2_LIB)/UefiBootServicesTableLib/*.c)

DEVICEPATH_SRCS = $(wildcard $(EDK2_LIB)/UefiDevicePathLib/*.c)
DEVICEPATH_SRCS := $(filter-out %/DevicePathUtilitiesBase.c %/UefiDevicePathLibOptionalDevicePathProtocol.c, $(DEVICEPATH_SRCS))
SRCS += $(DEVICEPATH_SRCS)
SRCS += $(wildcard $(EDK2_LIB)/BaseDebugLibNull/*.c)
SRCS += $(wildcard $(EDK2_LIB)/BasePcdLibNull/*.c)

# Filter out architecture specific files from BaseLib root wildcard if they conflict
# For now, let's see if we need specific X64 files.
# BaseLib typically needs some assembly for CPU operations.
# We might need to implement some intrinsics if they are missing.

# Objects
OBJS = $(SRCS:.c=.obj)

# Targets
all: Loader.efi

Loader.efi: $(OBJS)
	$(LD) $(LDFLAGS) -out:$@ $(OBJS)

%.obj: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) Loader.efi
