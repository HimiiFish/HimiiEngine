#include "Engine.h" // 假设您有一个 Engine.h，如果不需要可以移除
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // 对于某些平台和 main 函数的正确定义是推荐的
#include <iostream>

// 屏幕尺寸常量
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int argc, char *argv[])
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    // 初始化 SDL 视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 创建窗口
    window = SDL_CreateWindow("SDL3 Window",       // 窗口标题
                              SCREEN_WIDTH,        // 窗口宽度
                              SCREEN_HEIGHT,       // 窗口高度
                              SDL_WINDOW_RESIZABLE // 窗口标志，例如可调整大小
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 创建渲染器
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event event;

    // 主事件循环
    while (!quit)
    {
        // 处理事件队列
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            // 您可以在这里添加其他事件处理，例如键盘、鼠标输入
        }

        // 清除屏幕 (例如，设置为蓝色)
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // R, G, B, A
        SDL_RenderClear(renderer);

        // 在这里进行绘制操作
        // 例如: SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // SDL_RenderLine(renderer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        // 更新屏幕
        SDL_RenderPresent(renderer);
    }

    // 清理资源
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "SDL3 Application exited cleanly." << std::endl;
    return 0;
}
