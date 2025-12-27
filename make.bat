@echo off
chcp 65001 >nul
REM 动态检测WSL用户名（避免硬编码导致找不到用户）
for /f "delims=" %%i in ('wsl whoami') do set WSLUSER=%%i
echo 检测到 WSL 用户: %WSLUSER%

REM 删除WSL下旧的YinxuanLoaderPkg并创建目标目录
wsl -u %WSLUSER% -- rm -rf /home/%WSLUSER%/edk2/YinxuanLoaderPkg
wsl -u %WSLUSER% -- mkdir -p /home/%WSLUSER%/edk2/YinxuanLoaderPkg

REM 只复制源码（排除Build目录和make.bat自身）
for /d %%d in (*) do (
    if /I not "%%d"=="Build" (
        wsl -u %WSLUSER% -- cp -r /mnt/c/Users/%USERNAME%/Desktop/YinxuanLoaderPkg/%%d /home/%WSLUSER%/edk2/YinxuanLoaderPkg/
    )
)
for %%f in (*.*) do (
    if /I not "%%f"=="make.bat" if /I not "%%f"=="Build" (
        wsl -u %WSLUSER% -- cp /mnt/c/Users/%USERNAME%/Desktop/YinxuanLoaderPkg/%%f /home/%WSLUSER%/edk2/YinxuanLoaderPkg/
    )
)

REM 使用login shell运行，确保加载与交互式相同的环境
wsl -u %WSLUSER% -- bash -lic "cd ~/edk2 && source edksetup.sh && build -a X64 -t GCC5 -p YinxuanLoaderPkg/YinxuanLoaderPkg.dsc"

REM 拷贝生成的Loader.efi回本地Build目录
if not exist Build mkdir Build
wsl -u %WSLUSER% -- cp /home/%WSLUSER%/edk2/Build/YinxuanLoaderX64/DEBUG_GCC5/X64/Loader.efi /mnt/c/Users/%USERNAME%/Desktop/YinxuanLoaderPkg/Build/Loader.efi

echo 构建完成：Loader.efi 已复制到 Build 目录。
