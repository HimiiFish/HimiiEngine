using System;
using System.Runtime.InteropServices;

namespace Himii
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float X, Y, Z;
        public Vector3(float x, float y, float z) { X = x; Y = y; Z = z; }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFunctionsMap
    {
        public IntPtr LogFunc;
        public IntPtr Entity_HasComponent;
        public IntPtr Transform_GetTranslation;
        public IntPtr Transform_SetTranslation;
    }
}