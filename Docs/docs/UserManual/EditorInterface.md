# 编辑器界面 (Editor Interface)

HimiiEditor 采用 DockSpace 停靠布局，提供场景编辑、资源管理与脚本开发能力。

各功能模块（动画、物理、脚本等）的总览见 **[编辑器与功能](EditorFeatures.md)**。建议新手按 **[2D 平台跳跃教程](Tutorial2DPlatformer.md)** 边做边熟悉本页面板。

> **【配图占位】** `images/editor-dock-layout.png` — 默认停靠布局（Viewport、Hierarchy、Properties、Content Browser）

## 主要面板

### 1. 视口 (Viewport)

查看与编辑 2D/3D 场景的主区域。

- **右键 + WASD**：漫游（编辑模式下的场景相机）。
- **滚轮**：缩放。
- **Play** 模式下显示运行时画面；**Edit** 模式下可用 **ImGuizmo** 变换选中物体（移动 / 旋转 / 缩放）。

> **【配图占位】** `images/imguizmo-transform.png` — Edit 模式下视口 ImGuizmo

### 2. 场景层级 (Scene Hierarchy)

当前场景中的实体 (Entity) 树。

- **右键**：创建实体（Empty、Camera、Sprite 等）。
- **拖拽**：调整父子层级（若支持）。
- 选中实体后，右侧属性面板显示其组件。

### 3. 属性面板 (Properties)

显示选中实体的组件与参数。

- 编辑 **Transform**、**SpriteRenderer**、**Script Component** 等。
- **Add Component** 添加新组件。
- **Script Component**：填写类名、编辑 public 脚本字段（含 **Entity** 引用下拉）、**Edit** 打开脚本文件。

### 4. 内容浏览器 (Content Browser)

管理项目资源（纹理、脚本、场景等），对应项目目录下的 `assets` 等文件夹。

- 拖拽资源到视口或属性面板使用。
- 可创建 C# 脚本、打开场景文件（`.himii`）等。

> **【配图占位】** `images/content-browser-assets.png` — Content Browser 展示典型 `assets/` 结构

## 日志与调试面板

### Console（Window → Console）

脚本运行时日志、C# 编译输出与引擎警告/错误共用这一面板。

- **Clear**：清空运行时缓冲（最近一次编译输出会保留，直到下一次 `dotnet build`）。
- **Script / Compile / Engine**：通道开关。默认 Script 与 Compile 打开；Engine 打开时只显示 Warning / Error。
- **Engine Info / Trace**：显示引擎 Info / Trace（默认关）。
- **Auto Scroll**：新日志时自动滚到底部。
- 进入 **Play** 时自动清空运行时日志，**不会**清掉编译结果。
- 编译中 / 成功 / 失败显示在窗口顶部。点击带 `文件.cs(行,列): error` 的红色行，可在配置的 IDE 中打开对应脚本（见 [脚本工作流](ScriptWorkflow.md)）；warning 为黄色且不可跳转。

> **【配图占位】** `images/console-panels.png` — Console 面板

## 工具栏与运行模式

视口上方工具栏：

- **Play**：运行游戏（复制场景、启动脚本与物理）；再次点击或 **Stop** 返回编辑。
- **Simulate**：仅物理模拟（不运行脚本生命周期）。
- **Stop**：结束 Play / Simulate，恢复编辑场景。

> **【配图占位】** `images/viewport-play-toolbar.png` — 视口上方 Play / Simulate / Stop 工具栏

## 菜单要点

| 菜单 | 项 | 说明 |
|------|-----|------|
| **File** | New / Open / Save Project | 项目管理 |
| **File** | Build Project... | 导出可分发游戏目录（见 [编辑器与功能 · 构建项目](EditorFeatures.md#build-project)） |
| **File** | Open C# Project | 用配置的 IDE 打开解决方案 |
| **File** | Project Settings | 项目级脚本 IDE 覆盖（`.hproj`） |
| **Edit** | Preferences | 全局默认脚本 IDE（`editor_settings.yaml`） |
| **Window** | Console | 运行时日志与脚本编译输出 |
| **Window** | Animation Editor | 编辑 `.anim` 逐帧动画资产 |
| **Window** | TileMap / Particle 等 | 其他专项资源编辑器 |

## Animation Editor（2D 逐帧动画）

通过 **Window → Animation Editor** 或双击内容浏览器中的 `.anim` 打开。

- **左栏**：命名动画列表（Add / Rename / Delete）、FPS、Loop、图集 Cell Size。
- **右栏**：**Atlas Picker**（点格子组成时间轴）、**Timeline**（帧顺序与重排）。
- **保存**：面板内 **Save**；全局 **Ctrl+S** 仅在已打开且已修改的 `.anim` 时写入，不会为空白新动画弹另存为。

完整说明与配图见 [2D 逐帧动画](SpriteAnimation.md)。

## 保存工程（Ctrl+S）

**Ctrl+S** 会保存：

- 项目 `.hproj`
- 当前场景 `.himii`
- 已修改且已落盘的纹理 `.meta`、TileMap、**已打开的 `.anim`** 等

不会在未保存的新动画上强制弹出 **Save As**（除非你在 Animation Editor 内主动 Save 且无路径）。

> 按 Ctrl+S 后仅状态栏/日志提示保存，无文件对话框。

## 脚本 IDE 配置

- **Edit → Preferences**：设置默认 IDE（Visual Studio、VS Code、Rider 或 Custom 可执行文件）。
- **File → Project Settings**：项目可覆盖为「使用编辑器默认」或指定 IDE。

详见 [脚本工作流](ScriptWorkflow.md)。
