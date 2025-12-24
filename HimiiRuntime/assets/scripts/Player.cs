using Himii;

public class Player:Entity
{
    public override void OnCreate()
    {
        base.OnCreate();
        Console.WriteLine("Player created");
    }

    public override void OnUpdate(float deltaTime)
    {
        base.OnUpdate(deltaTime);
        
        if(Input.IsKeyDown(KeyCode.Space))
        {
            if(HasComponent<Rigidbody2DComponent>())
            {
                var rb = GetComponent<Rigidbody2DComponent>();
                rb.ApplyLinearImpulseToCenter(new Vector2(0, 1f),true);
            }
        }
    }
}