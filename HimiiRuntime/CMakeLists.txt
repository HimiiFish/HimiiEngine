project(HimiiRuntime)

add_executable(HimiiRuntime src/RuntimeApp.cpp)

target_compile_features(HimiiRuntime PUBLIC cxx_std_17)

target_include_directories(HimiiRuntime PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/src
)

target_link_libraries(HimiiRuntime PRIVATE Engine)

if(MSVC)
    target_link_options(HimiiRuntime PRIVATE
        "$<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>"
        "$<$<NOT:$<CONFIG:Debug>>:/ENTRY:mainCRTStartup>"
    )
endif()

himii_set_output_dirs(HimiiRuntime)

add_dependencies(HimiiRuntime PackEngineResources)

add_custom_command(TARGET HimiiRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DTARGET_DIR=$<TARGET_FILE_DIR:HimiiRuntime>
        -DHIMII_EDITOR_DIR=${CMAKE_SOURCE_DIR}/HimiiEditor
        -DENGINE_HPCK=${HIMII_ENGINE_HPCK}
        -DSCRIPT_CORE_DIR=${CMAKE_SOURCE_DIR}/ScriptCore/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/net8.0
        -DCONFIG=$<CONFIG>
        -P ${CMAKE_SOURCE_DIR}/cmake/RuntimePostBuild.cmake
    COMMENT "Post-Build: staging runtime files for Debug or Release layout"
)

# Export Templates layout (T1): Release Runtime shell + native DLLs + HimiiEngine content.
# Not mixed into the editor's HimiiEngine/ install prefix.
install(TARGETS HimiiRuntime
    RUNTIME DESTINATION ExportTemplates/Windows
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
)

install(FILES "${HIMII_ENGINE_HPCK}"
    DESTINATION ExportTemplates/Windows/HimiiEngine
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
    RENAME engine.hpck
)

install(FILES
    "${CMAKE_SOURCE_DIR}/ScriptCore/bin/Release/net8.0/ScriptCore.dll"
    "${CMAKE_SOURCE_DIR}/ScriptCore/bin/Release/net8.0/ScriptCore.runtimeconfig.json"
    DESTINATION ExportTemplates/Windows/HimiiEngine
    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
)

foreach(install_configuration Release RelWithDebInfo MinSizeRel)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/bin/HimiiRuntime/${install_configuration}/"
        DESTINATION ExportTemplates/Windows
        CONFIGURATIONS ${install_configuration}
        OPTIONAL
        FILES_MATCHING
            PATTERN "*.dll"
    )
endforeach()
