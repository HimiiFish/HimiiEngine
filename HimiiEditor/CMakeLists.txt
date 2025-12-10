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

if(MSVC)
    target_compile_options(HimiiEditor PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
endif()

himii_set_output_dirs(HimiiEditor)

set(SCRIPT_CORE_BUILD_DIR "${CMAKE_SOURCE_DIR}/ScriptCore/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/net8.0")

# Copy font assets to the executable output directory so the runtime can
# load them without referencing the repository structure.
add_custom_command(TARGET HimiiEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:HimiiEditor>/assets
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:HimiiEditor>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:HimiiEditor>/assets
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:HimiiEditor>/resources
    COMMENT "Copying all assets to output directory"

     # 2. 拷贝 C# DLLs (新增逻辑)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.dll"
            "$<TARGET_FILE_DIR:HimiiEditor>"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.pdb"
            "$<TARGET_FILE_DIR:HimiiEditor>"

    # 3. 极其重要：runtimeconfig.json 必须拷过去，否则 nethost 无法启动 .NET 运行时
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SCRIPT_CORE_BUILD_DIR}/ScriptCore.runtimeconfig.json"
            "$<TARGET_FILE_DIR:HimiiEditor>"

    COMMENT "Copying Assets and ScriptCore DLLs to output directory"
)
