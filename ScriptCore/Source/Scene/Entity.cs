using System;

namespace Himii
{
    public class Entity
    {
        public ulong ID { get; internal set; }

        protected Entity() { ID = 0; }

        internal Entity(ulong id) { ID = id; }

        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetTranslation(ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(ID, ref value);
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            int hashCode = typeof(T).FullName.GetHashCode();
            return InternalCalls.Entity_HasComponent(ID, hashCode);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T();
            component.Entity = this;
            return component;
        }

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float ts) { }
    }
}