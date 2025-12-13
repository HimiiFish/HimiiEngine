using System;
using Himii;


public class Player : Entity
{
    public override void OnCreate()
    {
        Console.WriteLine("Player entity created.");
    }

    public override void OnUpdate(float ts)
    {
        if (Input.IsKeyDown(KeyCode.Space))
        {
            Console.WriteLine("Jump!");
            if (HasComponent<Rigidbody2DComponent>())
            {
                var rb = GetComponent<Rigidbody2DComponent>();
                rb.ApplyLinearImpulseToCenter(new Vector2(0, 10.0f), true);
            }
        }
    }
}

