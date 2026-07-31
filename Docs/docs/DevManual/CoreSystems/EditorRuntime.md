# 编辑器运行时（Play / Simulate）

本文档描述 HimiiEditor 主循环如何驱动 `World`，以及 Edit / Play / Simulate 状态切换。面向引擎贡献者。

架构分层见 [架构概览](../Architecture.md)。脚本宿主细节见 [脚本系统](Scripting.md)。

---

## 1) 进程启动（概要）

1. `Application` 创建窗口，注册 Application 级模块并 `InitializeAll`（Resource → Render → Audio → Script）。
2. 压入 `ImGuiLayer` 与 `EditorLayer`（及可选其他 Layer）。
3. 主循环：事件 → Layer `OnUpdate` → ImGui → 呈现。
4. 退出时 Shutdown 模块、释放脚本宿主等。

编辑器启动加载分步（图标、天空盒、字体、空场景 + `World`、面板、最近项目）在 `EditorLayer::AdvanceEditorStartupLoading`；若有命令行项目路径则调用 `OpenProject`。

---

## 2) 每帧谁在 Tick

| 场景状态 | 驱动 |
|----------|------|
| **Edit** | 编辑器相机、视口绘制等由 EditorLayer / 面板逻辑处理；世界模块表仍挂在当前编辑 Scene 上 |
| **Play** | `World::OnUpdateRuntime(timestep, …)`：按 `WorldUpdatePhase` 顺序推进 UI → 脚本 → 动画 → 物理 → FixedUpdate → 粒子 → Render |
| **Simulate** | `World::OnUpdateSimulation(timestep, editorCamera)`：物理 → 动画 → ScriptFixedUpdate → Render（不跑完整游戏脚本 Update 路径） |

Application 级模块（音频设备、脚本宿主寿命等）仍由 `Application` 的 `ModuleRegistry::UpdateAll` 推进，与 World 阶段表分离。

---

## 3) Play

`EditorLayer::StartScenePlay`（摘要）：

1. 若正在 Simulate，先 `OnSceneStop`。
2. 清空命令历史与 `ConsoleLog`。
3. `m_ActiveScene = Scene::Copy(m_EditorScene)`，再 `World::SetActiveScene`（会重建世界模块）。
4. `World::OnRuntimeStart` → 场景侧 Runtime 起停（含脚本实例化、物理世界创建等）。
5. Hierarchy 等面板切到活动场景。

每帧：`m_World->OnUpdateRuntime(...)`。

---

## 4) Simulate

`EditorLayer::OnSceneSimulate`：

1. 若正在 Play，先停止。
2. 复制编辑场景并 `SetActiveScene`。
3. `World::OnSimulationStart`（物理等仿真起动；不按 Play 完整实例化游戏脚本）。

每帧：`m_World->OnUpdateSimulation(ts, m_EditorCamera)`。

---

## 5) Stop

`EditorLayer::OnSceneStop`：

1. 按原状态调用 `OnRuntimeStop` 或 `OnSimulationStop`。
2. 将活动场景恢复为 `m_EditorScene`，再次 `SetActiveScene`。
3. 面板上下文切回编辑场景。

---

## 6) 打开项目 / 场景

- `OpenProject`：加载 `.hproj`、资源与脚本程序集，再 `OpenScene(StartScene)` 或 `NewScene`。
- `OpenScene` / `NewScene`：创建或反序列化 `Scene`，`World::SetActiveScene` → 世界模块按新 Scene 重建。

因此冷启动日志里可能先为启动空场景注册一轮 World 模块，打开项目场景后再注册一轮——属预期。

---

## 代码索引

- `HimiiEditor/src/EditorLayer.cpp`：`StartScenePlay` / `OnSceneSimulate` / `OnSceneStop` / `OpenProject`
- `Engine/src/World/World.cpp`：`SetActiveScene`、`OnUpdateRuntime`、`OnUpdateSimulation`、`BuildModulesForActiveScene`
- `Engine/src/World/WorldUpdatePhase.h`
