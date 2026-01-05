# 编辑器界面 (Editor Interface)

HimiiEditor 的界面布局旨在提供直观的游戏开发体验。

## 主要面板

### 1. 视口 (Viewport)
视口是您查看和编辑 3D/2D 场景的主要区域。
*   **右键按住 + WASD**：漫游场景。
*   **滚轮**：缩放。

### 2. 场景层级 (Scene Hierarchy)
显示当前场景中所有的实体 (Entity)。
*   **右键**：创建新实体 (Create Empty, Camera, Sprite 等)。
*   **拖拽**：改变父子关系。

### 3. 属性面板 (Properties)
显示当前选中实体的组件 (Component) 详细信息。
*   在此处修改 Transform, Tag, SpriteRenderer 等组件参数。
*   点击 **Add Component** 按钮为实体添加新功能。

### 4. 内容浏览器 (Content Browser)
管理项目资源（图片、脚本、场景文件等）。
*   对应于项目根目录下的 `Assets` 文件夹。
*   拖拽资源到视口或属性面板即可使用。
