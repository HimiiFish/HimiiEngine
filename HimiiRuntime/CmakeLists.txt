project(HimiiRuntime)

# 添加可执行文件
# WIN32 选项会让程序在 Windows 上启动时不显示黑框控制台
# 如果你想看日志调试，可以先去掉 WIN32
add_executable(HimiiRuntime src/RuntimeApp.cpp)

# 设置 C++ 标准
target_compile_features(HimiiRuntime PUBLIC cxx_std_17)

# 包含引擎头文件
target_include_directories(HimiiRuntime PRIVATE 
    ${CMAKE_SOURCE_DIR}/Engine/src
)

# 链接引擎核心库 (假设你的引擎库 Target 叫 Himii-Engine)
target_link_libraries(HimiiRuntime PRIVATE Engine)


# ==============================================================
# [构建后操作] Post-Build Commands
# 这是 Runtime 能否运行的关键！我们需要把所有依赖文件复制到输出目录
# ==============================================================

set(SCRIPT_CORE_BUILD_DIR "${CMAKE_SOURCE_DIR}/ScriptCore/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/net8.0")

add_custom_command(TARGET HimiiRuntime POST_BUILD
    # [删除] 既然是静态库，不需要复制 Himii-Engine.dll/.lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:HimiiRuntime>/assets
    # 1. [Nethost 关键] 复制 .runtimeconfig.json
    # nethost 启动时会在 exe 同级目录下找 "HimiiRuntime.runtimeconfig.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.dll"
            "$<TARGET_FILE_DIR:HimiiRuntime>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.pdb"
            "$<TARGET_FILE_DIR:HimiiRuntime>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.runtimeconfig.json"
            "$<TARGET_FILE_DIR:HimiiRuntime>"
)

# 注意：
# 如果你使用了 vcpkg 且安装了动态库（如 glfw3.dll, openal32.dll 等），
# vcpkg 通常会自动处理 DLL 复制，或者你需要手动在这里添加 copy 命令。