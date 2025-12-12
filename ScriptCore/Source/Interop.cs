using System;
using System.Runtime.InteropServices;

namespace Himii
{
    public static class Interop
    {
        // 内部委托定义
        internal delegate void LogFuncDelegate(IntPtr msg);
        internal delegate void TransformPosDelegate(ulong entityID, out Vector3 vec); // out 用于返回
        internal delegate void TransformSetPosDelegate(ulong entityID, ref Vector3 vec); // ref 用于输入

        // 保存委托实例
        internal static LogFuncDelegate NativeLog;
        internal static TransformPosDelegate GetTranslation;
        internal static TransformSetPosDelegate SetTranslation;

        [UnmanagedCallersOnly]
        public static void Initialize(IntPtr functionTablePtr)
        {
            // 1. 从指针读取结构体
            var funcs = Marshal.PtrToStructure<NativeFunctionsMap>(functionTablePtr);

            // 2. 将 IntPtr 转为 Delegate
            NativeLog = Marshal.GetDelegateForFunctionPointer<LogFuncDelegate>(funcs.LogFunc);
            GetTranslation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetTranslation);
            SetTranslation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetTranslation);

            Console.WriteLine("[C#] Interop initialized. Functions bound.");
        }
    }

    public static class Debug
    {
        public static void Log(string msg)
        {
            Interop.NativeLog(Marshal.StringToHGlobalAnsi(msg));
        }
    }
}