@echo off
setlocal

REM Check if a build type (Debug/Release) is provided
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" (
    echo Usage: build.bat [Debug^|Release]
    echo Example: build.bat Debug
    goto :eof
)

REM Convert build type to lowercase for comparison
set BUILD_TYPE_LOWER=%BUILD_TYPE%
if /i "%BUILD_TYPE_LOWER%"=="debug" (
    set CONFIGURE_PRESET=x64-debug
    set BUILD_PRESET=build-x64-debug-win
) else if /i "%BUILD_TYPE_LOWER%"=="release" (
    set CONFIGURE_PRESET=x64-release
    set BUILD_PRESET=build-x64-release-win
) else (
    echo Invalid build type: %BUILD_TYPE%. Please use Debug or Release.
    goto :eof
)

REM Get the directory of this script
set SCRIPT_DIR=%~dp0

REM Configure the project
echo Configuring project with preset: %CONFIGURE_PRESET%
cmake --preset %CONFIGURE_PRESET% -S "%SCRIPT_DIR%."
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    goto :eof
)

REM Build the project
echo Building project with preset: %BUILD_PRESET%
cmake --build --preset %BUILD_PRESET%
if %errorlevel% neq 0 (
    echo CMake build failed.
    goto :eof
)

echo Build completed successfully for %BUILD_TYPE% configuration.

endlocal
:eof
