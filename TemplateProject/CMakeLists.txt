project(TemplateProject)

file(GLOB_RECURSE SOURCES "./*.cpp")

# 头文件为src目录下的所有.h文件
file(GLOB_RECURSE HEADERS "./*.h")

set(ALL_SOURCES
    ${SOURCES}
    ${HEADERS}
)

add_executable(TemplateProject ${ALL_SOURCES})

# Link against the Engine library
target_link_libraries(TemplateProject PRIVATE Engine)

# Include Engine headers
target_include_directories(TemplateProject PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)
# Copy font assets to the executable output directory so the runtime can
# 将字体资源复制到可执行文件输出目录，以便运行时可以使用
# load them without referencing the repository structure.
# 不参考仓库结构加载它们。
add_custom_command(TARGET TemplateProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMENT "Copying all assets to output directory"
)