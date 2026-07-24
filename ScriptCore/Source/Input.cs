namespace HimiiEngine
{
    public static class Input
    {
        /// <summary>键按住（含重复触发）。</summary>
        public static bool IsKeyDown(KeyCode keycode)
        {
            return InternalCalls.Input_IsKeyDown != null && InternalCalls.Input_IsKeyDown(keycode) != 0;
        }

        /// <summary>本帧刚按下（边沿，不含重复）。</summary>
        public static bool IsKeyPressed(KeyCode keycode)
        {
            return InternalCalls.Input_IsKeyPressed != null && InternalCalls.Input_IsKeyPressed(keycode) != 0;
        }

        /// <summary>本帧刚释放。</summary>
        public static bool IsKeyReleased(KeyCode keycode)
        {
            return InternalCalls.Input_IsKeyReleased != null && InternalCalls.Input_IsKeyReleased(keycode) != 0;
        }

        public static bool IsMouseButtonDown(int button)
        {
            return InternalCalls.Input_IsMouseButtonDown != null && InternalCalls.Input_IsMouseButtonDown(button) != 0;
        }

        public static Vector2 MousePosition
        {
            get
            {
                InternalCalls.Input_GetMousePosition(out Vector2 result);
                return result;
            }
        }

        /// <summary>水平轴：A/Left = -1，D/Right = +1。</summary>
        public static float GetAxisHorizontal()
        {
            return InternalCalls.Input_GetAxisHorizontal?.Invoke() ?? 0.0f;
        }

        /// <summary>垂直轴：S/Down = -1，W/Up = +1。</summary>
        public static float GetAxisVertical()
        {
            return InternalCalls.Input_GetAxisVertical?.Invoke() ?? 0.0f;
        }
    }
}
