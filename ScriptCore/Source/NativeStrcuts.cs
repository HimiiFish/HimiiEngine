using System;
using System.Runtime.InteropServices;

namespace Himii
{
	[StructLayout(LayoutKind.Sequential)]
	internal struct NativeFunctionsMap
	{
		public IntPtr LogFunc;
		public IntPtr Entity_HasComponent;

		// Transform
		public IntPtr Transform_GetTranslation;
		public IntPtr Transform_SetTranslation;

		// Input
		public IntPtr Input_IsKeyDown;

		// Rigidbody2D
		public IntPtr Rigidbody2D_ApplyLinearImpulse;
		public IntPtr Rigidbody2D_ApplyLinearImpulseToCenter;
	}
}