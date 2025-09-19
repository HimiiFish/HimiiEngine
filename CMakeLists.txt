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
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 确保 Visual Studio 的输出目录存在，以避免链接器错误
if(MSVC)
    # 让 MSVC 以 UTF-8 读取/编译源文件，消除 C4828 警告
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>" "$<$<C_COMPILER_ID:MSVC>:/utf-8>")

    # 可保留或移除这个目录创建；目标级路径会按需创建
    file(MAKE_DIRECTORY
        ${CMAKE_BINARY_DIR}/bin
        ${CMAKE_BINARY_DIR}/lib
    )
endif()

# 统一每个目标的输出目录：多配置与单配置生成器均兼容
function(himii_set_output_dirs target_name)
    # 单配置生成器或兜底
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${target_name}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${target_name}"
    )

    # 多配置生成器下按配置分别指定
    foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${cfg} CFG_UPPER)
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} "${CMAKE_BINARY_DIR}/bin/${target_name}/${cfg}"
            LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} "${CMAKE_BINARY_DIR}/bin/${target_name}/${cfg}"
            ARCHIVE_OUTPUT_DIRECTORY_${CFG_UPPER} "${CMAKE_BINARY_DIR}/lib/${target_name}/${cfg}"
        )
    endforeach()
endfunction()

# 添加子项目
add_subdirectory(Engine)
add_subdirectory(HimiiEditor)
add_subdirectory(TemplateProject)
