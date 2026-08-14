# 脚本 API (Scripting API)

HimiiEngine 使用 **C#**（**.NET 8 + CoreCLR**）编写游戏逻辑。脚本编译为项目中的 **`GameAssembly.dll`**，由编辑器在 **Play** 时加载。所有公开 API 位于命名空间 **`HimiiEngine`**（引擎提供 **`ScriptCore.dll`**）。

```csharp
using HimiiEngine;
```

编写与编译流程见 [脚本工作流](ScriptWorkflow.md)；首个完整示例见 [教程：2D 平台跳跃](Tutorial2DPlatformer.md)。

---

## API 索引

| 类别 | 类型 / 成员 | 说明 |
|------|-------------|------|
| 基类 | `Entity` | 脚本实体基类 |
| 生命周期 | `OnCreate` / `OnUpdate` / `OnFixedUpdate` / `OnDestroy` | 仅 Play 模式调用 |
| 日志 | `Log.Info` / `Warning` / `Error` | 输出到 Console |
| 输入 | `Input.IsKeyDown` / `IsKeyPressed` / `IsKeyReleased` / `GetAxisHorizontal` / `GetAxisVertical` / `IsMouseButtonDown` / `MousePosition` | 键盘与鼠标 |
| 变换 | `Position` / `Transform` | 实体位置、旋转、缩放 |
| 组件 | `GetComponent<T>()` / `HasComponent<T>()` | 访问引擎组件 |
| 2D 物理 | `Rigidbody2D` | 速度、冲量、BodyType、FixedRotation |
| 碰撞体 | `BoxCollider2D` / `CircleCollider2D` | 偏移、尺寸/半径、材料参数、IsTrigger、Layer |
| 渲染 | `SpriteRenderer` | 颜色、纹理、翻转、SortingLayer/Order |
| 相机 | `Camera` | OrthographicSize、BackgroundColor、Primary |
| 动画 | `SpriteAnimation` | `Play` / `CurrentAnimation` |
| 瓦片地图 | `Tilemap` | 读写格子、边界查询 |
| 物理查询 | `Physics2D.Raycast` | 2D 射线检测 |
| 场景 | `SceneManager.LoadScene` / `ActiveScenePath` | 运行时切换 `.himii` 场景 |
| Prefab | `Entity.Instantiate` | 运行时实例化 `.hprefab` |
| 时间 | `Time.DeltaTime` | 帧间隔（秒） |
| 碰撞 | `OnCollisionEnter2D` / `OnCollisionExit2D` | Play 模式物理碰撞回调 |
| 触发器 | `OnTriggerEnter2D` / `OnTriggerExit2D` | Play 模式 Trigger 回调 |
| 序列化 | `[SerializeField]` | Inspector 显示 private 字段 |

---

## 创建与挂载脚本

1. **Content Browser** → `assets/scripts` → 右键 **Create → C# Script**。
2. 类继承 **`Entity`**，例如 `public class PlayerController : Entity`。
3. 在场景中选中实体 → **Add Component → Script** → **Class Name** 填写类名（与 `.cs` 中一致，区分大小写）。
4. **File → Open C# Project** 在外部 IDE 编辑；保存后 **Window → Console** 显示编译结果。

---

## Entity 生命周期

仅在 **Play** 模式、且实体带有有效 **Script** 组件时调用：

```csharp
using HimiiEngine;

public class PlayerController : Entity
{
    public override void OnCreate()
    {
        Log.Info($"Entity {ID} created.");
    }

    public override void OnUpdate(float timestep)
    {
        // timestep：上一帧间隔（秒），用于帧率无关移动
        Position += new Vector3(1.0f, 0.0f, 0.0f) * timestep;
    }

    public override void OnDestroy()
    {
        Log.Info("Entity destroyed.");
    }
}
```

| 方法 | 调用时机 |
|------|----------|
| `OnCreate` | 实体脚本实例化后一次 |
| `OnUpdate(timestep)` | 每帧（物理步进**之前**） |
| `OnFixedUpdate(timestep)` | 每帧物理步进与碰撞事件**之后** |
| `OnDestroy` | 实体销毁或 Stop 时 |

平台跳跃、着地检测、读写 `Rigidbody2D.Velocity` 等应放在 **`OnFixedUpdate`**；输入采集与相机可放在 `OnUpdate`。详见 [脚本工作流 - 物理时序](ScriptWorkflow.md#物理时序与平台跳跃重要)。

---

## 日志 (Log)

```csharp
Log.Info("Game started.");
Log.Warning("Low health!");
Log.Error("Something went wrong.");
```

- 显示在 **Window → Console**（进入 Play 时会清空运行时缓冲，编译输出保留）。
- 编译输出与 `Log` 在同一面板，用 **Compile** / **Script** 通道区分。
- 请勿依赖 `Console.WriteLine` 作为游戏日志（不会进入编辑器 Console）。

---

## 输入 (Input)

```csharp
if (Input.IsKeyDown(KeyCode.W))
{
    // 键按住（每帧为 true）
}

if (Input.IsKeyPressed(KeyCode.Space))
{
    // 本帧刚按下（边沿检测，适合跳跃）
}

if (Input.IsKeyReleased(KeyCode.E))
{
    // 本帧刚松开
}

float horizontal = Input.GetAxisHorizontal(); // A/D 或 Left/Right，-1 ~ 1
float vertical = Input.GetAxisVertical();     // W/S 或 Up/Down

Vector2 mouse = Input.MousePosition;
if (Input.IsMouseButtonDown(0))
{
    // 左键
}
```

| 方法 | 说明 |
|------|------|
| `IsKeyDown` | 键当前按住 |
| `IsKeyPressed` | 本帧刚按下（单帧 true） |
| `IsKeyReleased` | 本帧刚松开 |
| `GetAxisHorizontal` | 水平轴：Left/Right 或 A/D |
| `GetAxisVertical` | 垂直轴：Up/Down 或 W/S |

常用键码定义在 **`KeyCode`** 枚举（`A`、`D`、`Left`、`Right`、`Space` 等），完整列表见 `ScriptCore` 中 `KeyCode.cs`。

---

## Transform 与位置

每个实体均有 **Transform**。可通过实体快捷属性或组件访问：

```csharp
Position += new Vector3(1.0f, 0.0f, 0.0f) * timestep;

Transform.Scale = new Vector3(2.0f, 2.0f, 1.0f);
Transform.Rotation = new Vector3(0.0f, 0.0f, 0.0f);
```

**注意**：角色左右朝向请用 **`SpriteRenderer.FlipHorizontal`**，不要将 **`Transform.Scale.x`** 设为负数（会导致碰撞与渲染异常）。

在 **Play** 模式下，若实体带有 **Rigidbody2D**，修改 **`Position`** 或 **`Transform.Rotation`** 会同步到 Box2D 刚体（相当于传送）。Dynamic 角色日常移动仍推荐在 **`OnFixedUpdate`** 中使用 **`Rigidbody2D.Velocity`**。

---

## 组件访问

```csharp
var rigidbody = GetComponent<Rigidbody2D>();
if (rigidbody != null)
    rigidbody.Velocity = new Vector2(5.0f, 0.0f);

bool hasAnimation = HasComponent<SpriteAnimation>();
```

`GetComponent<T>()` 在实体未挂载该组件时返回 `null`。

### 静态工厂（可选）

```csharp
Entity player = Entity.Find("Player");
Entity spawned = Entity.Create("Enemy");
Entity.Destroy(spawned);
```

---

## Rigidbody2D

```csharp
var rigidbody = GetComponent<Rigidbody2D>();
if (rigidbody != null)
{
    rigidbody.Velocity = new Vector2(3.0f, 0.0f);
    rigidbody.ApplyImpulse(new Vector2(0.0f, 5.0f), wake: true);
}
```

| 成员 | 说明 |
|------|------|
| `Velocity` | 线速度（`Vector2`） |
| `BodyType` | `Static` / `Dynamic` / `Kinematic`（运行时读写） |
| `FixedRotation` | 是否锁定 Z 轴旋转 |
| `ApplyImpulse(impulse, wake)` | 对质心施加冲量 |
| `ApplyImpulse(impulse, point, wake)` | 在世界坐标点施加冲量 |

Inspector 中设置 **Body Type**（Static / Dynamic / Kinematic）、**Fixed Rotation** 等。详见 [2D 物理](Physics2D.md)。

---

## BoxCollider2D / CircleCollider2D

```csharp
var boxCollider = GetComponent<BoxCollider2D>();
if (boxCollider != null)
{
    Vector2 size = boxCollider.Size;
    boxCollider.Friction = 0.8f;
}

var circleCollider = GetComponent<CircleCollider2D>();
if (circleCollider != null)
    circleCollider.Radius = 0.5f;
```

| 组件 | 属性 |
|------|------|
| `BoxCollider2D` | `Offset`、`Size`、`Density`、`Friction`、`Restitution`、`IsTrigger`、`Layer` |
| `CircleCollider2D` | `Offset`、`Radius`、`Density`、`Friction`、`Restitution`、`IsTrigger`、`Layer` |

**IsTrigger**：勾选后形状为传感器，不产生物理碰撞响应，但会触发 `OnTriggerEnter2D` / `OnTriggerExit2D`。

**Layer**：物理层索引（0–7），与项目 **Project Settings → Physics 2D Layers** 中的层名及碰撞矩阵对应。

运行时修改碰撞体参数会更新组件数据；形状在下次进入 **Play** 时重建。详见 [2D 物理](Physics2D.md)。

---

## Tilemap

```csharp
var tilemap = GetComponent<Tilemap>();
if (tilemap != null && tilemap.HasBounds)
{
    tilemap.GetBounds(out int minX, out int minY, out int maxX, out int maxY);
    ushort tileId = tilemap.GetTile(0, 0);
    tilemap.SetTile(1, 0, tileId);
}
```

| 成员 | 说明 |
|------|------|
| `Width` / `Height` | 地图尺寸（稀疏分块模型的外接范围） |
| `HasBounds` | 是否存在有效格子 |
| `GetBounds` | 已绘制格子的包围盒 |
| `GetTile` / `SetTile` | 读写瓦片 ID |

---

## Physics2D

```csharp
RaycastHit2D hit = Physics2D.Raycast(start, end);
if (hit.Hit)
    Log.Info($"Hit entity {hit.EntityID} at {hit.Point}");
```

| 字段 | 说明 |
|------|------|
| `Hit` | 是否命中 |
| `Point` / `Normal` | 命中点与世界法线 |
| `Distance` | 沿射线距离 |
| `EntityID` | 命中实体 ID |

---

## SceneManager

```csharp
if (SceneManager.LoadScene("scenes/Level1.himii"))
{
    Log.Info("Loaded level 1.");
}

string current = SceneManager.ActiveScenePath;
```

- 路径相对于项目 **`assets/`** 目录。
- 仅在 **Play** 模式有效；会停止当前脚本、清空实体并加载新场景。
- `LoadScene` 返回 `bool` 表示是否成功。
- `ActiveScenePath` 返回当前场景的 assets 相对路径（未加载时为空）。
- 项目 **Start Scene** 在 `.hproj` 中配置。

---

## Prefab 实例化

```csharp
Entity enemy = Entity.Instantiate("prefabs/Enemy.hprefab");
if (enemy != null)
    enemy.Position = new Vector3(5.0f, 0.0f, 0.0f);
```

- 路径相对于 **`assets/`**。
- 仅在 **Play** 模式会为脚本调用 `OnCreate`。
- 编辑器中：Hierarchy 右键 **Save as Prefab**，或将 `.hprefab` 拖入 Viewport 实例化。

---

## Time

```csharp
float delta = Time.DeltaTime;
```

与 `OnUpdate(timestep)` 参数等价，便于在非生命周期方法中读取帧间隔。

---

## SpriteRenderer

```csharp
var spriteRenderer = GetComponent<SpriteRenderer>();
spriteRenderer.FlipHorizontal = true;
spriteRenderer.Color = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
spriteRenderer.TextureHandle = textureHandle;
```

| 属性 | 说明 |
|------|------|
| `Color` | `Vector4` 着色 |
| `SpriteAssetHandle` / `SpriteHandle` | 子 Sprite 资产句柄 |
| `TextureHandle` | 纹理句柄（可自动绑定默认子 Sprite） |
| `FlipHorizontal` | 水平镜像绘制 |
| `SortingLayer` | 排序层索引（越小越靠后绘制） |
| `SortingOrder` | 同层内的绘制顺序 |

Sorting Layer 名称在 **Project Settings → Sorting Layers** 中配置。

---

## Camera

```csharp
var camera = GetComponent<Camera>();
if (camera != null)
{
    camera.OrthographicSize = 8.0f;
    camera.BackgroundColor = new Vector4(0.1f, 0.1f, 0.15f, 1.0f);
    camera.Primary = true;
}
```

| 属性 | 说明 |
|------|------|
| `OrthographicSize` | 正交相机半高（2D 常用） |
| `BackgroundColor` | 清屏颜色 |
| `Primary` | 是否为主相机（渲染时使用） |

---

## SpriteAnimation

```csharp
var animation = GetComponent<SpriteAnimation>();
animation.Play("Run");
animation.CurrentAnimation = "Idle";
animation.FrameRate = 12.0f;
animation.Playing = true;
animation.Stop();
```

| API | 说明 |
|-----|------|
| `Play()` | 播放当前 `CurrentAnimation`，重置帧 |
| `Play(string animationName)` | 切换命名动画并播放 |
| `CurrentAnimation` | 与 `.anim` 内动画 **Name** 一致（区分大小写） |
| `Playing` | 是否推进帧 |
| `FrameRate` | 大于 0 时覆盖资产内默认 FPS |
| `Stop()` | 停止播放 |

资产与编辑器说明见 [2D 逐帧动画](SpriteAnimation.md)。

---

## 2D 碰撞回调

在 **Play** 模式下，碰撞开始/结束时调用（双方若挂 Script 且已实例化，各收到一次回调）：

```csharp
public override void OnCollisionEnter2D(Collision2DInfo collision)
{
    Log.Info($"Enter: {collision.OtherEntityID}");
}

public override void OnCollisionExit2D(Collision2DInfo collision)
{
    Log.Info($"Exit: {collision.OtherEntityID}");
}

public override void OnTriggerEnter2D(Collision2DInfo collision)
{
    Log.Info($"Trigger enter: {collision.OtherEntityID}");
}

public override void OnTriggerExit2D(Collision2DInfo collision)
{
    Log.Info($"Trigger exit: {collision.OtherEntityID}");
}
```

| 说明 | 细节 |
|------|------|
| 碰撞回调 | 双方 **IsTrigger 均未勾选** 时触发 `OnCollisionEnter2D` / `OnCollisionExit2D` |
| 触发器回调 | 至少一方 **IsTrigger 勾选** 时触发 `OnTriggerEnter2D` / `OnTriggerExit2D`（无物理推力） |
| 触发条件 | 双方有 **Box** 或 **Circle Collider 2D**；至少一方参与动态模拟 |
| 不触发 | **Simulate** 模式不运行脚本实例 |
| 用途 | 伤害、拾取、机关等；**平台着地优先用射线**（见 [脚本工作流](ScriptWorkflow.md)） |

`Collision2DInfo` 字段：

| 字段 | 说明 |
|------|------|
| `OtherEntityID` | 对方实体 ID |
| `Normal` | 支撑法线（指向本实体，世界空间）；脚下地面通常 `Normal.Y` 较大 |
| `Point` | 接触点世界坐标（首个 manifold 点） |

更多组件参数见 [2D 物理](Physics2D.md)。

---

## Inspector 与 [SerializeField]

**public 实例字段** 或带 **`[SerializeField]`** 的 **private 实例字段** 会出现在属性面板，并随场景 YAML 的 `ScriptFields` 保存：

```csharp
using HimiiEngine;

public class PlayerController : Entity
{
    public float MoveSpeed = 5.0f;
    public Entity Target;

    [SerializeField]
    private float jumpSpeed = 7.0f;
}
```

`[SerializeField]` 由 **ScriptCore.dll** 提供（`HimiiEngine.SerializeField`），游戏脚本只需 `using HimiiEngine;`，**无需**在项目内复制该文件。

| 规则 | 说明 |
|------|------|
| 命名空间 | 特性在 **`HimiiEngine`**，脚本需 `using HimiiEngine;` |
| 支持类型 | 数值、布尔、`Vector2/3/4`、`string`、`KeyCode`、`Entity` 等 |
| 不支持 | static 字段、属性（Property） |
| 持久化 | 修改后 **Ctrl+S** 保存场景 |
| 不显示 | 检查类名、重新编译 ScriptCore、重启编辑器后重选实体 |

> **【配图占位】** `images/inspector-script-serializefield.png`

---

## 相关文档

- [教程：2D 平台跳跃](Tutorial2DPlatformer.md)
- [编辑器与功能](EditorFeatures.md)
- [脚本工作流](ScriptWorkflow.md)
- [编辑器界面](EditorInterface.md)
- [2D 逐帧动画](SpriteAnimation.md)
- [2D 物理](Physics2D.md)
