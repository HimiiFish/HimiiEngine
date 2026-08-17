# 2D 物理系统 (Physics 2D)

HimiiEngine 内置 **Box2D v3** 作为 2D 物理引擎。

平台角色与地面配置示例见 **[教程：2D 平台跳跃](Tutorial2DPlatformer.md)**；编辑器模式说明见 **[编辑器与功能](EditorFeatures.md)**。

## 启用物理

为实体添加以下组件（通常同时需要 **Transform**）：

### Rigidbody 2D

定义刚体类型与旋转锁定。

| Body Type | 说明 |
|-----------|------|
| **Static** | 不移动（地面、墙等） |
| **Dynamic** | 受力与碰撞影响（玩家、可推动物体） |
| **Kinematic** | 主要由脚本/编辑器驱动，模拟行为与 Dynamic 不同 |

- **Fixed Rotation**：勾选后锁定 Z 轴旋转。
- **Body Type**、**Fixed Rotation** 可在运行时通过脚本读写（见 [脚本 API - Rigidbody2D](ScriptingAPI.md#rigidbody2d)）。

### Box Collider 2D

矩形碰撞形状。

- **Offset** / **Size**：相对 Transform 的偏移与尺寸。
- **Density**、**Friction**、**Restitution**、**Restitution Threshold**：密度、摩擦、弹性及弹性阈值。
- **Is Trigger**：勾选后为传感器，不产生物理碰撞响应，触发 `OnTriggerEnter2D` / `OnTriggerExit2D`。
- **Layer**：物理层（0–7），决定与哪些层发生碰撞。

### Circle Collider 2D

圆形碰撞形状。

- **Offset**、**Radius**：圆心与半径。
- 材料参数、**Is Trigger**、**Layer** 同 Box Collider。

**Box / Circle Collider** 可单独挂在实体上（无需 Rigidbody2D）：进入 **Play** 时引擎会为其自动创建 **Static** 刚体，适合地面、墙等静止碰撞体。

**Player** 等需要移动、受力的对象应使用 **Rigidbody2D（Dynamic 或 Kinematic）** + 碰撞体。

碰撞双方至少一方需参与物理模拟（有刚体，或仅有碰撞体时隐式 Static 刚体）。进入 **Play** 或 **Simulate** 时创建物理世界；**Stop** 后销毁。

## Tilemap 碰撞（Tilemap Collider 2D）

用于由瓦片组成的地面/墙体，**不需要** Rigidbody2D。

1. 实体上已有 **Tilemap** 组件并指定 `.tilemap` 资源。
2. 在 **TileMap Setup → Collision** 中为**图块类型**（普通图块或 Rule Tile，非场景里已画的格子）勾选 **Collidable**，并 **Save TileSet**。
3. 为同一实体添加 **Tilemap Collider 2D** 组件（Inspector 中可关闭 **Enabled**）。
4. 进入 **Play** 或 **Simulate** 后，引擎会为所有 `Collidable` 且非空的格子创建静态 Box2D 碰撞形状。

说明：

- **Collidable** 是 TileSet 里每种图块类型的属性，不是单个场景格子的属性。格子若是 Rule Tile，碰撞用该 Rule Tile 的 Collidable，不随边角精灵变化。
- **Slice Grid** 会按图集坐标 `(列, 行)` 保留同位置普通图块的 Collidable 设置，且不会清除 Rule Tile。
- Inspector 中 **Tilemap Collider 2D** 显示诊断：已绘制格子数、将参与碰撞的格子数、未知图块 ID 数。
- 可选 **Merge Adjacent Cells**：合并相邻可碰撞格为更少的大矩形（大地图性能更好，默认关闭）。
- 未添加 **Tilemap Collider 2D** 的 Tilemap **不会**产生物理碰撞。
- 编辑器菜单 **Show physics colliders** 可对 Collidable 格子显示绿色线框（与运行时判定一致）。
- 运行时修改瓦片或 TileSet 后，需重新 **Play** 才会重建碰撞（与 Box/Circle Collider 相同）。
- 控制台若提示 unknown tile IDs：地图上存的图块 ID 与当前 TileSet 不一致，请重新 Slice 或重画。

## 脚本控制刚体

实体上若有 `Rigidbody2D` 组件，可通过脚本访问：

```csharp
var rb = GetComponent<Rigidbody2D>();
if (rb != null)
{
    rb.BodyType = Rigidbody2DBodyType.Dynamic;
    rb.FixedRotation = true;
    rb.Velocity = new Vector2(3.0f, 0.0f);
    rb.ApplyImpulse(new Vector2(0.0f, 5.0f), wake: true);
}
```

Transform 与刚体位置在物理步进后会同步（Dynamic 物体以物理结果为准）。

## 碰撞事件（脚本）

在 **Play** 模式下，两个形状开始/结束接触时，引擎会调用脚本虚方法（双方实体若挂有 `ScriptComponent` 且已实例化，各收到一次回调，对方 ID 在 `Collision2DInfo.OtherEntityID` 中）：

```csharp
public override void OnCollisionEnter2D(Collision2DInfo collision)
{
    Log.Info($"Enter: {collision.OtherEntityID}");
}

public override void OnCollisionExit2D(Collision2DInfo collision)
{
    Log.Info($"Exit: {collision.OtherEntityID}");
}
```

### 触发器事件（脚本）

至少一方碰撞体勾选 **Is Trigger** 时，接触/分离会调用：

```csharp
public override void OnTriggerEnter2D(Collision2DInfo collision)
{
    Log.Info($"Trigger enter: {collision.OtherEntityID}");
}

public override void OnTriggerExit2D(Collision2DInfo collision)
{
    Log.Info($"Trigger exit: {collision.OtherEntityID}");
}
```

触发器不产生物理推力，适合拾取区、伤害区、关卡出口等。

## 物理层 (Physics 2D Layers)

项目支持 **8 个物理层**（索引 0–7）。在 **Project Settings → Physics 2D Layers** 中：

- 编辑每层显示名称（Inspector 中 Collider **Layer** 下拉使用）。
- 配置 **Collision Matrix**：勾选表示两层之间会发生碰撞/触发检测。

Collider 的 **Layer** 字段决定该形状所属层；未勾选的层组合将完全忽略彼此（碰撞与触发均不会触发）。

> **【配图占位】** `images/physics2d-layers-matrix.png` — Project Settings → Physics 2D Layers 与碰撞矩阵

保存项目（**File → Save Project**）后层配置写入 **`.hproj`** 项目文件。

注意：

- 仅在 **Play** 模式触发（**Simulate** 不运行脚本实例）。
- 双方需有碰撞体；动态交互至少一方通常为 **Dynamic** 或 **Kinematic**。
- 使用 `Log` 可在 **Console** 面板查看输出。

## 与编辑器的配合

- **Edit**：编辑碰撞体参数，不运行物理。
- **Simulate**：预览物理，不调用 `OnCreate` / `OnUpdate` / 碰撞脚本。
- **Play**：完整运行时（脚本 + 物理 + 碰撞回调）。
