# 开发路线图

本文档采用 **核心模块 - 最小执行单元** 的结构对应当下的开发进度与未来的规划。

**Phase A 定位（已冻结）**：继续深耕 **纯 2D**；以 **能力清单打勾** 为验收标准（不绑定某款样板游戏）。  
**实施顺序**：轨 1（场景结构）与轨 2（音频）并行，最后收束发布管线。

---

## 📅 已完成特性 (Phase 1: Foundation)

### 核心模块：基础架构 (Core Architecture)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **Application Loop** | 实现应用启动、主循环、关闭流程 |
| [x] | **LayerStack** | 实现层的压栈、弹栈与遍历更新 |
| [x] | **Window Abstraction** | 基于 GLFW 封装跨平台窗口创建与上下文管理 |
| [x] | **Event System** | 实现键盘、鼠标、窗口事件的分发与回调 |
| [x] | **Log System** | 集成 organic spdlog 日志库 |

### 核心模块：渲染系统 (Renderer)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **OpenGL Context** | 集成 GLAD 并管理 OpenGL 上下文 |
| [x] | **Shader Class** | 着色器的加载、编译与链接 |
| [x] | **Buffer System** | VertexBuffer, IndexBuffer, VertexArray 的抽象与实现 |
| [x] | **Texture System** | 2D 纹理加载与绑定 (stb_image) |
| [x] | **Orthographic Camera** | 正交相机控制器 |
| [x] | **Batch Renderer** | 2D 批处理渲染 (自动合并 DrawCall) |
| [x] | **Framebuffer** | 离屏渲染支持 (仅颜色附件) |
| [x] | **SubTexture2D** | 纹理图集切片支持 |

### 核心模块：物理系统 (Physics 2D)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **Box2D World** | 物理世界初始化与步进 (Step) |
| [x] | **Rigidbody2D** | 刚体组件属性同步 (Transform <-> Box2D Body) |
| [x] | **BoxCollider2D** | 矩形碰撞体与夹具 (Fixture) 创建 |
| [x] | **Collision Events 2D** | `OnCollisionEnter2D` / `OnCollisionExit2D` 脚本回调 |

### 核心模块：脚本系统 (Scripting)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **CoreCLR Host** | .NET 8 + 可收集 AssemblyLoadContext（非 Mono） |
| [x] | **Script Class** | C# 类加载与实例化（OnCreate / OnUpdate / OnDestroy） |
| [x] | **Internal Calls** | C++ 与 C# 互操作绑定 |
| [x] | **Input API** | C# `Input.IsKeyDown` 等接口 |
| [x] | **Script Fields** | Inspector 公开字段读写与 YAML 序列化（含 Entity 引用） |
| [x] | **Async Compile** | `dotnet build` 异步编译与文件监视 |
| [x] | **Script Console** | 已并入 Console：编译日志、错误行跳转 IDE |
| [x] | **IDE Launcher** | 全局/项目 IDE 配置（VS / VS Code / Rider / Custom） |
| [x] | **Script Log API** | C# `Log.Info` / `Warning` / `Error`，经 C++ `Log::PrintMessage` 输出 |

### 核心模块：编辑器 (Editor Interface)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **DockSpace** | 全屏停靠布局支持 |
| [x] | **Scene Viewport** | 场景编辑视口 (渲染 FBO 纹理) |
| [x] | **Hierarchy Panel** | 实体层级树状列表 |
| [x] | **Properties Panel** | 组件属性检视与修改 |
| [x] | **Content Browser** | 资源文件浏览与拖拽 |
| [x] | **ImGuizmo** | 视口内物体变换 Gizmo |
| [x] | **Editor Preferences** | 全局编辑器设置（含默认脚本 IDE） |
| [x] | **Project Settings** | 项目级 IDE 覆盖与 `.hproj` 序列化 |
| [x] | **Console Panel** | 运行时脚本日志 ImGui 面板（`Window → Console`） |

### 核心模块：游戏性与脚本（Phase 1 已落地部分）
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **Prefab System（基础）** | 单实体 `.hprefab` 保存与实例化（无实例覆盖） |
| [x] | **Scene Transition（基础）** | `SceneManager.LoadScene` + `ActiveScenePath` |
| [x] | **[SerializeField]** | ScriptCore 提供特性；private 字段 Inspector 显示与场景序列化 |
| [x] | **Undo/Redo** | 命令模式；Transform、Hierarchy 等操作已接入 |
| [x] | **Project Packager（基础）** | 已被完整 **Build Pipeline** 取代；历史：编辑器 File → Build Project 扁平拷贝 Runtime / 资源 |

### 核心模块：发布 (Distribution)
| 状态 | 最小执行单元 (Minimum Execution Unit) | 说明 |
| :--- | :--- | :--- |
| [x] | **Runtime Application** | `HimiiRuntime` 剥离编辑器的游戏运行程序 |

---

## 🎯 Phase A：2D 能力闭环（当前冲刺 · 已冻结）

验收方式：**下列最小执行单元全部勾选即算 Phase A 完成**（不以某款游戏发行为门槛）。

### 实施顺序
1. **轨 1 — 场景结构**：实体父子层级 → 多实体 Prefab → Canvas（含 Scaler）→ UIButton  
2. **轨 2 — 音频**（与轨 1 并行）：`AudioEngine` + miniaudio → `SoundAsset` / 非空间化 `SoundPlayer` + C# / 编辑器  
3. **收束 — 发布**：完整 Build Pipeline（在功能稳定后进行）

### 轨 1：场景结构与 UI
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [x] | **Entity Parent Hierarchy** | 只存 `Parent` 为唯一真相；Children 运行时重建/缓存；Hierarchy 显示为树 |
| [x] | **Local Transform** | `TransformComponent` 的 Position/Rotation/Scale 语义改为 **Local**；世界矩阵由父链计算（可缓存）；字段名暂不强制重命名 |
| [ ] | **Multi-Entity Prefab** | 利用父子层级序列化一组实体为 `.hprefab`；**不做**实例属性覆盖 / 变体 / Prefab 嵌套覆盖 |
| [x] | **Canvas (Screen Space Overlay)** | 引入 Canvas（或等价 UI 根）；至少支持 Screen Space Overlay |
| [x] | **Canvas Scaler** | 参考分辨率自适应；无 Scaler 则 Canvas 不算完成 |
| [x] | **UIButton（最小）** | 挂在 Canvas 子树下；点击回调（按下/悬停/点击）；不做通用布局系统 |

### 轨 2：音频
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [x] | **AudioEngine Abstraction** | 引擎侧音频接口抽象；首期后端为 **miniaudio**（解码与设备）；预留更换中间件的可能 |
| [x] | **SoundAsset** | 可引用音频资产（WAV / OGG / MP3）；一律解码进内存；`AssetHandle` 体系 |
| [x] | **SoundPlayer（非空间化）** | Play / Stop / Pause、Loop、Volume、Mute、PlayOnStart、PlayOneShot；最多 32 voice（超限优先丢弃新 OneShot）；**不做** 空间衰减 / Listener / Mixer |
| [x] | **Audio C# API** | ScriptCore `SoundPlayer` 组件可驱动上述播放控制 |
| [x] | **Audio Editor** | Inspector（含 Preview）、Content Browser 导入；仅 Play/Runtime 自动出声，Stop 时全部立刻停止 |

### 收束：发布管线
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [x] | **Build Pipeline** | 编辑器一键输出可运行目录树：统一配置、**瘦** `engine.hpck`、项目 `assets` 全量、资源校验、Runtime / 依赖 / `GameAssembly`；Editor 与 Templates 双 hpck 分层；缺关键文件则 Build 失败并报错。不要求干净机器冒烟或自动拉起 Runtime |

### Phase A 明确排除（本阶段不做、不算进度）
| 类别 | 排除项 |
| :--- | :--- |
| 渲染 / 物理 | Directional/Point Shadow、Bloom、Post-Processing Stack、3D 物理（Jolt/PhysX 等） |
| 游戏性 | 玩家存档 Save/Load、Prefab 实例覆盖/变体、音频空间化 |
| 工具链 | 统一 Asset Importer、Console 增强、粒子系统收尾 |
| 其他 | 网络、骨骼动画、Job System、Vulkan 后端落地 |

---

## 🚀 Phase B 及以后（未冻结 · 仅占位）

以下条目在 Phase A 完成前 **不计入当前冲刺**；排序与范围将在 Phase A 验收后另行冻结。

### 渲染增强 / 粒子
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [x] | **Directional Shadow** | 视锥级联阴影贴图（4 层 atlas，方向光） |
| [ ] | **Point Shadow** | 全向阴影 (CubeMap) |
| [ ] | **Bloom** | 泛光后处理特效 |
| [ ] | **Post-Processing Stack** | ToneMapping、Gamma Correction 等 |
| [ ] | **Particle System** | GPU/CPU 粒子可用化（编辑器已有基础） |

### 物理 3D
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [ ] | **Physics Engine Integration** | 集成 Jolt Physics 或 PhysX |
| [ ] | **Sphere / Capsule / Mesh Collider** | 3D 碰撞组件 |
| [ ] | **Physics Debug Draw** | 线框可视化调试 |

### 编辑器与内容管线
| 状态 | 最小执行单元 (Minimum Execution Unit) | 预期内容 |
| :--- | :--- | :--- |
| [ ] | **Asset Importer** | 统一的资源导入设置面板 |
| [ ] | **Console 增强** | 日志过滤/搜索/复制；默认展示引擎日志等 |

### 可能的后续方向（仅备忘）
- Prefab 实例覆盖 / 变体；2D 空间音频；玩家存档 API  
- 更完整的 UI 布局系统；Job System；Vulkan 后端  
- 网络、骨骼动画、AI（按产品需求再开）
