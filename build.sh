#!/bin/bash

# 设置默认构建类型
BUILD_TYPE="debug"

# 检查是否有参数传递，并据此设置构建类型
if [ "$1" == "release" ]; then
  BUILD_TYPE="release"
  echo "Building Release version..."
elif [ "$1" == "debug" ]; then
  echo "Building Debug version..."
elif [ -n "$1" ]; then
  echo "未知的构建类型: $1. 将使用默认的 debug 构建."
else
  echo "未指定构建类型，将使用默认的 debug 构建."
fi

# 根据构建类型选择 CMake 预设
CONFIGURE_PRESET="linux-${BUILD_TYPE}"
BUILD_PRESET="build-linux-${BUILD_TYPE}"

# 获取脚本所在的目录 (项目根目录)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT_DIR="${SCRIPT_DIR}"

# 构建目录 (与 CMakePresets.json 中的 binaryDir 对应)
BUILD_DIR="${PROJECT_ROOT_DIR}/build/${CONFIGURE_PRESET}"

# 确保构建目录存在
mkdir -p "${BUILD_DIR}"

echo "----------------------------------------------------"
echo "配置项目 (Preset: ${CONFIGURE_PRESET})"
echo "----------------------------------------------------"
cmake --preset "${CONFIGURE_PRESET}" -S "${PROJECT_ROOT_DIR}" -B "${BUILD_DIR}"

# 检查 CMake 配置是否成功
if [ $? -ne 0 ]; then
  echo "CMake 配置失败！"
  exit 1
fi

echo ""
echo "----------------------------------------------------"
echo "构建项目 (Preset: ${BUILD_PRESET})"
echo "----------------------------------------------------"
cmake --build "${BUILD_DIR}" --preset "${BUILD_PRESET}"

# 检查 CMake 构建是否成功
if [ $? -ne 0 ]; then
  echo "CMake 构建失败！"
  exit 1
fi

echo ""
echo "----------------------------------------------------"
echo "构建完成！"

exit 0