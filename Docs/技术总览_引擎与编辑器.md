# 技术总览（引擎与编辑器）

本文档总结本项目当前的技术栈、工程结构、核心架构、运行生命周期、关键调用关系，并整理常用 OpenGL/GLM/ImGui 相关知识点，便于快速上手与维护。

---

## 1) 技术栈一览

- 语言与构建
  - C++17
  - CMake + MSBuild（Windows）
  - vcpkg 清单模式（`vcpkg.json`）
- 第三方库（按用途）
  - 渲染：OpenGL 4.x（GLAD 加载）
  - 窗口与输入：GLFW
  - 数学：GLM
  - UI：Dear ImGui（Docking）
  - 纹理：stb_image
  - 日志与格式化：spdlog, fmt
- 运行形态
  - 原生桌面应用（TemplateProject.exe）
  - 内嵌编辑器 UI（DockSpace + 多面板）

---

## 2) 项目/目录结构（精简）

- 顶层
  - `CMakeLists.txt` / `CMakePresets.json` / `build.py`/`build_py.bat`
  - `vcpkg/`（本地 vcpkg）
  - `build/`（生成目录，区分 x64-debug / x64-release）
  - `Docs/`（文档）
- Engine/
  - `src/Engine.h`（聚合头）
  - `src/Himii/Core`：Application、Layer、LayerStack、Window、Log、Event 等
  - `src/Himii/Renderer`：Renderer、RenderCommand、Shader、Texture、Buffer、VertexArray、Framebuffer（抽象）等
  - `src/Platform/OpenGL`：OpenGL 具体实现（Buffer/VertexArray/Shader/Texture/Framebuffer 等）
  - `vender/imgui`：ImGui 集成
- TemplateProject/
  - Demo/编辑器层：`EditorLayer.*`、`CubeLayer.*`、`Example*` 等
  - 资源与着色器：`assets/`（textures/shaders/fonts）

---

## 3) 模块与架构

- Core（跨平台层）
  - Application：主循环、层管理（LayerStack）与事件派发
  - Layer：抽象生命周期接口（OnAttach/OnUpdate/OnDetach/OnImGuiRender/OnEvent）
  - Window/Input/Events：窗口、输入、事件类型（如 WindowResizeEvent）
- Renderer（API 无关抽象）
  - RenderCommand：封装清屏/视口/DrawIndexed 等
  - Shader/Texture/Buffer/VertexArray：渲染资源抽象
  - Framebuffer：离屏渲染目标抽象（颜色附件 + 深度/模板）
  - Renderer：BeginScene/Submit/EndScene（设置 VP 矩阵与批量提交）
- Platform.OpenGL（后端实现）
  - OpenGLFramebuffer：FBO + 颜色纹理（RGBA8）+ 深度模板 RBO（DEPTH24_STENCIL8），Bind 时设置 Viewport
  - 其它 GL 对象：VAO/VBO/IBO/Shader/Texture 的 OpenGL 实现
- UI/Editor（基于 ImGui）
  - DockSpace 顶部菜单条 + 面板系统
  - Scene/Game 视图面板（通过 ImGui::Image 展示 FBO 颜色纹理）
- 示例/玩法层（TemplateProject）
  - CubeLayer：地形生成与渲染、相机、离屏渲染驱动
  - EditorLayer：面板布局、尺寸/焦点/悬停状态回传、接收 FBO 纹理

---

## 4) 生命周期（运行时）

1. 启动
   - 初始化日志、窗口、OpenGL（GLAD）、ImGui 等
   - 装载 Layer：如 CubeLayer（场景逻辑）、EditorLayer（编辑器 UI）
2. 主循环（Application::Run，伪代码）
   - 处理系统事件 → 分发到各 Layer::OnEvent
   - 计算 DeltaTime（Timestep）
   - 遍历 LayerStack 调用 OnUpdate(ts)
   - 开始 ImGui 帧 → 遍历调用 OnImGuiRender() → 渲染 ImGui
3. 退出
   - 逆序 Detach 层、释放资源

常见 Layer 生命周期钩子：
- OnAttach：创建资源（VAO/VBO/IBO、Shader、Texture、Framebuffer 等）
- OnUpdate(ts)：更新输入/相机、执行渲染（含离屏渲染）
- OnImGuiRender：参数面板、视图面板等 UI
- OnDetach：释放资源

---

## 5) 典型调用路径（渲染帧）

- 输入/相机
  - EditorLayer 提供 Scene/Game 面板尺寸与悬停/聚焦标记
  - CubeLayer::OnUpdate：当 Scene 悬停/聚焦时响应 WASD + 右键鼠标以更新相机
- 离屏渲染
  - 获取面板期望尺寸 → 调整各自 FBO（Scene/Game）
  - 计算投影矩阵：`glm::perspective<float>(fov, aspect, near, far)`
  - 计算视图矩阵：`glm::lookAt(pos, target, up)`
  - Renderer::BeginScene(viewProj)
    - 先渲染天空盒（深度函数 LEQUAL + 深度写禁用）
    - 再渲染体素地形（光照/法线/纹理图集）
  - Renderer::EndScene()；FBO::Unbind()
- UI 展示
  - EditorLayer 读取 FBO 颜色纹理 ID → ImGui::Image 翻转 UV（(0,1)-(1,0)）显示

---

## 6) 主要子系统要点

### 6.1 帧缓冲（Framebuffer）
- 抽象接口：Bind/Unbind/Resize/GetColorAttachmentRendererID/GetSpecification
- 规格（Specification）：`Width/Height`
- OpenGL 实现（OpenGLFramebuffer）
  - 颜色附件：2D 纹理 RGBA8
  - 深度模板：Renderbuffer DEPTH24_STENCIL8
  - Invalidate()：在 Resize 时销毁并重建附件
  - Bind()：glBindFramebuffer + glViewport(0,0,w,h)

### 6.2 渲染资源
- Buffer/VertexArray：设置 BufferLayout，按顺序绑定属性（位置/法线/颜色/UV/索引等）
- Shader：统一设置采样器数组 `u_Texture[0..31]`，`SetIntArray` 绑定到纹理单元
- Texture：stb_image 加载，支持图集；渲染时按图集中 UV 采样

### 6.3 编辑器面板
- Scene 面板：主要用于操控相机
- Game 面板：展示相同场景的独立离屏结果（可扩展为独立相机）
- 面板尺寸变化 → 触发对应 FBO Resize，保持清晰度与正确纵横比

---

## 7) 地形生成（概览）

- 体素化：仅发射可见面的网格，极大降低三角形数
- 噪声层次
  - Plains（fBm）与 Mountain（Ridged）按 biome 混合
  - Continent（低频）塑造大陆与海洋格局
  - Detail + Warp 消除网格感并增加细节
- 形状整形
  - Plateau（台地分段）、CurveExponent（分布曲线）、ValleyDepth（谷地加深）、SeaLevel（海平面）
- UV 图集
  - 为不同面（顶/底/侧）选取不同 tile；支持 padding 避免采样污染

---

## 8) 天空盒与光照

- 天空盒：程序化渐变（Top/Horizon/Bottom），以相机位置为中心的大立方体
  - 绘制时：`glDepthFunc(GL_LEQUAL) + glDepthMask(GL_FALSE)`，避免遮挡前景物体
- 光照（简化）
  - 环境光（颜色+强度）
  - 定向光（方向+颜色+强度）
  - 在 `LitTexture.glsl` 中进行漫反射近似

---

## 9) OpenGL/GLM/ImGui 知识点速查

### OpenGL
- 坐标与裁剪空间
  - GLM 默认右手坐标；投影/视图组合后在 NDC（-1..1）进行裁剪
- 透视投影
  - `glm::perspective<float>(fovyRadians, aspect, zNear, zFar)`
  - 建议在调用处保证参数为 `float`，并包含 `glm/gtc/matrix_transform.hpp` 或 `glm/ext/matrix_clip_space.hpp`
- 深度测试/写入
  - 常规绘制：`glDepthFunc(GL_LESS)`，写入开启
  - 天空盒：`glDepthFunc(GL_LEQUAL)` + 关闭深度写（`glDepthMask(GL_FALSE)`）
- 剔除与绕序
  - 调试期可关闭剔除：`glDisable(GL_CULL_FACE)`；开启后需保证绕序一致（默认 CCW）
- Framebuffer
  - 颜色附件纹理（采样展示）+ 深度/模板（RBO）
  - Resize 时需删除并重建附件；Bind 时更新 Viewport
- 纹理图集
  - 统一采样器数组（绑定到各纹理单元）
  - UV 区块需留 padding 抗 bleeding，边缘使用 CLAMP 或适当的采样策略

### GLM
- 角度：`glm::radians(deg)` / `glm::degrees(rad)`
- 相机：`glm::lookAt(eye, center, up)`
- 常见 include
  - 变换：`#include <glm/gtc/matrix_transform.hpp>`
  - 透视/正交：`#include <glm/ext/matrix_clip_space.hpp>`（可选增强）

### ImGui
- Docking 启用后可自由布局
- 图像显示需注意 UV 翻转（OpenGL 纹理原点与 ImGui 的期望可能不同）
- 面板尺寸变化通过 `GetContentRegionAvail()` 获取并回传给渲染层

---

## 10) 构建与运行

- 依赖：本地 `vcpkg/` 已在工程中集成；CMake Presets 指向对应 triplet 与工具链
- 构建（示例）
  - Debug：`build/x64-debug` 目录内生成 `TemplateProject.exe`
  - Release：`build/x64-release`
- 资源
  - CMake 构建过程会将 `assets/` 拷贝到输出 `bin/` 目录

---

## 11) 性能与稳健性建议

- 网格生成
  - 继续剔除不可见面；分区块（Chunk）与懒加载可在大地图下提升性能
- 渲染
  - 使用索引缓冲、合并 draw call；必要时启用简单批处理
  - 后续可引入 MSAA、sRGB 帧缓冲，或阴影贴图等效果
- 编辑器/交互
  - 为 Game 面板增加独立相机及输入切换
  - 视口 letterboxing：保持目标纵横比时留黑避免拉伸

---

## 12) 未来工作（Roadmap）

- 资源系统（热加载/引用计数）
- 场景与实体组件系统（ECS）
- 多渲染后端适配层（如未来接 Vulkan/DirectX）
- 离屏后期处理链（Bloom/SSR/ToneMapping）
- 跨平台构建与 CI

---

## 13) 术语表（简）

- FBO（Framebuffer Object）：离屏渲染目标
- RBO（Renderbuffer Object）：通常用于深度/模板附件
- VAO/VBO/IBO：顶点数组/顶点缓冲/索引缓冲
- NDC：标准设备坐标，裁剪后范围 [-1,1]
- fBm：分形布朗运动（分形噪声叠加）
- Ridged：脊状噪声（强调山脊）
