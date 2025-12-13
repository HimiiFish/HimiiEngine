using System;
using System.Runtime.InteropServices;

namespace Himii
{
    public static class InternalCalls
    {
        internal delegate void LogFuncDelegate(IntPtr msg);
        internal delegate bool EntityHasComponentDelegate(ulong entityID, int typeHashCode);

        internal delegate void TransformPosDelegate(ulong entityID, out Vector3 vec);
        internal delegate void TransformSetPosDelegate(ulong entityID, ref Vector3 vec);

        internal delegate bool InputGetKeyDelegate(KeyCode keycode);

        internal delegate void Rigidbody2DApplyImpulseDelegate(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);
        internal delegate void Rigidbody2DApplyImpulseCenterDelegate(ulong entityID, ref Vector2 impulse, bool wake);

        internal static LogFuncDelegate NativeLog;
        internal static EntityHasComponentDelegate Entity_HasComponent;
        internal static TransformPosDelegate Transform_GetTranslation;
        internal static TransformSetPosDelegate Transform_SetTranslation;
        internal static InputGetKeyDelegate Input_IsKeyDown;
        internal static Rigidbody2DApplyImpulseDelegate Rigidbody2D_ApplyLinearImpulse;
        internal static Rigidbody2DApplyImpulseCenterDelegate Rigidbody2D_ApplyLinearImpulseToCenter;

        [UnmanagedCallersOnly]
        public static void Initialize(IntPtr functionTablePtr)
        {
            var funcs = Marshal.PtrToStructure<NativeFunctionsMap>(functionTablePtr);

            NativeLog = Marshal.GetDelegateForFunctionPointer<LogFuncDelegate>(funcs.LogFunc);
            Entity_HasComponent = Marshal.GetDelegateForFunctionPointer<EntityHasComponentDelegate>(funcs.Entity_HasComponent);

            Transform_GetTranslation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetTranslation);
            Transform_SetTranslation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetTranslation);

            Input_IsKeyDown = Marshal.GetDelegateForFunctionPointer<InputGetKeyDelegate>(funcs.Input_IsKeyDown);

            Rigidbody2D_ApplyLinearImpulse = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseDelegate>(funcs.Rigidbody2D_ApplyLinearImpulse);
            Rigidbody2D_ApplyLinearImpulseToCenter = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseCenterDelegate>(funcs.Rigidbody2D_ApplyLinearImpulseToCenter);

            Console.WriteLine("[C#] InternalCalls initialized.");
        }
    }
}