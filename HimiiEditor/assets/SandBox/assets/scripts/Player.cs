using Himii;

class Player:Entity
{
    public float speed = 1.0f;
    public override void OnUpdate(float ts)
    {
        if(Input.IsKeyDown(KeyCode.A))
        {
            Position -= new Vector3(-speed, 0, 0);
        }
        if(Input.IsKeyDown(KeyCode.D))
        {
            Position += new Vector3(speed, 0, 0);
        }
        if(Input.IsKeyDown(KeyCode.Space))
        {
            if(HasComponent<Rigidbody2DComponent>())
            {
                var rigidbody2D = GetComponent<Rigidbody2DComponent>();
                rigidbody2D.ApplyLinearImpulseToCenter(new Vector2(0, 1.0f), true);
            }
        }
    }
}