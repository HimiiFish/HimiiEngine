cmake_minimum_required (VERSION 3.12)
project(Engine)

# 打开MSVC的编辑和继续支持
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 查找依赖
find_package(glad CONFIG REQUIRED)
if(NOT glad_FOUND)
    message(FATAL_ERROR "glad not found. Please ensure it is installed via vcpkg.")
endif ()
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL not found. Please ensure it is installed via vcpkg.")
endif ()
find_package(spdlog CONFIG REQUIRED)
if(NOT spdlog_FOUND)
    message(FATAL_ERROR "spdlog not found. Please ensure it is installed via vcpkg.")
endif ()
find_package(glfw3 CONFIG REQUIRED)
if(NOT glfw3_FOUND)
    message(FATAL_ERROR "glfw3 not found. Please ensure it is installed via vcpkg.")
endif ()
find_package(yaml-cpp CONFIG REQUIRED)
if(NOT yaml-cpp_FOUND)
    message(FATAL_ERROR "yaml-cpp not found. Please ensure it is installed via vcpkg.")
endif ()

# 设置OpenGL的首选项
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

# 源文件为src目录下的所有.cpp文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 头文件为src目录下的所有.h文件
file(GLOB_RECURSE HEADERS "src/*.h")


set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/imgui")

# 检查 ImGui 目录是否存在
if (NOT EXISTS "${IMGUI_DIR}")
    message(FATAL_ERROR "ImGui directory not found at ${IMGUI_DIR}. Please check your submodule path or clone.")
endif()

# ImGui 核心源文件
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp # ImGui 1.80+ 包含 tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp   # 如果需要 ImGui 示例窗口，可以包含
)

set(IMGUI_BACKENDS
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
set(ENGINE_ALL_SOURCES
    ${SOURCES}
    ${HEADERS}
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
)

# 添加源文件和头文件到Engine库
# 不需指定SHARED或STATIC，因为CMake会根据BUILD_SHARED_LIBS变量自动处理
add_library(Engine STATIC ${ENGINE_ALL_SOURCES})

# 在Windows上，如果使用共享库，启用导出所有符号
if(WIN32 AND BUILD_SHARED_LIBS)
    set_property(TARGET Engine PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${IMGUI_DIR} ${IMGUI_DIR}/backends)

target_link_libraries(Engine PUBLIC OpenGL::GL glad::glad spdlog::spdlog glfw yaml-cpp)

# 启用 GLM 实验特性（如 gtx/component_wise），避免各 TU 单独定义宏
target_compile_definitions(Engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

# 强制 MSVC 以 UTF-8 编译此目标，进一步消除 C4828
if(MSVC)
    target_compile_options(Engine PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
endif()
