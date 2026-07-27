project(HimiiEditor)

file(GLOB_RECURSE SOURCES "./src/*.cpp")

file(GLOB_RECURSE HEADERS "./src/*.h")

set(ALL_SOURCES
    ${SOURCES}
    ${HEADERS}
)

if (WIN32)
    set(APP_ICON_RESOURCE "AppIcon.rc")
endif()

add_executable(HimiiEditor ${ALL_SOURCES} ${APP_ICON_RESOURCE})

target_link_libraries(HimiiEditor PRIVATE Engine)

target_include_directories(HimiiEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" PROJECT_ROOT_PATH)
target_compile_definitions(HimiiEditor PRIVATE
    HIMII_ROOT_DIR="${PROJECT_ROOT_PATH}")

if(MSVC)
    target_compile_options(HimiiEditor PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<C_COMPILER_ID:MSVC>:/utf-8>)
    # Release 等配置使用 GUI 子系统，不弹出 Win32 控制台；仍通过 mainCRTStartup 使用 main 入口
    target_link_options(HimiiEditor PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>"
        "$<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>"
    )
endif()

himii_set_output_dirs(HimiiEditor)

set_target_properties(HimiiEditor PROPERTIES
    OUTPUT_NAME_RELEASE "HimiiEngine"
    OUTPUT_NAME_RELWITHDEBINFO "HimiiEngine"
    OUTPUT_NAME_MINSIZEREL "HimiiEngine"
)

add_dependencies(HimiiEditor PackEngineResources HimiiRuntime)

add_custom_command(TARGET HimiiEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DTARGET_DIR=$<TARGET_FILE_DIR:HimiiEditor>
        -DHIMII_EDITOR_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DENGINE_HPCK=${HIMII_ENGINE_HPCK}
        -DSCRIPT_CORE_DIR=${CMAKE_SOURCE_DIR}/ScriptCore/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/net8.0
        -DCONFIG=$<CONFIG>
        -P ${CMAKE_SOURCE_DIR}/cmake/EditorPostBuild.cmake
    COMMAND ${CMAKE_COMMAND}
        -DRUNTIME_TARGET_DIR=$<TARGET_FILE_DIR:HimiiRuntime>
        -DEDITOR_TARGET_DIR=$<TARGET_FILE_DIR:HimiiEditor>
        -DCONFIG=$<CONFIG>
        -P ${CMAKE_SOURCE_DIR}/cmake/SyncExportTemplates.cmake
    COMMENT "Post-Build: staging editor files and Release ExportTemplates"
)

install(TARGETS HimiiEditor RUNTIME DESTINATION . CONFIGURATIONS Release RelWithDebInfo MinSizeRel)
install(FILES "${HIMII_ENGINE_HPCK}" DESTINATION HimiiEngine CONFIGURATIONS Release RelWithDebInfo MinSizeRel RENAME engine.hpck)
install(FILES
    "${CMAKE_SOURCE_DIR}/ScriptCore/bin/Release/net8.0/ScriptCore.dll"
    "${CMAKE_SOURCE_DIR}/ScriptCore/bin/Release/net8.0/ScriptCore.runtimeconfig.json"
    DESTINATION HimiiEngine
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
)
install(FILES
    "${CMAKE_SOURCE_DIR}/HimiiEditor/resources/default/editor_layout.ini"
    DESTINATION .
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
    RENAME editor_layout.ini
)
