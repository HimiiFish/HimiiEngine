using System;
using System.Runtime.InteropServices;

namespace Himii
{
    public static class InternalCalls
    {
        // delegate definitions
        internal delegate void LogFuncDelegate(IntPtr msg);

        internal delegate bool EntityHasComponentDelegate(ulong entityID, int typeHashCode);
        internal delegate ulong SceneCreateEntityDelegate(IntPtr name);
        internal delegate void SceneDestroyEntityDelegate(ulong entityID);
        internal delegate ulong SceneFindEntityDelegate(IntPtr name);

        internal delegate void TransformPosDelegate(ulong entityID, out Vector3 vec);
        internal delegate void TransformSetPosDelegate(ulong entityID, ref Vector3 vec);

        internal delegate bool InputGetKeyDelegate(KeyCode keycode);
        internal delegate bool InputGetMouseBtnDelegate(int button);
        internal delegate void InputGetMousePosDelegate(out Vector2 pos);

        internal delegate void Rigidbody2DApplyImpulseDelegate(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);
        internal delegate void Rigidbody2DApplyImpulseCenterDelegate(ulong entityID, ref Vector2 impulse, bool wake);
        internal delegate void Rigidbody2DGetVelocityDelegate(ulong entityID, out Vector2 velocity);
        internal delegate void Rigidbody2DSetVelocityDelegate(ulong entityID, ref Vector2 velocity);

        // static fields to hold the delegates
        internal static LogFuncDelegate NativeLog;

        internal static EntityHasComponentDelegate Entity_HasComponent;
        internal static SceneCreateEntityDelegate Scene_CreateEntity;
        internal static SceneDestroyEntityDelegate Scene_DestroyEntity;
        internal static SceneFindEntityDelegate Scene_FindEntityByName;

        internal static TransformPosDelegate Transform_GetTranslation;
        internal static TransformSetPosDelegate Transform_SetTranslation;
        internal static TransformPosDelegate Transform_GetRotation;
        internal static TransformSetPosDelegate Transform_SetRotation;
        internal static TransformPosDelegate Transform_GetScale;
        internal static TransformSetPosDelegate Transform_SetScale;

        internal static InputGetKeyDelegate Input_IsKeyDown;
        internal static InputGetMouseBtnDelegate Input_IsMouseButtonDown;
        internal static InputGetMousePosDelegate Input_GetMousePosition;

        internal static Rigidbody2DApplyImpulseDelegate Rigidbody2D_ApplyLinearImpulse;
        internal static Rigidbody2DApplyImpulseCenterDelegate Rigidbody2D_ApplyLinearImpulseToCenter;
        internal static Rigidbody2DGetVelocityDelegate Rigidbody2D_GetLinearVelocity;
        internal static Rigidbody2DSetVelocityDelegate Rigidbody2D_SetLinearVelocity;

        [UnmanagedCallersOnly]
        public static void Initialize(IntPtr functionTablePtr)
        {
            var funcs = Marshal.PtrToStructure<NativeFunctionsMap>(functionTablePtr);

            NativeLog = Marshal.GetDelegateForFunctionPointer<LogFuncDelegate>(funcs.LogFunc);

            Entity_HasComponent = Marshal.GetDelegateForFunctionPointer<EntityHasComponentDelegate>(funcs.Entity_HasComponent);
            Scene_CreateEntity = Marshal.GetDelegateForFunctionPointer<SceneCreateEntityDelegate>(funcs.Scene_CreateEntity);
            Scene_DestroyEntity = Marshal.GetDelegateForFunctionPointer<SceneDestroyEntityDelegate>(funcs.Scene_DestroyEntity);
            Scene_FindEntityByName = Marshal.GetDelegateForFunctionPointer<SceneFindEntityDelegate>(funcs.Scene_FindEntityByName);

            Transform_GetTranslation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetTranslation);
            Transform_SetTranslation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetTranslation);
            Transform_GetRotation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetRotation);
            Transform_SetRotation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetRotation);
            Transform_GetScale = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetScale);
            Transform_SetScale = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetScale);

            Input_IsKeyDown = Marshal.GetDelegateForFunctionPointer<InputGetKeyDelegate>(funcs.Input_IsKeyDown);
            Input_IsMouseButtonDown = Marshal.GetDelegateForFunctionPointer<InputGetMouseBtnDelegate>(funcs.Input_IsMouseButtonDown);
            Input_GetMousePosition = Marshal.GetDelegateForFunctionPointer<InputGetMousePosDelegate>(funcs.Input_GetMousePosition);

            Rigidbody2D_ApplyLinearImpulse = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseDelegate>(funcs.Rigidbody2D_ApplyLinearImpulse);
            Rigidbody2D_ApplyLinearImpulseToCenter = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseCenterDelegate>(funcs.Rigidbody2D_ApplyLinearImpulseToCenter);
            Rigidbody2D_GetLinearVelocity = Marshal.GetDelegateForFunctionPointer<Rigidbody2DGetVelocityDelegate>(funcs.Rigidbody2D_GetLinearVelocity);
            Rigidbody2D_SetLinearVelocity = Marshal.GetDelegateForFunctionPointer<Rigidbody2DSetVelocityDelegate>(funcs.Rigidbody2D_SetLinearVelocity);

            Console.WriteLine("[C#] InternalCalls initialized.");
        }
    }
}