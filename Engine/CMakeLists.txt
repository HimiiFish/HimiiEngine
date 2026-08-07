cmake_minimum_required (VERSION 3.12)
project(Engine)

# 打开MSVC的编辑和继续支持
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 查找依赖
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
# SPIRV-Cross
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)

find_package(box2d REQUIRED)
find_package(unofficial-nethost CONFIG REQUIRED)
find_package(Freetype REQUIRED)

# 设置OpenGL的首选项
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

# 源文件为src目录下的所有.cpp文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 头文件为src目录下的所有.h文件
file(GLOB_RECURSE HEADERS "src/*.h")

# ufbx is distributed as a C translation unit (alongside cgltf-style Mesh headers).
enable_language(C)
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Module/Render/Mesh/ufbx.c")
list(APPEND HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/Module/Render/Mesh/ufbx.h")

# GLOB_RECURSE 会同时扫到各平台实现；按目标 OS 剔除对方源文件，避免交叉编译。
if (WIN32)
    list(REMOVE_ITEM SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Linux/LinuxPlatformUtils.cpp")
elseif (UNIX AND NOT APPLE)
    list(REMOVE_ITEM SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/WindowsPlatformUtils.cpp")
    file(GLOB_RECURSE DIRECTX_SOURCES "src/Platform/DirectX/*.cpp")
    if (DIRECTX_SOURCES)
        list(REMOVE_ITEM SOURCES ${DIRECTX_SOURCES})
    endif()
    # Linux 仍复用 Windows 目录下的 GLFW 窗口/输入实现（跨平台 GLFW 封装）
    list(APPEND SOURCES "src/Platform/Windows/WindowsInput.cpp")
    list(APPEND SOURCES "src/Platform/Windows/WindowsWindow.cpp")
    list(APPEND HEADERS "src/Platform/Windows/WindowsWindow.h")
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/imgui")
set(IMGUIZMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/ImGuizmo")
set(MSDF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/msdf-atlas-gen")
set(MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/miniaudio")

# 检查 ImGui 目录是否存在
if (NOT EXISTS "${IMGUI_DIR}")
    message(FATAL_ERROR "ImGui directory not found at ${IMGUI_DIR}. Please check your submodule path or clone.")
endif()
if (NOT EXISTS "${IMGUIZMO_DIR}")
    message(FATAL_ERROR "ImGuizmo directory not found at ${IMGUIZMO_DIR}")
endif()
if (NOT EXISTS "${MSDF_DIR}")
    message(FATAL_ERROR "msdf-atlas-gen directory not found at ${MSDF_DIR}")
endif()


# ImGui 核心源文件
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

add_subdirectory("${CMAKE_SOURCE_DIR}/Engine/vender/nativefiledialog-extended" nfd_build)

set(IMGUIZMO_SOURCES
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
)

set(IMGUI_BACKENDS
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(ENGINE_ALL_SOURCES
    ${SOURCES}
    ${HEADERS}
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
    ${IMGUIZMO_SOURCES}
)

set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "" FORCE)
add_subdirectory("${MSDF_DIR}" msdf_build)

# 添加源文件和头文件到Engine库
add_library(Engine STATIC ${ENGINE_ALL_SOURCES})

# 在Windows上，如果使用共享库，启用导出所有符号
if(WIN32 AND BUILD_SHARED_LIBS)
    set_property(TARGET Engine PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${IMGUI_DIR} ${IMGUI_DIR}/backends ${IMGUIZMO_DIR} ${MSDF_DIR} ${MSDF_DIR}/msdfgen ${MSDF_DIR}/msdf-atlas-gen ${CMAKE_CURRENT_BINARY_DIR}/msdf_build/msdfgen/include ${MINIAUDIO_DIR})

target_link_libraries(Engine PUBLIC OpenGL::GL glad::glad spdlog::spdlog glfw yaml-cpp::yaml-cpp Vulkan::Vulkan unofficial::shaderc::shaderc spirv-cross-core spirv-cross-glsl SPIRV-Tools-static box2d::box2d unofficial::nethost::nethost nfd Freetype::Freetype msdf-atlas-gen msdfgen-ext msdfgen-core)

target_compile_definitions(Engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_compile_definitions(Engine PUBLIC 
    # 根据构建配置自动定义宏
    $<$<CONFIG:Debug>:HIMII_DEBUG>
    $<$<CONFIG:Release>:HIMII_RELEASE>
)

if (WIN32)
    target_compile_definitions(Engine PUBLIC
        HIMII_PLATFORM_WINDOWS
    )
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(Engine PUBLIC
        HIMII_PLATFORM_LINUX
    )
endif()

if(MSVC)
    target_compile_options(Engine PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
endif()

himii_set_output_dirs(Engine)
