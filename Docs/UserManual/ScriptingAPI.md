# 脚本 API (Scripting API)

HimiiEngine 使用 C# 作为脚本语言。

## 创建脚本

1.  在 **Content Browser** 中右键 -> Create -> C# Script。
2.  命名脚本（例如 `PlayerController.cs`）。
3.  双击打开（通常会启动 Visual Studio 或 VS Code）。

## Entity 生命周期

继承自 `Himii.Entity` 的类可以重写以下方法：

```csharp
using Himii;

public class PlayerController : Entity
{
    // 在实体创建时调用（游戏开始时）
    void OnCreate()
    {
        Console.WriteLine($"Entity {ID} Created!");
    }

    // 每帧调用
    void OnUpdate(float ts)
    {
        // ts (Timestep) 是上一帧的时间间隔，用于帧率无关的移动
    }

    // 在实体销毁时调用
    void OnDestroy()
    {
    }
}
```

## 输入 (Input)

使用 `Input` 类获取键盘和鼠标输入：

```csharp
if (Input.IsKeyDown(KeyCode.W))
{
    // 向前移动
}
```

## 组件交互

获取当前实体的组件：

```csharp
var transform = GetComponent<TransformComponent>();
transform.Translation += new Vector3(1.0f, 0.0f, 0.0f) * ts;
```
