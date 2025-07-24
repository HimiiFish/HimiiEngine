project(TemplateProject)

add_executable(TemplateProject TemplateProject.cpp "ExampleLayer.cpp" "ExampleLayer.h")

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
set(FONT_ASSETS
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/msyh.ttc
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/msyhbd.ttc
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/msyhl.ttc
)

add_custom_command(TARGET TemplateProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FONT_ASSETS}
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMENT "Copying font assets to output directory"
    COMMENT "将字体资源复制到输出目录"
)