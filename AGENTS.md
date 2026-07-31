# HimiiEngine Agent 规范

本文件是仓库内 AI Agent（Cursor / Claude Code / Codex 等）的**唯一共享约束源**，已入库。  
与开发者沟通、撰写本仓库规范与开发者文档时使用**简体中文**（路径、CLI、目录名保持原文）。

---

## 语言与命名

- **严禁缩写命名**：变量、函数、类、常量等须使用完整英文单词（例如用 `index` 而非 `idx`；不要用随意缩写的 `config` / `msg` / `ctx` 等）。
- 命名风格遵循各语言惯例与项目既有风格，但「不缩写」优先。
- **智能指针统一走 `EngineCore/Core/Core.h`**：
  - 使用 `Scope<T>` / `Ref<T>`，禁止在业务代码中直接写 `std::unique_ptr` / `std::shared_ptr` 作为对外类型别名用法。
  - 创建对象使用 `CreateScope<T>(...)` / `CreateRef<T>(...)`，禁止 `std::make_unique` / `std::make_shared`。
  - 仅允许在 `Core.h` 内部实现里调用 `std::make_unique` / `std::make_shared`；第三方库边界若必须接 `std::shared_ptr` API，应尽量经 `CreateRef` / `Ref`  wrapping 后再交给第三方。

---

## 改动边界

- **最小必要改动**：不重构无关代码，不顺手清理未要求的文件。
- **禁止修改** `Engine/vender/**` 及第三方子模块源码（除非任务明确要求升级依赖，并经开发者确认）。
- **禁止提交**构建与生成产物：`build/`、`bin/`、`obj/`、无意义的本地缓存等。
- **禁止擅自** `git commit` / `git push`；仅在开发者明确要求时提交。

---

## 新功能成套交付

新增或显著扩展一个**组件 / 资产类型 / 可序列化字段**时，默认应成套完成（缺一环则说明并征得同意后再省略）：

1. 引擎侧定义（如 `Components.h` / 资产类型）
2. 场景或资产**序列化**读写
3. Editor **Inspector Drawer**（遵守下文产品 UX）
4. 若对脚本开放：ScriptCore + `ScriptGlue` / `InternalCalls` 成对更新

---

## 跨平台与接口预留

撰写或扩展**引擎侧新功能**时，应优先判断：该能力是否会因**目标平台 / 后端实现**不同而分叉；若会，应在合适层级**预留抽象接口**，避免把某一平台的具体 API 写进上层业务。

- **参考现状**：渲染经 `RendererAPI` 等抽象选择后端（如 OpenGL；Vulkan / DirectX12 / Metal 等可扩展槽位），上层绘制逻辑不直绑单一图形 API。
- **适用**：窗口/输入、文件系统路径、动态库加载、音频输出、图形与计算后端、未来可能分平台的烘焙/打包步骤等。
- **不做过度设计**：若功能与平台无关（纯数据、序列化字段、多数玩法组件），不必强行加抽象层。
- **与「最小必要改动」平衡**：优先复用或延伸**已有**抽象（如现有 `RendererAPI`）；若需新增公共抽象基类 / 工厂 / 后端枚举，遵守上文「不确定时先问」，通过后再改。
- **禁止**：在 Editor / 场景 / 脚本胶水等上层路径直接散落 `#ifdef` 或裸调用某一平台专有 API（应下沉到 `Platform/**` 或对应后端实现）。

---

## 编辑时与运行时

- 玩法 / 模拟逻辑放在 Engine / Runtime / 脚本侧，不要塞进 Editor Panel。
- Editor 不得偷偷改写「仅运行时」状态，除非有明确的编辑器预览语义并经开发者确认。

---

## 不确定时先问

- 需要新增或扩展**公共 API**（含 `InspectorControls`、Scene、Asset、ScriptGlue 对外面）时：**先提问，通过后再改**。
- 需求、归属层级（Engine / HimiiEditor / ScriptCore / HimiiRuntime）或序列化兼容性不清晰时：先问，不猜测实现。

---

## 禁止硬编码路径与资源引用

- **不允许**在代码中直接硬编码路径、资源相对路径、默认资源文件名、包内条目名等「魔法字符串」（例如 `"assets/fonts/msyh.ttc"`、`"resources/skybox/right.bmp"`、固定输出目录名等）。
- 若确需约定路径 / 默认资源名 / 布局常量：先与开发者商量并确认放置位置（集中常量、配置、项目种子约定等），**通过后再写**；禁止 Agent 自行散落字面量。
- 已有集中 API / 常量时必须复用（如 `Project::GetDefaultGameplayFontRelativePath()`、`FileSystem`、项目配置字段），不要再复制一份字面量。
- 用户可见文案、日志说明不在本条禁止范围内；本条针对**机器解析用的路径与资源标识**。

---

## Release 与路径依赖

- 出现**路径依赖**问题（找不到资源、相对路径失效、误读源码树等）时，须先按 **Release 发布后的 HimiiEngine** 场景排查：安装/发布目录下**无法**依赖开发机工作区路径、仓库源码树，以及仅存在于本机 Debug/开发布局中的资源与文件。
- 引擎、Runtime、打包产物所需资源须走发布管线可携带的布局（如打包资源、项目相对路径、集中常量 / 配置 API），不得假定「当前工作目录旁仍有完整仓库」或「可回退到源码旁的 assets」。
- 修路径相关 bug 时，以独立发布目录可运行为验收标准；仅在工作区旁能跑通不算完成。

---

## 产品体验（Editor / Inspector）

面向**使用引擎的游戏开发者**，不是引擎实现者的调试便利。

### 核心原则

1. **用户语义优先**：只暴露可理解概念（资源名、预览、音量、颜色、枚举含义等）。
2. **隐藏内部实现**：不得把引擎内部细节当作常规可编辑字段。
3. **样式统一**：所有属性绘制必须走 `HimiiEditor/src/InspectorControls.h`。

### 禁止暴露

| 禁止在常规 Inspector 中直接展示/编辑 | 正确做法 |
|---|---|
| 原始 `AssetHandle` 数值 | 显示资源文件名 / Sprite 名；拖放与引用字段赋值 |
| 内部指针、原生地址、调试用原始 ID | 仅日志或专用调试面板（若明确需要） |
| 仅运行时有效、用户无法理解的缓存字段 | 不绘制；或由引擎侧自动维护 |

### InspectorControls

- Panel / Component Inspector Drawer **必须**使用已有 Controls（如 `DrawFloatControl`、`DrawObjectReferenceField`、`DrawEnumComboControl`）。
- **禁止**在 Drawer 中随意拼裸 `ImGui` 绕过统一样式（已封装在 Controls 内的能力除外）。
- 若现有 Controls 无法满足：先向开发者说明缺什么 → 同意后在 `InspectorControls` **新增**可复用 API → 禁止只在单个 Drawer 私自发明 UI。

### 属性行（UE 风格）

- 行骨架：`标签 | 值 | Reset`；右侧列只放 Reset。
- Reset 图标为 `↺`，仅偏离该字段默认值时显示；引用字段的 Reset = 清空为 None。
- 对象引用：缩略图 + 限宽名称框（约 200px）+ 名称旁可选打开 Editor；双击名称与该按钮共用回调。
- Properties 用 `BeginInspectorPropertiesStyle` / `EndInspectorPropertiesStyle` 收紧行距，不改全局 ImGui Style。

### Vector 与 GUI

- 彩色 X/Y/Z 轴按钮仅用于空间轴：`DrawVec3Control`、`DrawVec2AxisControl`。
- 非空间二维量（分辨率、碰撞 Size/Offset、Pivot 等）用无轴色 `DrawVec2Control`。
- 颜色用 `DrawColorControl`，不要用轴色表达 RGBA。
- GUI 用 `DrawInspectorSectionHeader` 分区；Canvas 根上的 Rect Transform 只读摘要；GUI Drawer 必须走 Controls。

### 文案

- 标签用产品语义（如 “Sprite”“Volume”），不要只堆引擎类型名。
- 清空引用、删除组件等破坏性操作须有明确控件与可预期结果。

适用范围：`HimiiEditor/src/panel/**`、`InspectorControls.*`，以及任何写入场景/资产并呈现给引擎用户的编辑器 UI。

---

## Git 与本机生成物

| 路径 | 是否提交 |
|------|----------|
| 根目录 `AGENTS.md`、`Docs/` | **提交** |
| `.cursor/`、`.claude/`、`.codex/`、`.agents/` | **不提交** |
| `.trellis/` | **不提交**（已弃用；若本机残留可忽略） |
