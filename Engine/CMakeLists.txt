# CMakeList.txt: Engine 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Engine")

find_package(SDL3 REQUIRED CONFIG)


#添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")
#添加头文件
file(GLOB_RECURSE HEADERS "include/*.h")
#设置include目录
include_directories("include")

# 将源代码添加到此项目的可执行文件。
add_executable (Engine ${SOURCES} ${HEADERS})

target_include_directories(Engine PUBLIC include)

target_link_libraries(Engine PRIVATE SDL3::SDL3)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Engine PROPERTY CXX_STANDARD 17)
  set_property(TARGET Engine PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

# TODO: 如有需要，请添加测试并安装目标。
