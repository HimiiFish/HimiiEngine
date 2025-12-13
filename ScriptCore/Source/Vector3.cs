using System.Runtime.InteropServices;

namespace Himii
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float X, Y, Z;

        public Vector3(float x, float y, float z) { X = x; Y = y; Z = z; }
        public Vector3(Vector2 xy, float z) { X = xy.X; Y = xy.Y; Z = z; }

        public static Vector3 Zero => new Vector3(0.0f, 0.0f, 0.0f);

        public Vector2 XY => new Vector2(X, Y);

        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        public static Vector3 operator *(Vector3 vector, float scalar) => new Vector3(vector.X * scalar, vector.Y * scalar, vector.Z * scalar);

        public override string ToString() => $"Vector3({X}, {Y}, {Z})";
    }
}