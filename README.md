
# YinxuanLoaderPkg

YinxuanLoaderPkg 是一个基于 UEFI 的操作系统引导程序，由菏泽鄄城县一中初中生席胤轩开发。该项目可用于学习 UEFI 编程、内存映射保存等底层开发技术。

## 功能简介

- 以 UEFI 应用程序形式启动
- 获取并保存内存映射信息到 memmap 文件
- 控制台输出欢迎信息和开发者信息
- 适合 UEFI/OSLoader 学习与二次开发

## 主要文件说明

- Main.c：主程序，负责内存映射获取与保存、欢迎信息输出等
- Loader.inf：UEFI 应用描述文件
- YinxuanLoaderPkg.dsc / .dec：EDK2 平台描述与包描述文件
- make.bat：自动化编译与同步脚本（适用于 Windows+WSL 环境）

## 依赖环境

- Windows 10/11 + WSL (推荐 Ubuntu)
- EDK2 完整源码（需提前 clone 并编译 BaseTools）
- GCC5 工具链（WSL 内）

## 编译与使用方法

### 1. WSL 环境准备

```sh
git clone https://github.com/tianocore/edk2.git ~/edk2
cd ~/edk2
make -C BaseTools
```

### 2. Windows 下自动同步与编译

直接运行 make.bat 即可：

```bat
make.bat
```

脚本会自动：
- 检测当前 WSL 用户
- 同步 YinxuanLoaderPkg 源码到 WSL 的 ~/edk2 目录下（排除 Build 目录）
- 在 WSL 下 source edksetup.sh 并自动 build
- 将生成的 Loader.efi 复制回 Windows 的 Build 目录

### 3. 手动编译（可选）

如需手动编译，可在 WSL 下执行：

```sh
cd ~/edk2
source edksetup.sh
build -a X64 -t GCC5 -p YinxuanLoaderPkg/YinxuanLoaderPkg.dsc
```

生成的 Loader.efi 位于：
~/edk2/Build/YinxuanLoaderX64/DEBUG_GCC5/X64/Loader.efi

## 许可证

本项目采用 Apache License 2.0 开源协议，详见 LICENSE 文件。
