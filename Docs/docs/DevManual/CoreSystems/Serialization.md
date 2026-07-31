# 场景序列化（YAML）

本文档描述 `.himii` 场景文件的序列化/反序列化设计，实现位于 `Engine/src/World/Scene/SceneSerializer.cpp`。

## 目标

- 将场景中实体与组件保存为可读 YAML（`.himii`）。
- 支持从文件还原场景，并在编辑器中 **File → Open Scene** / **Save Scene As** 使用。

## 场景 vs 资产序列化

| 种类 | 典型扩展名 | 谁实现 | 谁分发 |
|------|------------|--------|--------|
| **场景实体图** | `.himii`（作为场景打开时） | `World/Scene/SceneSerializer` | 编辑器 / 项目 Start Scene 直接调用 |
| **领域资产** | `.anim`、`.tileset`、`.tilemap`、`.particle`、纹理、字体、声音等 | 各 `Module/**` 内的 `IAssetSerializer` 实现 | `Resource/AssetSerializerRegistry`（`ResourceModule` 启动时注册） |

- `ResourceSystem` 提供 Import / GetAsset / AssetRegistry 等门面；**具体字段格式**仍在 Module（或 Scene）序列化器中。
- 扩展名到 `AssetType`、类型到反序列化实现的映射见 `Resource/BuiltinAssetSerializerRegistration.cpp`。
- `.himii` 也可作为资产类型出现在注册表中；**编辑器打开场景**走的是 `SceneSerializer`，不要与「仅当资源导入」路径混淆。

分层边界见 [架构概览 · Resource](../Architecture.md#8-resource-与序列化边界)。

## 文件格式概览

```yaml
Scene: ExampleScene
Entities:
  - Entity: 1234567890123456789
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    ScriptComponent:
      ClassName: MyGame.PlayerController
      ScriptFields:
        - Name: Speed
          Type: 5          # ScriptFieldType 枚举整型
          Data: 8.0
        - Name: Target
          Type: 12         # Entity
          Data: 9876543210987654321
    SpriteRendererComponent:
      Color: [1, 1, 1, 1]
      TexturePath: assets/textures/player.png
      TilingFactor: 1.0
    Rigidbody2DComponent:
      BodyType: 1
      FixedRotation: false
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
      Density: 1.0
      Friction: 0.5
      Restitution: 0.0
      RestitutionThreshold: 0.5
```

实际键名与 `SceneSerializer` 中 `YAML::Key` 一致（如 `TagComponent`、`TransformComponent` 等）。

## 已序列化的组件（当前）

| 组件 | 说明 |
|------|------|
| **Entity** (UUID) | `entity["Entity"]`，反序列化用 `CreateEntityWithUUID` |
| **TagComponent** | 实体名称 |
| **TransformComponent** | Position / Rotation / Scale |
| **CameraComponent** | 投影、裁剪、背景色、Primary 等 |
| **ScriptComponent** | `ClassName` + `ScriptFields`（含 Entity 引用 UUID） |
| **SpriteRendererComponent** | Color、SpriteAssetHandle、TilingFactor、FlipHorizontal |
| **SpriteAnimationComponent** | AnimationHandle、CurrentAnimationName、FrameRate、Playing、PreviewInScene |
| **CircleRendererComponent** | Color、Thickness、Fade |
| **MeshComponent** | Type、Color |
| **Rigidbody2DComponent** | BodyType、FixedRotation |
| **BoxCollider2DComponent** | Offset、Size、材料参数 |
| **CircleCollider2DComponent** | Offset、Radius、材料参数 |
| **UITransform / UIImage / UIText** 等 | UI 实体分支（`CreateUIEntityWithUUID`）；UIImage 使用 TextureHandle |

其他组件（粒子、Tilemap、动画等）若已在 `SerializeEntity` 中实现，以源码为准；未列出的组件在保存时不会写入。

## ScriptFields

- 与编辑器 Inspector 中 `ScriptComponent::Fields` 一致。
- `Type` 为 `ScriptFieldType` 整型；`Data` 类型随字段变化（标量、向量、UUID、字符串等）。
- 游戏脚本 **public 实例字段** 与 **`[SerializeField]` private 字段**（`HimiiEngine.SerializeField`，定义于 ScriptCore）参与反射与保存；`static` / `readonly` / `[NonSerialized]` 字段不参与。

## 动画资产（`.anim`）

由 `Module/Animation` 侧序列化器经 `AssetSerializerRegistry` 读写，非场景 YAML 内嵌。

- 新格式：`AssetType: SpriteFrames`，`Animations[]` 每条含 `Name`、`FrameRate`、`LoopMode`、`AtlasFrameCoordinates`。
- 旧格式：`AssetType: SpriteAnimation` 加载时迁移为单条 `default` 动画。

用户说明见 [2D 逐帧动画](../../UserManual/SpriteAnimation.md)。

## 实现要点

### 写入 (`Serialize`)

1. 遍历 `registry` 中带 `TransformComponent` 的实体（或 UI 实体路径）。
2. `SerializeEntity` 按组件存在性逐项 `Emitter` 输出。
3. 写入磁盘路径由 `SceneSerializer::Serialize(path)` 指定。

### 读取 (`Deserialize`)

1. `YAML::LoadFile` 解析 `Entities` 序列。
2. 每个条目调用 `DeserializeEntity`：
   - 读取 `Entity` UUID → `CreateEntityWithUUID` / `CreateUIEntityWithUUID`
   - 按块还原各组件字段
3. 缺失字段保留组件默认值。

### GLM 向量

`SceneSerializer.cpp` 内对 `glm::vec2/vec3/vec4` 提供 `YAML::convert` 与 Emitter 支持，格式为 Flow 序列 `[x, y, z]`。

## 编辑器集成

- **File → Open Scene**：加载 `.himii` 到当前编辑场景。
- **File → Save Scene As**：`EditorLayer::SerializeScene` → `SceneSerializer::Serialize`。
- 项目 **Start Scene** 在 `.hproj` 中配置，打开项目时可自动加载。

## 错误处理

- 文件打开/解析失败：`Deserialize` 返回 `false`。
- 缺少 `Entities` 节点：反序列化失败。
- 可选字段：有则解析，无则跳过。

## 待扩展（真实缺口）

- **Prefab** 引用与实例化覆盖
- 部分高级组件的统一资源句柄格式（AssetHandle vs 路径混用时的规范化）
- **实体父子层级** 若引入 Parent 组件，需在序列化中增加关系块
- 更完整的 **NativeScriptComponent** / 程序集拆分（多 asmdef）

## 测试建议

1. 空场景 + 单实体 Transform：保存 → 重新打开 → 比对变换。
2. 带 `ScriptComponent` 与 public 字段：Inspector 修改 → 保存 → 重开 → 字段与 Entity 引用一致。
3. 2D 物理组件：保存后碰撞体参数一致。
4. Play 模式前无需重新手动挂脚本类名（类名在 YAML 中）。

## 代码索引

- `SceneSerializer::Serialize` / `Deserialize` / `SerializeEntity` / `DeserializeEntity`
- `Engine/src/World/Scene/Components.h`
- `Engine/src/World/Scene/Scene.h`
- `Engine/src/Resource/AssetSerializerRegistry.h`、`IAssetSerializer.h`
- `Engine/src/Resource/BuiltinAssetSerializerRegistration.cpp`
