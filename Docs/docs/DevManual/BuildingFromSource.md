# 源码构建 (Building From Source)

## 前置要求 (Prerequisites)

* **操作系统**: Windows 10/11（Linux 构建说明见仓库 [README](https://github.com/HimiiFish/Himii-Engine/blob/main/README.md)）
* **Visual Studio 2022**: 安装 “使用 C++ 的桌面开发” 工作负载
* **CMake** 3.12+
* **Python 3**: 用于 `build.py`（可选）
* **.NET 8 SDK**: 编译 `ScriptCore` 与游戏脚本
* **Git**: 克隆时建议 `--recursive`（含 vcpkg 等子模块）

## 首次准备：生成本地 CMake 预设

仓库内提交的是跨平台模板 `CMakePresets.json.example`，**不包含**本机生成器（Visual Studio、Ninja 等）。每位开发者需复制为本地文件后再构建；`CMakePresets.json` 已加入 `.gitignore`，不会提交到 Git。

**Windows:**

```powershell
copy CMakePresets.json.example CMakePresets.json
```

**Linux:**

```bash
cp CMakePresets.json.example CMakePresets.json
```

若本机 CMake 默认生成器不符合预期，可在本地 `CMakePresets.json` 的 `windows-base`（或对应 preset）中自行添加，例如：

```json
"generator": "Visual Studio 17 2022"
```

或：

```json
"generator": "Ninja"
```

Linux 开发者通常复制后无需修改，直接使用 `linux-debug` / `linux-release` 即可。

## 构建步骤（推荐）

### 方式一：CMake Preset

```powershell
git clone --recursive <repository-url>
cd Himii-Engine

copy CMakePresets.json.example CMakePresets.json

cmake --preset x64-debug
cmake --build --preset build-x64-debug-win -j 8
```

Release 构建示例：

```powershell
cmake --preset x64-release
cmake --build --preset build-x64-release-win -j 8
```

### 方式二：Python 脚本

```powershell
python build.py debug
```

或双击 `build_py.bat`。

## 运行编辑器

1. 在 Visual Studio 中打开 `build/x64-debug` 下生成的解决方案，或将 **HimiiEditor** 设为启动项目。
2. 也可直接运行：

   `build/x64-debug/bin/HimiiEditor/Debug/HimiiEditor.exe`

3. 首次运行前需成功构建 **ScriptCore**（CMake 目标 `ScriptCore_Build`，构建 HimiiEditor 时会自动触发）。

## 输出目录说明

引擎通过 `himii_set_output_dirs` 统一输出到：

- 可执行文件：`${CMAKE_BINARY_DIR}/bin/<TargetName>/<Config>/`
- 静态库 Engine：`${CMAKE_BINARY_DIR}/lib/Engine/<Config>/`

示例（Windows）：

| 配置 | 编辑器 | 运行时 |
|------|--------|--------|
| Debug | `build/x64-debug/bin/HimiiEditor/Debug/HimiiEditor.exe` | `build/x64-debug/bin/HimiiRuntime/Debug/HimiiRuntime.exe` |
| Release | `build/x64-release/bin/HimiiEditor/Release/HimiiEngine.exe` | `build/x64-release/bin/HimiiRuntime/Release/HimiiRuntime.exe` |

Release 下编辑器目标在 CMake 中仍名为 `HimiiEditor`，输出文件名称为 `HimiiEngine.exe`。

### Debug 与 Release 资源布局

| 配置 | 引擎资源 | ScriptCore |
|------|----------|------------|
| Debug Editor | 松散目录 `assets/`、`resources/`（POST_BUILD 拷贝到 exe 旁） | exe 同目录 |
| Debug Runtime | 仅松散 `assets/shaders/`（字体/天空盒由游戏项目提供） | exe 同目录 |
| Release Editor | **胖** `HimiiEngine/engine.hpck`（shaders + fonts + resources） | `HimiiEngine/` 子目录 |
| Release Runtime / Templates | **瘦** `HimiiEngine/engine.hpck`（仅 shaders） | `HimiiEngine/` 子目录 |

Release 分发目录结构（Godot 式顶层入口 + Export Templates）：

```text
HimiiEngine.exe
editor_layout.ini
shader_cache/                   # 运行期可写着色器缓存（可选出现）
HimiiEngine/                    # 编辑器自身引擎数据（胖 engine.hpck）
  engine.hpck
  ScriptCore.dll
  ScriptCore.runtimeconfig.json
ExportTemplates/Windows/        # Build Project 用的玩家壳模板（瘦 engine.hpck）
  HimiiRuntime.exe
  <原生依赖 DLL...>
  HimiiEngine/
    engine.hpck
    ScriptCore.dll
    ScriptCore.runtimeconfig.json
```

`ResourcePacker` 构建两份包：`engine.editor.hpck`（胖）与 `engine.runtime.hpck`（瘦），POST_BUILD / install 按目标落盘为同名 `engine.hpck`。

`cmake --install` 会将 Release `HimiiRuntime` 安装到 `ExportTemplates/Windows/`（不再与编辑器 `HimiiEngine/` 混放）。HimiiEditor 的 **Release POST_BUILD** 也会把同套产物同步到 Editor 输出旁的 `ExportTemplates/Windows/`。

安装 Release 包：

```powershell
cmake --build --preset build-x64-release-win
cmake --install build/x64-release --prefix install/x64-release
```

安装后检查：

```text
install/x64-release/ExportTemplates/Windows/HimiiRuntime.exe
install/x64-release/ExportTemplates/Windows/HimiiEngine/engine.hpck
```

## 运行 HimiiRuntime（本地构建）

在完整构建 HimiiRuntime 目标后，可从输出目录启动：

```text
build/x64-debug/bin/HimiiRuntime/Debug/HimiiRuntime.exe
```

在 **HimiiRuntime 可执行文件所在目录**（或将其设为当前工作目录）放置 `.hproj` 文件；启动后会加载该目录下找到的第一个 `.hproj`。

运行前请确认同目录下已有 `ScriptCore.dll`、`ScriptCore.runtimeconfig.json`，以及项目所需的 `assets`（Debug 构建由 Post-build 从 `HimiiEditor` 拷贝；Release 使用 `HimiiEngine/engine.hpck`）。

## 故障排除

* **找不到 ScriptCore.dll**：先完整构建 HimiiEditor，或单独 `dotnet build ScriptCore`。
* **Build Project 找不到 Export Templates**：确认已构建 Release Editor（POST_BUILD 同步）或执行过 `cmake --install`；开发机也可依赖 `bin/HimiiRuntime/Release/` 回退路径。
* **vcpkg 依赖失败**：确认仓库内 `vcpkg` 子模块存在，并确认已从 `CMakePresets.json.example` 复制出本地 `CMakePresets.json`，然后重跑 `cmake --preset x64-debug`。
* 更多平台说明见仓库 [README](https://github.com/HimiiFish/Himii-Engine/blob/main/README.md)（含 Linux 与 `build.py` 用法）。
