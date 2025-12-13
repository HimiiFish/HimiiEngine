namespace Himii
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class TransformComponent : Component
    {
        public Vector3 Translation
        {
            get
            {
                InternalCalls.Transform_GetTranslation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(Entity.ID, ref value);
        }

        // 旋转和缩放的逻辑类似，需要在 Interop 中补充
    }

    public class Rigidbody2DComponent : Component
    {
        public void ApplyLinearImpulse(Vector2 impulse, Vector2 point, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulse(Entity.ID, ref impulse, ref point, wake);
        }

        public void ApplyLinearImpulseToCenter(Vector2 impulse, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulseToCenter(Entity.ID, ref impulse, wake);
        }
    }
}