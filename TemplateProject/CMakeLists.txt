project(TemplateProject)

add_executable(TemplateProject TemplateProject.cpp)

target_link_libraries(TemplateProject PRIVATE Engine)

target_include_directories(TemplateProject PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)

set_target_properties(TemplateProject PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ×Ô¶¯¿½±´ DLL
add_custom_command(TARGET TemplateProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Engine>
        $<TARGET_FILE_DIR:TemplateProject>
)
