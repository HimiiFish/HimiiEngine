#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "EngineCore/Core/UUID.h"
#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"
#include "Module/Render/RenderCore/Texture.h"

namespace Himii
{
    class EditorCommandHistory;

    struct ComponentInspectorDrawContext
    {
        Ref<Scene> scene;
        Entity entity;
        EditorCommandHistory* commandHistory = nullptr;

        // 从组件 key 获取图标（由 SceneHierarchyPanel 提供）
        std::function<Ref<Texture2D>(const std::string& iconKey)> getComponentIcon;

        // 打开联动面板（由 SceneHierarchyPanel 提供）
        std::function<void(AssetHandle)> requestTextureInspector;
        std::function<void(AssetHandle)> requestParticleEmitterEditor;
        std::function<void(AssetHandle)> requestTileMapEditor;
        std::function<void(AssetHandle)> requestMaterialEditor;
        std::function<void(const std::filesystem::path&)> requestAnimationEditor;
    };
} // namespace Himii

