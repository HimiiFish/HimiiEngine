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

# MSVC 特定设置
if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>" "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
endif()

# 添加子项目
add_subdirectory(ScriptCore)
add_subdirectory(Engine)
add_subdirectory(HimiiEditor)
add_subdirectory(HimiiRuntime)
add_dependencies(HimiiEditor ScriptCore_Build)
