using System;
using System.Runtime.InteropServices;

namespace Himii
{
	[StructLayout(LayoutKind.Sequential)]
	internal struct NativeFunctionsMap
	{
		public IntPtr LogFunc;

		// Scene/Entity
		public IntPtr Entity_HasComponent;
		public IntPtr Scene_CreateEntity;
        public IntPtr Scene_DestroyEntity;
        public IntPtr Scene_FindEntityByName;

        // Transform
        public IntPtr Transform_GetTranslation;
		public IntPtr Transform_SetTranslation;
		public IntPtr Transform_GetRotation;
        public IntPtr Transform_SetRotation;
        public IntPtr Transform_GetScale;
        public IntPtr Transform_SetScale;

        // Input
        public IntPtr Input_IsKeyDown;
        public IntPtr Input_IsMouseButtonDown;
        public IntPtr Input_GetMousePosition;

        // Rigidbody2D
        public IntPtr Rigidbody2D_ApplyLinearImpulse;
		public IntPtr Rigidbody2D_ApplyLinearImpulseToCenter;
        public IntPtr Rigidbody2D_GetLinearVelocity;
        public IntPtr Rigidbody2D_SetLinearVelocity;
    }
}