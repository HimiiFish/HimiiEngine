cmake_minimum_required (VERSION 3.12)
project(Engine)

# 打开MSVC的编辑和继续支持
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 查找依赖
find_package(SDL3 REQUIRED CONFIG)
if(NOT SDL3_FOUND)
    message(FATAL_ERROR "SDL3 not found. Please ensure it is installed via vcpkg.")
endif()
find_package(glad CONFIG REQUIRED)
if(NOT glad_FOUND)
    message(FATAL_ERROR "glad not found. Please ensure it is installed via vcpkg.")
endif ()
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL not found. Please ensure it is installed via vcpkg.")
endif ()
# 设置OpenGL的首选项
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Engine")

# 递归查找 Engine 下所有 .cpp 和 .h 文件
file(GLOB_RECURSE ALL_SOURCE_FILES "${SOURCE_DIR}/src/Himii/**/*.cpp" "${SOURCE_DIR}/src/Platform/*.cpp")
file(GLOB_RECURSE ALL_HEADER_FILES "${SOURCE_DIR}/src/Himii/**/*.h" "${SOURCE_DIR}/src/Include/*.h" "${SOURCE_DIR}/Platform/*.h")

# 添加 Engine 库
add_library(Engine STATIC ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES})

# 设置包含目录
target_include_directories(Engine PUBLIC
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Engine/Himii/Core
    ${CMAKE_SOURCE_DIR}/Engine/Himii/Events
    ${CMAKE_SOURCE_DIR}/Engine/Platform
)
# 在Windows上，如果使用共享库，启用导出所有符号
if(WIN32 AND BUILD_SHARED_LIBS)
    set_property(TARGET Engine PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(Engine PUBLIC SDL3::SDL3 OpenGL::GL glad::glad)
