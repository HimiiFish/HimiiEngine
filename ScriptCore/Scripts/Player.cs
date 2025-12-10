using System;
using Himii; // 引用引擎命名空间

// 继承自 Entity
public class Player : Entity
{
    private float m_Speed = 2.0f;

    // 相当于 Unity 的 Start()
    public override void OnCreate()
    {
        Console.WriteLine($"Player Created! ID: {ID}");

        // 测试一下初始位置
        Vector3 pos = Position;
        Console.WriteLine($"Initial Pos: {pos.X}, {pos.Y}, {pos.Z}");
    }

    // 相当于 Unity 的 Update()
    public override void OnUpdate(float ts)
    {
        Console.WriteLine($"Player Created! ID: {ID}");
        // 1. 获取当前位置
        Vector3 velocity = new Vector3(0.0f, 0.0f, 0.0f);

        // 2. 简单的移动逻辑 (模拟按键，因为我们还没做 Input)
        // 假设每一帧都向右上方移动
        velocity.X = 1.0f;
        velocity.Y = 1.0f;

        // 3. 应用速度 * 时间增量
        velocity.X *= m_Speed * ts;
        velocity.Y *= m_Speed * ts;

        // 4. 修改位置
        Vector3 pos = Position;
        pos.X += velocity.X;
        pos.Y += velocity.Y;

        // 5. 将新位置传回 C++
        Position = pos;
    }
}