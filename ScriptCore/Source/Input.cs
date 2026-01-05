namespace Himii
{
    public static class Input
    {
        public static bool IsKeyDown(KeyCode keycode)
        {
            return InternalCalls.Input_IsKeyDown(keycode);
        }

        public static bool IsMouseButtonDown(int button)
        {
            return InternalCalls.Input_IsMouseButtonDown(button);
        }

        public static Vector2 MousePosition
        {
            get
            {
                InternalCalls.Input_GetMousePosition(out Vector2 result);
                return result;
            }
        }
    }
}