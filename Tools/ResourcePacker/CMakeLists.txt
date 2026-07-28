cmake_minimum_required(VERSION 3.12)
project(ResourcePacker)

add_executable(ResourcePacker main.cpp)
set_target_properties(ResourcePacker PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
himii_set_output_dirs(ResourcePacker)

# Editor fat pack: shaders + fonts + editor resources (icons, splash, skybox).
set(HIMII_EDITOR_ENGINE_HPCK "${CMAKE_BINARY_DIR}/generated/engine.editor.hpck" CACHE INTERNAL
    "Packed editor engine resources (fat)")
# Runtime / Export Template slim pack: shaders only.
set(HIMII_RUNTIME_ENGINE_HPCK "${CMAKE_BINARY_DIR}/generated/engine.runtime.hpck" CACHE INTERNAL
    "Packed runtime engine resources (slim)")

# Backward-compatible alias used by some scripts; prefer EDITOR/RUNTIME variables.
set(HIMII_ENGINE_HPCK "${HIMII_EDITOR_ENGINE_HPCK}" CACHE INTERNAL "Alias for editor fat engine.hpck")

add_custom_command(
    OUTPUT "${HIMII_EDITOR_ENGINE_HPCK}"
    COMMAND $<TARGET_FILE:ResourcePacker>
        --root "${CMAKE_SOURCE_DIR}/HimiiEditor/assets" assets
        --root "${CMAKE_SOURCE_DIR}/HimiiEditor/resources" resources
        --output "${HIMII_EDITOR_ENGINE_HPCK}"
    DEPENDS ResourcePacker
    COMMENT "Packing editor (fat) engine.hpck"
    VERBATIM
)

add_custom_command(
    OUTPUT "${HIMII_RUNTIME_ENGINE_HPCK}"
    COMMAND $<TARGET_FILE:ResourcePacker>
        --root "${CMAKE_SOURCE_DIR}/HimiiEditor/assets/shaders" assets/shaders
        --output "${HIMII_RUNTIME_ENGINE_HPCK}"
    DEPENDS ResourcePacker
    COMMENT "Packing runtime (slim) engine.hpck"
    VERBATIM
)

add_custom_target(PackEngineResources DEPENDS
    "${HIMII_EDITOR_ENGINE_HPCK}"
    "${HIMII_RUNTIME_ENGINE_HPCK}")
