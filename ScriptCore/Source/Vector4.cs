using System.Runtime.InteropServices;

namespace Himii
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4
    {
        public float X, Y, Z, W;
        public Vector4(float x, float y, float z, float w) { X = x; Y = y; Z = z; W = w; }
    }
}