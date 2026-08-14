# 编辑器与功能

本文介绍 **HimiiEditor** 的整体能力划分，以及动画、物理、脚本等子系统的使用入口。界面布局与面板操作详见 [编辑器界面](EditorInterface.md)。

---

## 编辑器能做什么

HimiiEditor 是基于停靠布局的集成开发环境，主要能力包括：

| 能力 | 说明 | 深入阅读 |
|------|------|----------|
| **场景编辑** | 实体层级、Transform、组件、视口 Gizmo | [编辑器界面](EditorInterface.md) |
| **资源管理** | 纹理、场景、动画、脚本、Tilemap 等 | 下文「内容浏览器」 |
| **2D 逐帧动画** | 单 `.anim` 内多条命名动画、图集选帧 | [2D 逐帧动画](SpriteAnimation.md) |
| **2D 物理** | 刚体、碰撞体、Tilemap 碰撞、碰撞回调 | [2D 物理](Physics2D.md) |
| **C# 脚本** | 编译、Play 运行、Inspector 字段 | [脚本工作流](ScriptWorkflow.md)、[脚本 API](ScriptingAPI.md) |
| **运行调试** | Play / Simulate / Stop、Console 日志 | [编辑器界面](EditorInterface.md) |

建议学习路径：**[快速开始](GettingStarted.md)** → **[2D 平台跳跃教程](Tutorial2DPlatformer.md)** → 按需查阅本页各专题。

---

## 场景与实体

### 场景文件

- 场景保存为 **`assets/scenes/*.himii`**（YAML）。
- 在 **Content Browser** 双击打开；**Ctrl+S** 保存当前场景与工程。

### 实体 (Entity)

- 在 **Scene Hierarchy** 创建、重命名、组织父子关系。
- 每个实体必有 **Transform**；通过 **Add Component** 挂载渲染、物理、脚本等组件。
- 选中实体后，**Properties** 面板编辑各组件参数。

### 视口操作

- **Edit 模式**：右键 + **WASD** 漫游场景相机；滚轮缩放；**ImGuizmo** 变换选中物体。
- **Play 模式**：显示运行时画面（脚本与物理生效）。
 
> 视口 + Hierarchy + Properties 三栏同框。

---

## 内容浏览器

项目资源对应磁盘上的 **`assets/`** 目录：

| 类型 | 常见路径 | 用途 |
|------|----------|------|
| 场景 | `assets/scenes/` | `.himii` |
| 脚本 | `assets/scripts/` | `.cs` |
| 纹理 | `assets/textures/` | `.png` 等 |
| 动画 | `assets/animation/` | `.anim`（多命名逐帧序列） |
| Tilemap | `assets/tilemaps/` | `.tilemap` |

- **拖拽** 资源到 Inspector 字段（如 Animation、Texture）。
- **右键** 可创建 C# 脚本、文件夹等。
- 纹理导入后生成 **`.meta`**，修改切片等信息后需保存工程。

> **【配图占位】** `images/content-browser-assets.png` — Content Browser 典型 `assets/` 结构

---

## 2D 逐帧动画

Himii 使用 **`.anim` 资产** 组织 2D 逐帧动画：一个文件绑定一张图集，内含多条 **命名动画**（如 `Idle`、`Run`）。

### 打开 Animation Editor

- **Window → Animation Editor**
- 或双击 Content Browser 中的 `.anim`

### 核心工作流

1. 拖入图集，设置 **Cell Size**。
2. **Animations** 列表 **Add** 命名动画。
3. 在 **Atlas Picker** 按播放顺序点格子。
4. **Timeline** 调整帧顺序；设置 FPS、Loop。
5. **Save** / **Save As** 写入磁盘。

### 场景中使用

实体添加 **Sprite Renderer** + **Sprite Animation**，指定 `.anim` 与 **Current Animation**。脚本可调用 `Play("Run")` 切换。

完整说明：[2D 逐帧动画](SpriteAnimation.md)。

---

## 2D 物理

内置 **Box2D** 二维物理。

### 常用组件

| 组件 | 典型用途 |
|------|----------|
| **Rigidbody 2D** | Static（墙/地）、Dynamic（玩家）、Kinematic（平台等） |
| **Box / Circle Collider 2D** | 碰撞形状与材料（摩擦、弹性） |
| **Tilemap Collider 2D** | 瓦片地图批量静态碰撞 |

### 编辑器中的三种模式

| 模式 | 物理 | 脚本生命周期 | 碰撞脚本回调 |
|------|------|--------------|--------------|
| **Edit** | 否 | 否 | 否 |
| **Simulate** | 是 | 否 | 否 |
| **Play** | 是 | 是 | 是 |

平台跳跃教程中的地面与玩家配置见 [教程：2D 平台跳跃](Tutorial2DPlatformer.md)。

完整说明：[2D 物理](Physics2D.md)。

---

## 脚本系统

- 语言：**C#**，目标 **.NET 8**，运行时 **CoreCLR**。
- 游戏逻辑写在 `assets/scripts/`，编译为 **`GameAssembly.dll`**。
- 实体挂载 **Script** 组件，填写 **Class Name**（与 C# 类名一致）。

### 编译与日志

**Window → Console** 同时显示：

- 运行时 `Log.Info` / `Warning` / `Error`
- `dotnet build` 输出（error 行可跳转 IDE）
- 引擎 Warning / Error（Info / Trace 需勾选 **Engine Info / Trace**）

Play 只清空运行时日志，保留最近一次编译结果。

### Inspector 字段

- **public 实例字段** 或 **`[SerializeField]` private 字段**（`using HimiiEngine;`）可在属性面板编辑并写入场景。

详见 [脚本工作流](ScriptWorkflow.md)、[脚本 API](ScriptingAPI.md)。

---

## 专项编辑器窗口

| 菜单 | 功能 |
|------|------|
| **Window → Animation Editor** | 编辑 `.anim` |
| **Window → TileMap Editor** | 编辑瓦片地图与 TileSet |
| **Window → Console** | 运行时日志与脚本编译输出 |

其他窗口以编辑器当前版本菜单为准。

---

## 保存与快捷键

| 操作 | 行为 |
|------|------|
| **Ctrl+S** | 保存 `.hproj`、当前场景、已修改且已落盘的 `.anim` / `.meta` 等 |
| Animation Editor **Save As** | 首次保存新动画资产的路径 |
| **Play / Stop** | 进入或退出运行时（编辑场景在 Stop 后恢复） |

全局 **Ctrl+S** 不会为「从未 Save As 过」的空白新动画强制弹出另存为对话框。

---

## 构建项目 {#build-project}

通过 **File → Build Project...** 将当前游戏项目导出为可独立运行的 **Release 分发目录**：

1. 选择输出可执行文件路径（例如 `MyGame/MyGame.exe`）。**请使用专用输出目录**：
   - 允许：空目录；或已是完整发布包（同时含 `Game.hproj`、`HimiiEngine/engine.hpck`、`GameAssembly.dll`，可覆盖）
   - 其它非空目录会被拒绝
   - 成功时该 exe 所在目录会被整体替换为发布包；失败则尽量保留上一份完整包
2. 编辑器显示模态阶段进度（编译 → 校验 Export Template → 写出 → 提交）；期间不可操作编辑器，**无取消**。成功后会自动打开输出目录（不自动启动游戏）。
3. 玩家壳来自 **Export Templates**（优先 `编辑器目录/ExportTemplates/Windows/`）；开发机构建时若无 Templates，可回退到同构建树 `bin/HimiiRuntime/Release/`。
4. 成功后输出目录大致为：

```text
MyGame.exe
<原生依赖 DLL...>
HimiiEngine/
  engine.hpck               # 瘦包：仅运行时 shaders（不含编辑器 resources / 引擎字体）
  ScriptCore.dll
  ScriptCore.runtimeconfig.json
assets/                     # 项目资源全量（含默认 fonts/；3D 项目含 skybox/）
AssetRegistry.yaml          # AssetHandle 解析所需
GameAssembly.dll
Game.hproj                  # 发布配置：ScriptModulePath / AssetDirectory=assets；不含本机 IDE 字段
```

5. 在输出目录运行生成的 exe（工作目录为该目录）即可加载 `Game.hproj` 与 StartScene。

**引擎包分层**：编辑器安装目录下的 `HimiiEngine/engine.hpck` 为**胖包**（shaders + 编辑器字体 + icons/splash/天空盒等）；`ExportTemplates` / 玩家包中的同名文件为**瘦包**（仅 shaders）。玩法默认字体来自项目 `assets/fonts/`（新建或打开项目时自动种子拷贝）；不要把编辑器旁的 `shader_cache/` 当作发布内容。

**前置条件**：发布版 Editor 需随附 `ExportTemplates/Windows/`（含 `HimiiRuntime.exe`、旁路 DLL、**瘦** `HimiiEngine/engine.hpck` 与 ScriptCore）。源码开发机也可先构建 **Release** `HimiiRuntime` 作为回退。若缺失模板、脚本编译失败、StartScene 为绝对路径或不存在等，Build 会失败并给出原因。

> **【配图占位】** `images/build-project-menu.png` — **File → Build Project...** 菜单项

---

## 相关文档

- [编辑器界面](EditorInterface.md)：面板与工具栏细节  
- [教程：2D 平台跳跃](Tutorial2DPlatformer.md)  
- [2D 逐帧动画](SpriteAnimation.md)  
- [2D 物理](Physics2D.md)  
- [脚本 API](ScriptingAPI.md)  
