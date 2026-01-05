using System;
using System.Runtime.InteropServices;

namespace Himii
{
    public class Entity
    {
        public ulong ID { get; internal set; }

        protected Entity() { ID = 0; }

        internal Entity(ulong id) { ID = id; }

        public static Entity Create(string name)
        {
            IntPtr namePtr = Marshal.StringToCoTaskMemUTF8(name);
            ulong id = InternalCalls.Scene_CreateEntity(namePtr);
            Marshal.FreeCoTaskMem(namePtr);
            return new Entity(id);
        }

        public static void Destroy(Entity entity)
        {
            if (entity != null)
                InternalCalls.Scene_DestroyEntity(entity.ID);
        }

        public static Entity Find(string name)
        {
            IntPtr namePtr = Marshal.StringToCoTaskMemUTF8(name);
            ulong id = InternalCalls.Scene_FindEntityByName(namePtr);
            Marshal.FreeCoTaskMem(namePtr);

            if (id == 0) return null;
            return new Entity(id);
        }

        // --- Component Accessors ---

        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetTranslation(ID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetTranslation(ID, ref value);
        }

        // 缓存 Transform 实例，避免每次访问 entity.Transform 都产生垃圾回收 (GC)
        private Transform _transform;

        public Transform Transform
        {
            get
            {
                if (_transform == null)
                {
                    // 这里我们手动创建 Transform 而不调用 GetComponent<Transform>
                    // 因为每个 Entity 必定有 Transform，且 GetComponent 通常会创建一个新对象
                    _transform = new Transform();
                    _transform.Entity = this;
                }
                return _transform;
            }
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