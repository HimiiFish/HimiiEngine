cmake_minimum_required (VERSION 3.8)
project(Engine)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 查找依赖
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/sdl3")
find_package(SDL3 REQUIRED CONFIG)
find_package(glad CONFIG REQUIRED)
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# 源文件
file(GLOB_RECURSE SOURCES "src/Core/*.cpp")
file(GLOB_RECURSE HEADERS "src/**/*.h")

add_library(Engine SHARED ${SOURCES} ${HEADERS})

target_include_directories(Engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(Engine PRIVATE SDL3::SDL3 OpenGL::GL glad::glad)

set_target_properties(Engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Debug
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

set_property(TARGET Engine PROPERTY CXX_STANDARD 17)
set_property(TARGET Engine PROPERTY CXX_STANDARD_REQUIRED ON)
