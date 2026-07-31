# 图形笔记（OpenGL / GLM / ImGui）

贡献者速查。渲染抽象与模块归属见 [架构概览](../Architecture.md)；世界绘制入口为 `Module/Render/SceneRenderer`。

---

## OpenGL

- 透视：`glm::perspective(fovyRad, aspect, near, far)`
- 深度：常规 `GL_LESS`；天空盒可用 `GL_LEQUAL` + 关闭深度写
- FBO：颜色纹理 + 深度 RBO；Resize 时重建附件
- 具体调用应落在 `Platform/OpenGL` 或 Render 模块实现中，上层经 `RendererAPI` / Renderer2D 等接口

---

## GLM

- `glm::lookAt`、`glm::radians` / `degrees`
- `#include <glm/gtc/matrix_transform.hpp>`

---

## ImGui

- Docking 布局；`ImGui::Image` 显示 FBO 时 UV 常为 `(0,1)-(1,0)` 翻转
- 面板尺寸用 `GetContentRegionAvail()` 回传渲染层
- 内容缩放等经 `Window` / `ImGuiPlatformBackend`，避免在 `ImGuiLayer` 直依赖 GLFW

---

## 编辑器视口（概要）

1. 根据视口尺寸 Resize 场景 / Game FBO  
2. Play 或编辑绘制路径最终经 World `Render` 阶段或编辑器专用绘制进入 `SceneRenderer` / Renderer2D  
3. 将颜色附件交给 ImGui 显示  
4. Edit 模式下 ImGuizmo 操作选中实体 Transform  

（3D 网格、天空盒等因项目与场景内容而异；2D 批处理为主路径。）
