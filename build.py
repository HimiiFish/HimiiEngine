#!/usr/bin/env python3
"""
跨平台CMake构建脚本
支持Windows和Linux平台的Debug/Release构建
"""

import os
import sys
import platform
import subprocess
import argparse
import shutil
from pathlib import Path


class BuildScript:
    def __init__(self):
        self.script_dir = Path(__file__).parent.absolute()
        self.system = platform.system().lower()
        self.is_windows = self.system == "windows"
        self.is_linux = self.system == "linux"
        
        # 配置平台特定的预设
        if self.is_windows:
            self.configure_presets = {
                "debug": "x64-debug",
                "release": "x64-release"
            }
            self.build_presets = {
                "debug": "build-x64-debug-win",
                "release": "build-x64-release-win"
            }
            self.executable_names = ["Engine.exe", "HimiiEngineProject.exe"]
        elif self.is_linux:
            self.configure_presets = {
                "debug": "linux-debug",
                "release": "linux-release"
            }
            self.build_presets = {
                "debug": "build-linux-debug",
                "release": "build-linux-release"
            }
            self.executable_names = ["Engine", "HimiiEngineProject"]
        else:
            print(f"错误: 不支持的操作系统: {self.system}")
            sys.exit(1)
    
    def check_dependencies(self):
        """检查必要的依赖"""
        print("正在检查构建依赖...")
        
        # 检查CMake
        try:
            result = subprocess.run(["cmake", "--version"], 
                                  capture_output=True, text=True, check=True)
            cmake_version = result.stdout.split('\n')[0]
            print(f"✓ 找到CMake: {cmake_version}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("✗ 错误: 未找到CMake，请先安装CMake")
            return False
        
        # 检查vcpkg目录
        vcpkg_dir = self.script_dir / "vcpkg"
        if not vcpkg_dir.exists():
            print("✗ 错误: 未找到vcpkg目录")
            return False
        print("✓ 找到vcpkg目录")
        
        # 在Linux上检查Ninja
        if self.is_linux:
            try:
                subprocess.run(["ninja", "--version"], 
                             capture_output=True, check=True)
                print("✓ 找到Ninja构建系统")
            except (subprocess.CalledProcessError, FileNotFoundError):
                print("✗ 警告: 未找到Ninja，可能会影响Linux构建")
        
        return True
    
    def clean_build(self, build_type):
        """清理构建目录"""
        configure_preset = self.configure_presets[build_type.lower()]
        build_dir = self.script_dir / "build" / configure_preset
        
        if build_dir.exists():
            print(f"正在清理构建目录: {build_dir}")
            shutil.rmtree(build_dir)
            print("✓ 构建目录已清理")
        else:
            print("构建目录不存在，无需清理")
    
    def configure_project(self, build_type):
        """配置项目"""
        configure_preset = self.configure_presets[build_type.lower()]
        
        print(f"正在配置项目 (预设: {configure_preset})...")
        
        cmd = ["cmake", "--preset", configure_preset, "-S", str(self.script_dir)]
        
        try:
            result = subprocess.run(cmd, cwd=self.script_dir, check=True)
            print("✓ 项目配置成功")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ 项目配置失败，退出代码: {e.returncode}")
            return False
    
    def build_project(self, build_type):
        """构建项目"""
        build_preset = self.build_presets[build_type.lower()]
        
        print(f"正在构建项目 (预设: {build_preset})...")
        
        cmd = ["cmake", "--build", "--preset", build_preset]
        
        try:
            result = subprocess.run(cmd, cwd=self.script_dir, check=True)
            print(f"✓ 项目构建成功 ({build_type} 配置)")
            return True
        except subprocess.CalledProcessError as e:
            print(f"✗ 项目构建失败，退出代码: {e.returncode}")
            return False
    
    def find_executable(self, build_type):
        """查找生成的可执行文件"""
        configure_preset = self.configure_presets[build_type.lower()]
        
        if self.is_windows:
            # Windows下根据构建类型选择子目录 (Debug/Release)
            build_subdir = "Debug" if build_type.lower() == "debug" else "Release"
            executable_base_path = self.script_dir / "build" / configure_preset / "Engine" / build_subdir
        else:
            # Linux下直接在Engine目录
            executable_base_path = self.script_dir / "build" / configure_preset / "Engine"
        
        for exe_name in self.executable_names:
            exe_path = executable_base_path / exe_name
            if exe_path.exists():
                return exe_path
        
        return None
    
    def run_executable(self, build_type, auto_run=False):
        """运行生成的可执行文件"""
        exe_path = self.find_executable(build_type)
        
        if exe_path:
            if auto_run:
                choice = 'y'
            else:
                choice = input(f"是否运行程序 \"{exe_path}\"? (y/n): ").lower()
            
            if choice == 'y':
                print(f"正在启动 \"{exe_path}\"...")
                try:
                    if self.is_windows:
                        subprocess.Popen([str(exe_path)], cwd=exe_path.parent)
                    else:
                        subprocess.Popen([str(exe_path)], cwd=exe_path.parent)
                    print("✓ 程序已启动")
                except Exception as e:
                    print(f"✗ 启动程序失败: {e}")
            else:
                print("程序未启动")
        else:
            configure_preset = self.configure_presets[build_type.lower()]
            if self.is_windows:
                build_subdir = "Debug" if build_type.lower() == "debug" else "Release"
                executable_base_path = self.script_dir / "build" / configure_preset / "Engine" / build_subdir
            else:
                executable_base_path = self.script_dir / "build" / configure_preset / "Engine"
            
            print(f"✗ 在 \"{executable_base_path}\" 中未找到可运行的程序")
            print(f"   查找的文件: {', '.join(self.executable_names)}")
    
    def build(self, build_type, clean=False, auto_run=False):
        """完整的构建流程"""
        build_type = build_type.lower()
        
        if build_type not in ["debug", "release"]:
            print("错误: 构建类型必须是 'debug' 或 'release'")
            return False
        
        print(f"开始构建 HimiiEngine ({build_type.upper()} 配置)")
        print(f"平台: {platform.system()} {platform.machine()}")
        print("-" * 50)
        
        # 检查依赖
        if not self.check_dependencies():
            return False
        
        # 清理构建目录（如果需要）
        if clean:
            self.clean_build(build_type)
        
        # 配置项目
        if not self.configure_project(build_type):
            return False
        
        # 构建项目
        if not self.build_project(build_type):
            return False
        
        print("-" * 50)
        print(f"✓ 构建完成 ({build_type.upper()} 配置)")
        
        # 运行可执行文件
        self.run_executable(build_type, auto_run)
        
        return True


def main():
    parser = argparse.ArgumentParser(description="HimiiEngine跨平台构建脚本")
    parser.add_argument("build_type", 
                       choices=["debug", "release", "Debug", "Release"],
                       help="构建类型 (Debug/Release)")
    parser.add_argument("--clean", "-c", 
                       action="store_true",
                       help="构建前清理构建目录")
    parser.add_argument("--run", "-r",
                       action="store_true", 
                       help="构建完成后自动运行程序")
    parser.add_argument("--clean-only",
                       action="store_true",
                       help="仅清理构建目录，不进行构建")
    
    args = parser.parse_args()
    
    builder = BuildScript()
    
    if args.clean_only:
        builder.clean_build(args.build_type)
        return
    
    success = builder.build(args.build_type, args.clean, args.run)
    
    if not success:
        sys.exit(1)


if __name__ == "__main__":
    main()
