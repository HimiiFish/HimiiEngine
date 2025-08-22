project(TemplateProject)

file(GLOB_RECURSE SOURCES "./*.cpp")

# ͷ�ļ�ΪsrcĿ¼�µ�����.h�ļ�
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

# 强制 MSVC 以 UTF-8 编译此目标，进一步消除 C4828
if(MSVC)
    target_compile_options(TemplateProject PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
endif()
# Copy font assets to the executable output directory so the runtime can
# load them without referencing the repository structure.
add_custom_command(TARGET TemplateProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:TemplateProject>/assets
    COMMENT "Copying all assets to output directory"
)