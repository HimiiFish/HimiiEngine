using System.Runtime.InteropServices;

namespace Himii
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float X, Y;

        public Vector2(float x, float y) { X = x; Y = y; }

        public static Vector2 Zero => new Vector2(0.0f, 0.0f);

        public static Vector2 operator +(Vector2 a, Vector2 b) => new Vector2(a.X + b.X, a.Y + b.Y);
        public static Vector2 operator *(Vector2 vector, float scalar) => new Vector2(vector.X * scalar, vector.Y * scalar);

        public float LengthSquared() => X * X + Y * Y;
        public float Length() => (float)System.Math.Sqrt(LengthSquared());
    }
}