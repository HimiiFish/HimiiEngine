namespace Himii
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class Transform : Component
    {
        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetTranslation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(Entity.ID, ref value);
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.Transform_GetRotation(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetRotation(Entity.ID, ref value);
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.Transform_GetScale(Entity.ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetScale(Entity.ID, ref value);
        }
    }

    public class Rigidbody2D : Component
    {
        public Vector2 Velocity
        {
            get
            {
                InternalCalls.Rigidbody2D_GetLinearVelocity(Entity.ID, out Vector2 result);
                return result;
            }
            set => InternalCalls.Rigidbody2D_SetLinearVelocity(Entity.ID, ref value);
        }

        public void ApplyImpulse(Vector2 impulse, Vector2 point, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulse(Entity.ID, ref impulse, ref point, wake);
        }

        public void ApplyImpulse(Vector2 impulse, bool wake)
        {
            InternalCalls.Rigidbody2D_ApplyLinearImpulseToCenter(Entity.ID, ref impulse, wake);
        }
    }
}