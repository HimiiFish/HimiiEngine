project(TemplateProject)

add_executable(TemplateProject TemplateProject.cpp "ExampleLayer.cpp" "ExampleLayer.h")

# Link against the Engine library
target_link_libraries(TemplateProject PRIVATE Engine)

# Include Engine headers
target_include_directories(TemplateProject PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)