# HimiiEngine

[![猫猫访问计数器](https://starry-trace-sky-moe-counter.vercel.app/get/@HimiiEngine?theme=rule34)](#)

[![编程语言](https://img.shields.io/badge/编程语言-C++_17-blue.svg?style=for-the-badge)](#)
[![构建工具](https://img.shields.io/badge/构建工具-Cmake_>=3.8-green.svg?style=for-the-badge)](#)

This a **2D GameEngine**

## 环境部署（Windows）

1. 拉取仓库（注意包含子模块**vcpkg**）

> 关于vcpkg：一个C++的包管理工具，类似Python的pip，C#的nuget，用来管理C++项目的依赖库。

2. 运行`vcpkg/bootstrap-vcpkg.exe`下载`vcpkg.exe`（会下载到`vcpkg/`目录下）

3. 使用`vcpkg.exe`安装依赖项
打开终端，切换到`vcpkg/`目录，运行`.\vcpkg.exe install`即可
> 该过程会根据`vcpkg.json`配置的依赖安装'，依赖存放位置为`vcpkg_installed/`

4. 重载构建cmake

> [!IMPORTANT]
> 本项目遵循 [**MIT License**](https://github.com/HimiiFish/HimiiEngine/blob/main/LICENSE)

