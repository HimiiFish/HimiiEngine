#include "Engine.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

void RenderTriangle()
{
    std::cout << "Rendering triangle..." << std::endl;

    // 定义三角形的顶点
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); // 红色
    glVertex2f(-0.5f, -0.5f);    // 左下角
    glColor3f(0.0f, 1.0f, 0.0f); // 绿色
    glVertex2f(0.5f, -0.5f);     // 右下角
    glColor3f(0.0f, 0.0f, 1.0f); // 蓝色
    glVertex2f(0.0f, 0.5f);      // 顶部
    glEnd();

    // 刷新缓冲区
    glFlush();
}

int main(int argc, char *argv[])
{
    SDL_Window *window = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("OpenGL Triangle", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    bool quit = false;
    SDL_Event event;

    while (!quit)
    {
        // 清除屏幕
        glClear(GL_COLOR_BUFFER_BIT);
        RenderTriangle();
        SDL_GL_SwapWindow(window);
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
        }
    }

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}