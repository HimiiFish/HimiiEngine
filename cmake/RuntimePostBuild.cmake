if(NOT DEFINED TARGET_DIR)
    message(FATAL_ERROR "RuntimePostBuild.cmake: TARGET_DIR is required")
endif()

if(CONFIG STREQUAL "Release" OR CONFIG STREQUAL "RelWithDebInfo" OR CONFIG STREQUAL "MinSizeRel")
    set(HIMII_ENGINE_DIR "${TARGET_DIR}/HimiiEngine")
    file(MAKE_DIRECTORY "${HIMII_ENGINE_DIR}")

    if(EXISTS "${ENGINE_HPCK}")
        file(COPY_FILE "${ENGINE_HPCK}" "${HIMII_ENGINE_DIR}/engine.hpck" ONLY_IF_DIFFERENT)
    endif()

    foreach(script_core_file ScriptCore.dll ScriptCore.runtimeconfig.json)
        set(source_file "${SCRIPT_CORE_DIR}/${script_core_file}")
        if(EXISTS "${source_file}")
            file(COPY_FILE "${source_file}" "${HIMII_ENGINE_DIR}/${script_core_file}" ONLY_IF_DIFFERENT)
        endif()
    endforeach()
else()
    # Debug Runtime mirrors slim pack: shaders only (fonts/skybox come from the game project).
    # Remove stale full-tree copies from older POST_BUILD layouts.
    if(EXISTS "${TARGET_DIR}/resources")
        file(REMOVE_RECURSE "${TARGET_DIR}/resources")
    endif()
    if(EXISTS "${TARGET_DIR}/assets/fonts")
        file(REMOVE_RECURSE "${TARGET_DIR}/assets/fonts")
    endif()

    file(MAKE_DIRECTORY "${TARGET_DIR}/assets/shaders")
    file(COPY "${HIMII_EDITOR_DIR}/assets/shaders/." DESTINATION "${TARGET_DIR}/assets/shaders")

    foreach(script_core_file ScriptCore.dll ScriptCore.pdb ScriptCore.runtimeconfig.json)
        set(source_file "${SCRIPT_CORE_DIR}/${script_core_file}")
        if(EXISTS "${source_file}")
            file(COPY_FILE "${source_file}" "${TARGET_DIR}/${script_core_file}" ONLY_IF_DIFFERENT)
        endif()
    endforeach()
endif()
