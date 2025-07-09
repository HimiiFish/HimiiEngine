cmake_minimum_required(VERSION 3.12)

project(HimiiEngineProject LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
# 设置C++标准为必需
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置库的默认构建类型
# 如果未指定构建类型，则默认为构建动态库
# ON -> 构建动态库
# OFF -> 构建静态库
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# 为所有目标设置输出目录
# 单配置生成器（例如 Makefiles）使用这些通用路径
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 多配置生成器（例如 Visual Studio）使用这些特定于配置的路径
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} config_upper)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# 确保 Visual Studio 的输出目录存在，以避免链接器错误
if(MSVC)
    file(MAKE_DIRECTORY
        ${CMAKE_BINARY_DIR}/bin
        ${CMAKE_BINARY_DIR}/lib
    )
endif()

# 添加子项目
add_subdirectory(Engine)
add_subdirectory(TemplateProject)
