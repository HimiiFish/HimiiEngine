# 第三方依赖

本文记录 HimiiEngine **源码构建**所用的第三方快照。游戏项目使用预编译 Editor / Runtime 时不必自行安装这些库。

发行用的各库 LICENSE 全文收集（`ThirdPartyNotices`）不在本文件范围内。

## 原则

- **混合分桶**：要改源码或 fork、单文件嵌入、没有可靠 vcpkg port、或与引擎版本强绑定并会打补丁 → `Engine/vender/`；其余走 vcpkg。
- **vcpkg 冻结**：根目录 `vcpkg.json` 的 `builtin-baseline` 钉死官方 port 目录；仓库内 `vcpkg` 子模块必须停在同一提交。一般包不写 `overrides`。
- **子模块**：父仓库只认已提交的 SHA。`.gitmodules` 里的 `branch`（如 imgui 的 `docking`）仅作升级时的跟踪线。禁止日常 `git submodule update --remote`。
- **升级**：升 baseline、子模块 SHA 或拷源码更新必须是专门改动，并编过 HimiiEditor；禁止在功能 PR 里顺手升级。
- **公共 API**：`glm` 视为引擎数学类型。`entt`、yaml-cpp、box2d、glfw 等不得出现在游戏或脚本会碰到的头文件里。

## vcpkg（钉死 baseline）

当前 `builtin-baseline` 与 `vcpkg` 子模块提交：

`edffab1bcd2cb5b8c17d6ba34d5651ea0bf82979`

下表版本取自该提交下各 port 的 `vcpkg.json`（升级 baseline 后必须改本表）。

| 库 | 版本 | 许可证 | 备注 |
|---|---|---|---|
| glad | 0.1.36 | MIT | OpenGL 加载器 |
| glfw3 | 3.4 | Zlib | 窗口与输入 |
| spdlog | 1.16.0 | MIT | 日志 |
| glm | 1.0.2 | MIT | 公共数学 ABI |
| stb | 2024-07-29 | MIT 或 CC-PDDC | 头文件库集合 |
| yaml-cpp | 0.8.0 | MIT | 场景 / 资产序列化 |
| entt | 3.16.0 | MIT | ECS，仅引擎实现 |
| vulkan | 2023-12-17（stub） | 见 Vulkan SDK / headers | Windows / macOS |
| shaderc | 2025.2 | Apache-2.0 | 着色器编译 |
| spirv-cross | 1.4.309.0 | Apache-2.0 | SPIR-V 反射 / 反编译 |
| spirv-tools | 1.4.309.0 | Apache-2.0 | SPIR-V 工具 |
| box2d | 3.1.1 | MIT | 2D 物理 |
| nethost | 8.0.3 | MIT | .NET 宿主 |
| freetype | 2.13.3 | FTL 或 GPL-2.0-or-later | 字体光栅 |

间接依赖（fmt、zlib 等）由 vcpkg 按同一 baseline 解析，不在本表逐条列出。

## `Engine/vender`

### 子模块（钉 SHA，要跟上游或 fork）

| 库 | 版本 / 标签 | 提交 | 许可证 | Fork |
|---|---|---|---|---|
| imgui | v1.92.1-docking | `44aa9a4b3a6f27d09a4eb5770d095cbd376dfc4b` | MIT | 是（`HimiiFish/imgui`，docking） |
| ImGuizmo | （上游 master 快照） | `71f14292205c3317122b39627ed98efce137086a` | MIT | 是（`HimiiFish/ImGuizmo`） |
| nativefiledialog-extended | v1.3.0 | `fc168e8605bfa51aaec22ab0c4e46b9de665a437` | Zlib | 否 |
| msdf-atlas-gen | v1.4 | `2ede254314a2512252a225fa6c975948d6af559a` | MIT | 否 |

### 拷贝源码（单文件 / 几乎不升级）

| 库 | 版本 | 许可证 | 路径 |
|---|---|---|---|
| ufbx | 0.23.0 | MIT | `Engine/vender/ufbx/` |
| cgltf | 1.14 | MIT | `Engine/vender/cgltf/` |
| miniaudio | 0.11.22 | 公有领域或 MIT-0 | `Engine/vender/miniaudio/`（含其自带的 `stb_vorbis.c`） |

## 新增库时

1. 用上文「原则」判断进 `vender` 还是 vcpkg。
2. 进 vcpkg：只改 `vcpkg.json` 依赖名，**不要**顺手改 `builtin-baseline`。
3. 进 `vender`：子模块钉 SHA，或拷源码并在本表写明版本。
4. 更新本页表格。
