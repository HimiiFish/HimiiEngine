# 架构概览（引擎与编辑器）

本文档面向 **引擎源码贡献者**，描述 HimiiEngine 的目标分层、`Engine/src/` 目录地图、Application / Module / World / Resource / Project 边界，以及与编辑器、运行时的关系。

用户侧入门请看 [快速开始](../UserManual/GettingStarted.md)。构建步骤见 [源码构建](BuildingFromSource.md)。帧循环、脚本宿主、日志与图形速查见 [核心系统](#延伸阅读)。

---

## 1) 技术栈

| 类别 | 选型 |
|------|------|
| 语言 / 构建 | C++17、CMake、vcpkg 清单模式 |
| 渲染 | OpenGL（GLAD）、GLM；经 `RendererAPI` 等抽象，平台实现在 `Platform/**` |
| 窗口 / 输入 | 抽象 `Window` / `Input`；Windows 等后端在 `Platform/**`（如 GLFW） |
| UI | Dear ImGui（Docking）、ImGuizmo；ImGui 平台后端在 `Platform/ImGui/` |
| 物理 2D | Box2D v3（`Module/Physics`） |
| 脚本 | .NET 8、CoreCLR、`hostfxr`、可收集 ALC（`Module/Script` + `ScriptCore`） |
| 序列化 | yaml-cpp；场景见 `SceneSerializer`，资产见 `AssetSerializerRegistry` |
| 日志 | spdlog + 编辑器 `ConsoleLog` 缓冲 |

---

## 2) 目标分层（逻辑）

```text
Game / Gameplay
Editor / Tools
World / ECS          ← 项目运行时会话 + Scene/Level
Module 子系统        ← Render | Physics | Audio | Animation …
Resource             ← 句柄、引用计数、加载调度、序列化分发
HimiiEngine 内核     ← Application、事件、数学、平台抽象入口
Platform Abstraction
```

仓库顶层：

```text
Himii-Engine/
├── Engine/           # 静态库（EngineCore / Module / World / Resource / Project / Platform）
├── HimiiEditor/      # 编辑器可执行程序
├── HimiiRuntime/     # 无编辑器 UI 的运行时启动器
├── ScriptCore/       # C# 脚本宿主 API
├── Tools/            # 构建期工具（如 ResourcePacker）
├── cmake/            # Post-build staging
├── Docs/docs/        # 用户手册 + 开发手册（发布到文档站点）
└── build/            # CMake 生成目录（本地，不提交）
```

命名空间目前仍为 `Himii`。对外 include 根为 `EngineCore/**`、`Module/**`、`Resource/**`、`World/**`、`Project/**`。

---

## 3) 目标目录（`Engine/src/`）

| 目录 | 职责 |
|------|------|
| `EngineCore/` | 内核：Application、Core、Events、Math、ImGui、基础工具；不含磁盘工程描述 |
| `Module/` | 功能子系统（Audio、Render、Physics、Animation、Particle、Script、Tilemap、UserInterface、Resource 模块入口等） |
| `World/` | World 会话、世界级模块表与阶段调度；`World/Scene/` 为关卡切片（实体、组件、场景序列化） |
| `Resource/` | 资源门面、句柄与 AssetManager、序列化器注册表 |
| `Project/` | 磁盘工程描述（`.hproj`、路径与层设置等），与 World 分目录、分寿命 |
| `Platform/` | 窗口、时钟、进程、ImGui 后端等平台实现 |

约定：

- 高阶渲染（Renderer2D/3D、批处理、`SceneRenderer` 等）归 **Module/Render**，不堆在内核。
- 领域资产类型与 **具体序列化逻辑** 归各 Module（或 World 的 Scene/Prefab）；Resource 做公共管线与按类型/扩展名分发。
- 平台专有 API 下沉到 `Platform/**`，上层（Editor / Scene / ScriptGlue）不散落裸调用。

---

## 4) Application

### 职责

- 程序生命周期：窗口创建、Layer 栈、事件冒泡、主循环。
- 持有 **Application 级** `ModuleRegistry`，对已注册 `IModule` 做 Init / Update / Shutdown。
- 持有 **当前 World** 句柄（可热替换）；不把物理等世界系统挂在 Application 上。

### 明确不负责

- 不知具体图形/窗口 API 细节（清屏、GLFW 直调等应在模块或 Platform）。
- 不注册、不拥有 Project 业务细节。
- 不直接 Init 各领域「单例式上帝启动器」。

### 当前 Application 级模块（注册次序）

1. `ResourceModule` — 注册内置资产序列化器；Shutdown 时清理并 Unbind  
2. `RenderModule` — 图形初始化（需在窗口创建之后）  
3. `AudioModule`  
4. `ScriptModule` — CoreCLR / 脚本宿主寿命  

实现见 `Engine/src/EngineCore/Core/Application.cpp`。

---

## 5) 两级模块：IModule 与 IWorldModule

| 级别 | 接口 / 注册表 | 例子 | 谁 Tick |
|------|----------------|------|---------|
| Application 级 | `IModule` + `ModuleRegistry` | Resource、Render、Audio、Script | Application |
| World 级 | `IWorldModule` + `WorldModuleRegistry` | Physics2D、UI、动画、粒子、场景绘制、脚本 Update/FixedUpdate | World（由调用方按阶段显式驱动） |

### 更新顺序（World）

- **禁止**用注册先后或 priority 数字决定跨阶段时序。
- 调用方通过 `Update(WorldUpdatePhase)` **枚举显式驱动**；跨阶段顺序只认调用序列。
- 同阶段内多模块保留稳定注册次序。

当前阶段枚举（`World/WorldUpdatePhase.h`）：

`UserInterface` → `ScriptUpdate` → `Animation` → `Physics` → `ScriptFixedUpdate` → `Presentation` → `Render`

Runtime 帧由 `World::OnUpdateRuntime` 按上述顺序调用；Simulation 帧由 `World::OnUpdateSimulation` 使用子集（物理 → 动画 → ScriptFixedUpdate → Render）。细节见 [编辑器运行时](CoreSystems/EditorRuntime.md)。

### 换 Scene 与模块重建

`World::SetActiveScene` 在活动场景变化时会 `TearDownModules` 再 `BuildModulesForActiveScene`：世界模块绑定具体 `Scene*`，因此每次打开/新建/Play 复制场景都会重新注册一轮 World 模块（日志中出现两次「registered」通常是启动空场景 + 随后加载项目场景，属预期行为）。

---

## 6) World 与 Scene

| 概念 | 含义 |
|------|------|
| **World** | 某个已打开 Project 的 **运行时会话**：世界模块表、阶段调度、当前活动 Scene |
| **Scene / Level** | 可加载、可切换的场景切片；主要承载实体与关卡数据（`entt` + 组件） |

换 Scene 时重置/重建与关卡绑定的世界模块状态（如物理世界），而不必拆掉整个 Application。

Scene 侧保留实体 API、视口尺寸、Runtime/Simulation 起停入口；世界绘制由 `SceneRenderer` 承担，经 `SceneRenderModule` 挂在 `WorldUpdatePhase::Render`。UI 推进经 `UserInterfaceModule`。

---

## 7) Project

- Project 是 **持久化工程描述**（名称、资产目录、Start Scene、2D/3D 等配置）。
- 与 World **分目录、分寿命**：编辑器可在未 Play 时编辑 Project；打开工程时读取并绑定资源。
- **不**把 Project 元数据当作普通 Resource 资产（除非将来单独立项）。

---

## 8) Resource 与序列化边界

### Resource 负责

- 句柄、引用计数、加载/卸载调度。
- 对外门面：`ResourceSystem`（Bind 的 `AssetManager` 为权威；提供 Get/Import/Registry 等糖衣）。
- `AssetSerializerRegistry` / `IAssetSerializer`：按类型与扩展名分发。
- 调用内核提供的基础 IO，不实现全套平台读写。

### Module / World 负责

- **具体序列化逻辑**（字段、格式、领域不变量）写在各自模块内，初始化时向 Resource 注册。
- 场景实体图由 `World/Scene/SceneSerializer` 读写 `.himii`（与资产注册表分工见 [场景序列化](CoreSystems/Serialization.md)）。

### 推荐数据流

```text
业务 → ResourceSystem 对外 API
         → 公共：IO / 句柄 / 引用计数 / 查注册表
         → Module（或 World）自己的 Serializer 实现
```

---

## 9) 当前 Module 地图（骨架）

| 路径 | 内容（摘要） |
|------|----------------|
| `Module/Audio/` | 音频实现；`AudioModule : IModule` |
| `Module/Render/` | Renderer2D/3D、`RenderModule`；`SceneRenderModule` + `SceneRenderer` |
| `Module/Physics/` | `Physics2DModule`、`Physics2DWorld` |
| `Module/Animation/` | SpriteAnimation 资产/系统；`SpriteAnimationModule` → `Animation` 阶段 |
| `Module/Particle/` | 粒子资产/系统；`ParticleModule` → `Presentation` |
| `Module/Script/` | ScriptEngine / Glue / Compiler；`ScriptUpdateModule` / `ScriptFixedUpdateModule` |
| `Module/Tilemap/` | TileSet / TileMap 等 |
| `Module/UserInterface/` | `UserInterfaceModule`；场景 UI 实现 |
| `Module/Resource/` | `ResourceModule`：注册内置序列化器 |

---

## 10) HimiiEditor 与 HimiiRuntime

- **HimiiEditor**：`EditorLayer` + 各 Panel；持有 `World`，Edit / Play / Simulate 下切换活动 Scene 并调用 `World` 的 Runtime/Simulation API。
- **HimiiRuntime**：无编辑器 UI 的启动器，同样经引擎模块与 World 跑场景。
- **ScriptCore**：C# 侧 Entity、Input、Log、`InternalCalls` 等，供游戏程序集引用。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| 编辑器主循环、Play / Simulate | [编辑器运行时](CoreSystems/EditorRuntime.md) |
| 脚本宿主与调用链 | [脚本系统（C++）](CoreSystems/Scripting.md) |
| 日志与 Console | [日志数据流](CoreSystems/Logging.md) |
| OpenGL / GLM / ImGui 速查 | [图形笔记](CoreSystems/GraphicsNotes.md) |
| `.himii` 与资产序列化 | [场景序列化](CoreSystems/Serialization.md) |
| 源码构建 | [Building From Source](BuildingFromSource.md) |
| 用户：脚本 API | [Scripting API](../UserManual/ScriptingAPI.md) |
| 功能规划 | [Roadmap](../Roadmap.md) |
