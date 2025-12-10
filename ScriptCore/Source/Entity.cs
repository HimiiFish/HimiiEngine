namespace Himii
{
    public class Entity
    {
        public ulong ID { get; internal set; }

        public Vector3 Position
        {
            get
            {
                Interop.GetTranslation(ID, out Vector3 result);
                return result;
            }
            set
            {
                Interop.SetTranslation(ID, ref value);
            }
        }

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float ts) { }
    }
}