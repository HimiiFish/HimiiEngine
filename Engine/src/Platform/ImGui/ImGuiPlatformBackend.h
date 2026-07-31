#pragma once

struct ImDrawData;

namespace Himii
{
    /// ImGui 平台/图形后端（GLFW + OpenGL 实现位于 Platform/**）。
    class ImGuiPlatformBackend
    {
    public:
        static void Initialize(void *nativeWindow);
        static void Shutdown();
        static void NewFrame();
        static void RenderDrawData(ImDrawData *drawData);
        /// 多视口：备份/恢复当前 GL 上下文并绘制平台窗口。
        static void RenderPlatformWindows();
        static void RecreateDeviceObjects();
    };
}
