project(HimiiEditor)

file(GLOB_RECURSE SOURCES "./src/*.cpp")

# 头锟侥硷拷为src目录锟铰碉拷锟斤拷锟斤拷.h锟侥硷拷
file(GLOB_RECURSE HEADERS "./src/*.h")

set(ALL_SOURCES
    ${SOURCES}
    ${HEADERS}
)

add_executable(HimiiEditor ${ALL_SOURCES})

# Link against the Engine library
target_link_libraries(HimiiEditor PRIVATE Engine)

# Include Engine headers
target_include_directories(HimiiEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_PATH)
target_compile_definitions(HimiiEditor PRIVATE 
    HIMII_ROOT_DIR="${PROJECT_ROOT_PATH}")


if(MSVC)
    target_compile_options(HimiiEditor PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
endif()

himii_set_output_dirs(HimiiEditor)

set(SCRIPT_CORE_BUILD_DIR "${CMAKE_SOURCE_DIR}/ScriptCore/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/net8.0")

# Copy font assets to the executable output directory so the runtime can
# load them without referencing the repository structure.
add_custom_command(TARGET HimiiEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:HimiiEditor>/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:HimiiEditor>/resources

    # 2. 拷贝 Assets 和 Resources 文件夹
    # 注意：这里使用了 copy_directory，意味着如果你修改了 Source 里的资源，
    # 必须重新编译一次 Editor 才会同步过去。
    # 更好的方式是以后做资源系统时直接读取 Source 目录，或者使用 mklink (Symbolic Link)。
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:HimiiEditor>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:HimiiEditor>/resources

            # 3. 拷贝 ScriptCore 核心库
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.dll"
            "$<TARGET_FILE_DIR:HimiiEditor>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.pdb"
            "$<TARGET_FILE_DIR:HimiiEditor>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.runtimeconfig.json"
            "$<TARGET_FILE_DIR:HimiiEditor>"


    COMMENT "Post-Build: Copying Assets, Scripts, and CSPROJ to output bin directory"
)
