# 源码构建 (Building From Source)

## 前置要求 (Prerequisites)

*   **操作系统**: Windows 10/11
*   **Visual Studio 2022**: 安装 "Desktop development with C++" 工作负载。
*   **Python 3**: 用于运行构建脚本。
*   **Git**: 用于版本控制。

## 构建步骤

1.  克隆仓库（包含子模块）：
    ```powershell
    git clone --recursive https://github.com/YourUsername/HimiiEngine.git
    cd HimiiEngine
    ```

2.  运行构建脚本生成项目文件：
    ```powershell
    ./build.py
    ```
    或者双击运行 `build_py.bat`。

3.  打开生成的 Visual Studio 解决方案（通常在 `build/` 目录或根目录下，取决于生成脚本配置）。

4.  将 `HimiiEditor` 设为启动项目。

5.  按 F5 编译并运行。
