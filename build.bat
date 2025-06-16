@echo off
REM build.bat - 一键构建脚本 (Windows)

REM --- 设置 Visual Studio 开发人员命令提示符环境 ---
echo 正在尝试设置 Visual Studio 开发人员命令提示符环境...

REM 检查 cl.exe 是否已在 PATH 中 (一个简单的检查，判断环境是否已配置)
where cl.exe >nul 2>nul
if %errorlevel% == 0 (
    echo Visual Studio 环境似乎已设置 (cl.exe found).
) else (
    echo cl.exe 未在 PATH 中找到。正在尝试调用 VsDevCmd.bat...
    
    REM ** 用户注意: 您可能需要根据您的 Visual Studio 安装路径和版本调整以下路径 **
    REM 示例路径 (Visual Studio 2022 Community):
    set "VS_DEV_CMD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    REM 示例路径 (Visual Studio 2019 Community):
    REM set "VS_DEV_CMD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
    REM 示例路径 (Visual Studio 2019/2022 Build Tools):
    REM set "VS_DEV_CMD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

    if exist "%VS_DEV_CMD_PATH%" (
        echo 正在调用: "%VS_DEV_CMD_PATH%"
        call "%VS_DEV_CMD_PATH%" -arch=amd64 -host_arch=amd64
        REM 再次检查 cl.exe
        where cl.exe >nul 2>nul
        if %errorlevel% == 0 (
            echo Visual Studio 环境已成功设置。
        ) else (
            echo 调用 VsDevCmd.bat 后仍未找到 cl.exe。
            echo 请确保 "%VS_DEV_CMD_PATH%" 的路径正确，并且 Visual Studio 已正确安装。
            echo 或者，请从“适用于 VS 的 x64 本机工具命令提示符”(或类似名称)运行此脚本。
            goto :eof
        )
    ) else (
        echo 未找到 VsDevCmd.bat 于 "%VS_DEV_CMD_PATH%"。
        echo 请执行以下操作之一:
        echo 1. 在此脚本中更新 VS_DEV_CMD_PATH 为您正确的 VsDevCmd.bat 路径。
        echo 2. 从“适用于 VS 的 x64 本机工具命令提示符”(或类似名称的开发人员命令提示符)手动运行此 build.bat 脚本。
        goto :eof
    )
)
echo ----------------------------------------------------
REM --- 环境设置结束 ---

REM 设置默认构建类型
set BUILD_TYPE=debug
set ARCHITECTURE=x64

REM 检查是否有参数传递，并据此设置构建类型
if /I "%1" == "release" (
    set BUILD_TYPE=release
    echo Building Release version...
) else if /I "%1" == "debug" (
    echo Building Debug version...
) else if NOT "%1" == "" (
    echo 未知的构建类型: %1. 将使用默认的 debug 构建.
) else (
    echo 未指定构建类型，将使用默认的 debug 构建.
)

REM 根据构建类型选择 CMake 预设
REM 假设您的 Windows 配置预设命名为 "x64-debug", "x64-release"
REM 假设您的 Windows 构建预设命名为 "build-x64-debug-win", "build-x64-release-win"
set CONFIGURE_PRESET=%ARCHITECTURE%-%BUILD_TYPE%
set BUILD_PRESET=build-%ARCHITECTURE%-%BUILD_TYPE%-win

REM 获取脚本所在的目录 (项目根目录)
set PROJECT_ROOT_DIR=%~dp0
REM 移除末尾的反斜杠 (如果存在)
if "%PROJECT_ROOT_DIR:~-1%"=="\" set "PROJECT_ROOT_DIR=%PROJECT_ROOT_DIR:~0,-1%"

REM 构建目录 (与 CMakePresets.json 中的 binaryDir 对应)
set BUILD_DIR=%PROJECT_ROOT_DIR%\build\%CONFIGURE_PRESET%

REM 确保构建目录存在
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

echo ----------------------------------------------------
echo 配置项目 (Preset: %CONFIGURE_PRESET%)
echo ----------------------------------------------------
cmake --preset "%CONFIGURE_PRESET%" -S "%PROJECT_ROOT_DIR%" -B "%BUILD_DIR%"

REM 检查 CMake 配置是否成功
if errorlevel 1 (
    echo CMake 配置失败！
    exit /b 1
)

echo.
echo ----------------------------------------------------
echo 构建项目 (Preset: %BUILD_PRESET%)
echo ----------------------------------------------------
cmake --build "%BUILD_DIR%" --preset "%BUILD_PRESET%"

REM 检查 CMake 构建是否成功
if errorlevel 1 (
    echo CMake 构建失败！
    exit /b 1
)

echo.
echo ----------------------------------------------------
echo 构建完成！
echo 可执行文件可能位于: %BUILD_DIR%\Engine\%BUILD_TYPE%\Engine.exe
echo (或者 %BUILD_DIR%\Engine\Engine.exe，取决于您的 CMake 输出配置)
echo ----------------------------------------------------

exit /b 0