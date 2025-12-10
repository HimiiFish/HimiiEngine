# 1. 查找系统中的 dotnet 命令
find_program(DOTNET_EXE dotnet REQUIRED)

# 2. 定义 C# 项目的路径
set(CSHARP_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CSHARP_PROJECT_FILE "${CSHARP_PROJECT_DIR}/ScriptCore.csproj")

# 3. 创建构建目标
add_custom_target(ScriptCore_Build ALL
    # 执行 dotnet build
    COMMAND ${DOTNET_EXE} build "${CSHARP_PROJECT_FILE}" -c $<IF:$<CONFIG:Debug>,Debug,Release>
    COMMENT "Building C# ScriptCore..."
    VERBATIM
)