# 脚本工作流 (Script Workflow)

本文说明在 HimiiEditor 中编写、编译与调试 C# 游戏脚本的典型流程。

## 项目中的脚本文件

打开项目后，脚本通常位于：

```
<ProjectDir>/
├── assets/
│   └── scripts/          # 游戏 C# 源码（*.cs）
├── <ProjectName>.sln     # 供 IDE 打开
├── <ProjectName>.csproj
└── <ProjectName>.hproj   # 项目配置（含 ScriptModulePath 等）
```

编译产物 **`GameAssembly.dll`**（及 `.pdb`）路径由 `.hproj` 的 `ScriptModulePath` 指定，构建时输出到项目目录或 `bin` 子目录（以项目模板为准）。

引擎自带 **`ScriptCore.dll`**（API：`Entity`、`Input`、`Log` 等），由 HimiiEditor 构建时拷贝到可执行文件旁，**不要**修改其命名空间下的核心类型。

## 编译

- 打开项目时：若 `GameAssembly.dll` 比全部 `assets/scripts/**/*.cs` 与根目录 `GameAssembly.csproj` 都新，则**跳过编译**，直接加载已有 DLL；否则异步 `dotnet build`。
- 保存 `.cs` 或 `GameAssembly.csproj` 后，文件监视会触发异步编译（debounce）。编译进行中再次保存会**排队**，结束后再编一轮。
- **Play / Simulate 中**不会自动编译，也不会强制 Stop；仅标记脚本为 dirty。再次点击 **Play**（或菜单 **Compile and Reload**）时才会编译。
- 点击 **Play** 前若脚本有改动或正在编译，会先编译成功再进入 Play。
- 编译日志在 **Window → Console**（打开 **Compile** 通道，默认已开）：
  - 成功 / 失败状态
  - **红色 error** 行可点击，在配置的 IDE 中打开对应位置（Visual Studio：打开项目 `.sln` 后再打开文件，不使用 `/edit`）
  - **黄色 warning** 仅高亮显示，不可点击跳转

> **【配图占位】** `images/script-console-compile.png` — Console 中的编译输出

## 运行与热重载策略

- **Play**：复制场景、加载 `GameAssembly`、为带 `ScriptComponent` 的实体调用 `OnCreate`；每帧先 `OnUpdate`，再 Box2D 步进与碰撞事件，最后 **`OnFixedUpdate`**（物理之后，见下节）。
- **Stop** 后回到编辑场景；修改脚本后需再次 **Play** 才会加载新 DLL（**不在 Play 中热替换程序集**）。
- 编译成功且曾中断 Play 时，可能提示重新按 Play。

运行时日志使用 `Log.Info` / `Warning` / `Error`，在 **Window → Console** 查看（见 [脚本 API](ScriptingAPI.md)）。

## 物理时序与平台跳跃（重要）

Play 模式下，单帧脚本与物理顺序为：

1. **`OnUpdate`**：读输入、UI、相机等（此时尚未执行本帧物理步进）
2. **Box2D `World.Step`** + 同步 `Rigidbody2D` 位置
3. **`OnCollisionEnter2D` / `OnCollisionExit2D`**（若本帧有接触变化）
4. **`OnFixedUpdate`**：适合移动、跳跃、**着地检测**（此时速度与本帧碰撞已更新）

### 平台角色着地建议

| 做法 | 说明 |
|------|------|
| **推荐** | 在 `OnFixedUpdate` 中用 **`Physics2D.Raycast`** 从脚底向下短射线判定着地；用 `hit.Normal.Y` 过滤墙面 |
| **可用** | `OnCollisionEnter2D` 中若 `collision.Normal.Y` 足够大，表示脚下有支撑（法线指向本实体） |
| **不推荐** | 单独依赖 `OnCollisionEnter` 维护 `_isGrounded`：出生时可能与地面重叠而无 begin 事件；起跳后盒体仍贴地时也不会再次 Enter |

起跳后可在数帧内忽略射线着地（避免贴地误触），详见 Test 项目 `PlayerController.cs`。

修改 **ScriptCore**（新增 `OnFixedUpdate`、扩展 `Collision2DInfo`）后，需 **重新编译 ScriptCore 并重启 HimiiEditor**，再 **Compile and Reload** 游戏程序集。

## IDE 配置

### 全局默认（Edit → Preferences）

- 选择 **Visual Studio**、**VS Code**、**Rider** 或 **Custom**（自定义可执行文件与参数模板）。
  - Custom 占位符：`{ProjectDir}` `{Solution}` `{File}` `{ProjectName}` `{Line}`
- 保存到引擎数据目录下的 `editor_settings.yaml`。

### 项目覆盖（File → Project Settings）

- **Use Editor Default**：继承全局设置。
- 或为本项目单独指定 IDE / Custom 路径。
- 写入 `.hproj`，随项目一并分享。

> **【配图占位】** `images/project-settings-ide.png` — Project Settings 脚本 IDE 配置

### 打开项目 / 脚本

- **File → Open C# Project**：用当前有效配置打开 `.sln` 或项目目录。
- 属性面板 **Script Component → Edit**：打开对应 `.cs` 文件。

**Custom** 参数占位符：`{ProjectDir}`、`{Solution}`、`{File}`、`{ProjectName}`。

## 常见问题

| 现象 | 处理 |
|------|------|
| Play 后脚本不执行 | 检查类名是否与 `ScriptComponent` 中一致（含命名空间）；Console 中 Compile 是否成功 |
| Inspector 无字段 | 字段须为 **public 实例** 或 **private + `[SerializeField]`**；脚本类名与组件一致；**重新编译 ScriptCore 并重启编辑器** 后重选实体 |
| `[SerializeField]` 无智能提示 | 用 IDE 打开 **`.sln`**；`GameAssembly` 引用 **ScriptCore.dll**；脚本顶部 `using HimiiEngine;`；重载 C# 语言服务 |
| Ctrl+S 弹出保存 Animation | 仅在 Animation Editor 内对**未 Save As 的新动画**点 Save 会弹窗；全局 Ctrl+S 已忽略未落盘动画 |
| Log 无输出 | 使用 `Himii.Log`，非 `Console.WriteLine`；确认 Console 面板已打开 |
| IDE 打不开 | Preferences / Project Settings 中检查 IDE 路径；VS Code 需 `code` 在 PATH |

## 相关文档

- [脚本 API](ScriptingAPI.md)
- [教程：2D 平台跳跃](Tutorial2DPlatformer.md)
- [编辑器与功能](EditorFeatures.md)
- [编辑器界面](EditorInterface.md)
- [2D 逐帧动画](SpriteAnimation.md)
- [脚本 API](ScriptingAPI.md)
- [源码构建](../DevManual/BuildingFromSource.md)
