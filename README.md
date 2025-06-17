# HimiiEngine

[![猫猫访问计数器](https://starry-trace-sky-moe-counter.vercel.app/get/@HimiiEngine?theme=rule34)](#)

[![编程语言](https://img.shields.io/badge/编程语言-C++_17-blue.svg?style=for-the-badge)](#)
[![构建工具](https://img.shields.io/badge/构建工具-Cmake_>=3.8-green.svg?style=for-the-badge)](#)

This a **2D GameEngine**

> [!IMPORTANT]
> 本项目遵循 [**MIT License**](https://github.com/HimiiFish/HimiiEngine/blob/main/LICENSE)

## HimiiEngine 构建指南

本项目提供了跨平台的Python构建脚本 `build.py`，支持Windows和Linux平台。

## 前置要求

### Windows
- CMake 3.8 或更高版本
- Visual Studio 2022 (或更新版本)
- Python 3.6 或更高版本
- vcpkg (已包含在项目中)

### Linux
- CMake 3.8 或更高版本
- Ninja 构建系统
- GCC 或 Clang 编译器 (支持C++17)
- Python 3.6 或更高版本
- vcpkg (已包含在项目中)

## 使用方法

### 基本构建命令

```bash
# Debug 构建
python build.py debug

# Release 构建
python build.py release
```

### 高级选项

```bash
# 清理构建目录后重新构建
python build.py debug --clean

# 构建完成后自动运行程序
python build.py release --run

# 同时使用清理和自动运行
python build.py debug --clean --run

# 仅清理构建目录（不构建）
python build.py debug --clean-only
```

### 命令行参数说明

- `build_type`: 构建类型，可选 `debug` 或 `release`
- `--clean, -c`: 构建前清理构建目录
- `--run, -r`: 构建完成后自动运行生成的程序
- `--clean-only`: 仅清理构建目录，不进行构建
- `--help, -h`: 显示帮助信息

## 构建输出

构建生成的文件位于：

- Windows: `build/x64-debug/` 或 `build/x64-release/`
- Linux: `build/linux-debug/` 或 `build/linux-release/`

可执行文件位于以下位置：

- Windows: `Engine/Debug/` 或 `Engine/Release/` 子目录中，文件名为 `Engine.exe`
- Linux: `Engine/` 目录中，文件名为 `Engine`

### Windows平台可执行文件位置说明

在Windows平台上，Visual Studio生成器会根据构建类型将可执行文件放在不同的子目录中：

- **Debug构建**: 可执行文件位于 `build/x64-debug/Engine/Debug/` 目录
- **Release构建**: 可执行文件位于 `build/x64-release/Engine/Release/` 目录

这是Visual Studio的默认行为，构建脚本会自动查找正确的目录。

## 故障排除

### 常见问题

1. **CMake未找到**
   - 确保CMake已安装并添加到系统PATH中

2. **Visual Studio未找到** (Windows)
   - 确保安装了Visual Studio 2022或更新版本
   - 确保安装了C++开发工具

3. **Ninja未找到** (Linux)
   - Ubuntu/Debian: `sudo apt install ninja-build`
   - CentOS/RHEL: `sudo yum install ninja-build`
   - Arch: `sudo pacman -S ninja`

4. **vcpkg依赖问题**
   - 确保vcpkg目录存在于项目根目录
   - 检查vcpkg是否已正确初始化

### 手动构建（备选方案）

如果Python脚本出现问题，可以使用传统的CMake命令：

#### Windows
```bash
# 配置
cmake --preset x64-debug

# 构建
cmake --build --preset build-x64-debug-win
```

#### Linux
```bash
# 配置
cmake --preset linux-debug

# 构建
cmake --build --preset build-linux-debug

