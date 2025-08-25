# 场景序列化实现思路（YAML）

本文档总结当前 Scene 序列化/反序列化（YAML）的设计目标、数据模型、YAML Schema、关键实现与扩展点，方便维护与后续接入编辑器 UI。

## 目标与范围

- 目标：将场景中的实体与核心组件保存为可读可编辑的 `.yaml`，并支持从 `.yaml` 还原。
- 范围（当前版本）：
  - 仅序列化包含 `Transform` 的实体（作为最小存在条件）。
  - 组件覆盖：`ID`（如存在）、`Tag`、`Transform`、`SpriteRenderer`、`CameraComponent`。
  - 未覆盖：纹理资源路径/UV 细节（仅保留颜色与 tiling）、脚本/网格资源、层级关系等。

## 数据模型与字段

- Entity 最低要求：必须有 `Transform` 才会被写入。
- 组件字段映射：
  - ID：`ID.id`（UUID，若存在则写入 `ID` 字段，类型为 uint64）
  - Tag：`Tag.name` → `Tag`
  - Transform：
    - `Position: [x, y, z]`
    - `Rotation: [x, y, z]`（欧拉角，单位：弧度/度 取决于项目内部约定；当前实现与引擎一致）
    - `Scale: [x, y, z]`
  - SpriteRenderer：
    - `Color: [r, g, b, a]`
    - `Tiling: float`
  - Camera：
    - `Primary: bool`
    - `Projection: "Perspective" | "Orthographic"`
    - 透视：`FovYDeg: float`
    - 正交：`OrthoSize: float`（以可视高度为基准的缩放/zoom 值）
    - 裁剪：`NearZ: float`, `FarZ: float`
    - 朝向控制：
      - `UseLookAt: bool`
      - `LookAtTarget: [x,y,z]`
      - `Up: [x,y,z]`

## YAML Schema 示例

```yaml
Scene: Untitled
Entities:
  - ID: 123456789        # 可选，存在 ID 组件才写入
    Tag: "Camera"
    Transform:
      Position: [0, 0, 5]
      Rotation: [0, 0, 0]
      Scale:    [1, 1, 1]
    Camera:
      Primary: true
      Projection: Perspective
      FovYDeg: 45.0
      OrthoSize: 10.0
      NearZ: 0.1
      FarZ: 100.0
      UseLookAt: false
      LookAtTarget: [0, 0, 0]
      Up: [0, 1, 0]

  - Tag: "Quad"
    Transform:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale:    [1, 1, 1]
    SpriteRenderer:
      Color: [1, 0.5, 0.2, 1]
      Tiling: 1.0
```

## 关键实现细节

文件位置：
- `Engine/src/Himii/Scene/SceneSerializer.h/.cpp`

### 1) 基于 yaml-cpp 的读写

- 依赖：`yaml-cpp`（已在 `Engine/CMakeLists.txt` 中 `find_package` 并链接）。
- 写入使用 `YAML::Emitter`，读取使用 `YAML::LoadFile`。

### 2) GLM 向量的序列化支持

- 在 `SceneSerializer.cpp` 内提供 `YAML::convert<glm::vec3/vec4>` 与 `Emitter` 运算符重载：
  - 写入为 Flow 序列（`[x, y, z]` / `[x, y, z, w]`）。
  - 读取时验证为定长序列并解析为 `glm` 向量。

### 3) Serialize 流程（写入）

- 遍历 `entt::registry` 内所有实体：
  - 过滤：若实体无 `Transform`，跳过（保证最小有效实体）。
  - 按组件依次写入对应字段（见“数据模型与字段”）。
- 最终写入到指定路径的文本文件。

伪代码（与实现一致）：
- BeginMap: Scene/Entities
- For each entity with Transform:
  - BeginMap: Entity
    - if ID: write `ID`
    - if Tag: write `Tag`
    - if Transform: write Position/Rotation/Scale
    - if SpriteRenderer: write Color/Tiling
    - if Camera: write Primary/Projection/Fov/OrthoSize/Near/Far/UseLookAt/LookAtTarget/Up
  - EndMap
- EndSeq/EndMap, flush to file

### 4) Deserialize 流程（读取）

- 读取 `Entities` 数组，逐个创建实体：
  - 目前默认使用 `Scene::CreateEntity(name)` 生成新 UUID。
  - 依次检测并还原 Transform、SpriteRenderer、Camera 字段。
- 注意：当前版本未写回 `ID`（即使 YAML 中存在 `ID`），因此会生成新的 UUID。

改进建议（已在引擎具备能力）：
- 当 YAML 存在 `ID` 时，调用 `Scene::CreateEntityWithUUID(uuid, name)`，确保稳定 ID 回放；
- 同时更新 `Scene::m_EntityMap`（`CreateEntityWithUUID` 已内部处理）。

## 相机与渲染的配合

- `CameraComponent` 在 `Scene::OnUpdate` 中被读取：
  - 根据 `Projection` 自动刷新投影矩阵：
    - 透视：`SetFovYDeg(fov)`
    - 正交：`SetOrthographicBySize(orthoSize, near, far)`（保持垂直尺寸，水平按纵横比）
  - 视图矩阵：
    - `UseLookAt == true`：使用 `glm::lookAt(Position, LookAtTarget, Up)`；
    - 否则：通过 `Transform` 欧拉角设置相机旋转与位置，避免 `SetPosition` 影响旋转。

## 错误处理与边界情况

- 打开文件/解析失败：返回 `false`，外层可据此提示用户。
- YAML 结构缺失：若缺 `Entities`，反序列化返回 `false`。
- 字段缺失：采用“存在则解析，否则使用组件默认值”的模式（例如 `Tiling`、`FovYDeg` 等）。
- 非法向量：`convert<>` 会校验序列长度，失败则不覆盖默认值。

## 现状与待办

- 已完成：
  - 序列化/反序列化核心逻辑（ID/Tag/Transform/SpriteRenderer/Camera）。
  - `Engine` 构建已链接 `yaml-cpp`。
- 待完成：
  - 编辑器 UI 接入：添加菜单项“File > Save Scene… / Open Scene…”，调用 `SceneSerializer`；
  - 反序列化使用 `CreateEntityWithUUID` 保持稳定 ID；
  - 扩展序列化内容：
    - `SpriteRenderer.texture`（资源路径/句柄）、`uvs`；
    - `NativeScriptComponent`（脚本类名与可序列化参数）；
    - `MeshRenderer`（网格/材质/纹理资源标识）；
    - 实体层级/关系（若引入 Parent/Child 组件）；
    - 选择/拾取所需的稳定 `PickID`（若与选择系统耦合）。

## 与编辑器的集成建议

- 保存：
  - 从当前 `Scene*` 构造 `SceneSerializer`，`Serialize(path)`。
- 打开：
  - 可先清空场景，再 `Deserialize(path)`；或提供“追加合并”模式（需要冲突策略与 UUID 处理）。
- Windows 文件对话框：
  - 可使用现有跨平台封装（若有），或临时集成原生对话框/ImGui file dialog。

## 测试建议

- 最小回放：
  - 仅 Transform 的实体 1~2 个；保存→清空→读取→比对位置/旋转/缩放。
- SpriteRenderer：
  - 颜色、tiling 的保存与恢复；
- Camera：
  - 透视/正交切换、FOV/OrthoSize、Near/Far、UseLookAt 路径回放；
- UUID 稳定性（在迁移到 `CreateEntityWithUUID` 后）：
  - 写入 ID→读取后比对 ID 是否保持一致。

## 代码指引

- 入口：`SceneSerializer::Serialize/Deserialize`
- 组件定义：`Engine/src/Himii/Scene/Components.h`
- 场景入口与实体创建：`Engine/src/Himii/Scene/Scene.h/.cpp`
- 相机基类与编辑器相机：`Engine/src/Himii/Scene/SceneCamera.*`、`Engine/src/Himii/Renderer/EditorCamera.*`

---

如需扩展更多组件的序列化，请遵循“存在则写入/读取，缺省使用默认值”的策略，并尽量使用资源标识（路径/UUID）而非裸指针，以便跨会话还原。
