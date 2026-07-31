#pragma once

#include "Resource/Asset.h"
#include "EngineCore/Core/UUID.h"
#include "Module/Render/Texture.h"

#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Himii
{

    struct SpriteDefinition
    {
        AssetHandle Handle = 0;
        std::string Name;
        glm::ivec4 PixelRect{0, 0, 0, 0};
        glm::vec2 Pivot{0.5f, 0.5f};
        glm::vec4 Border{0.0f, 0.0f, 0.0f, 0.0f};
    };

    enum class TextureSpriteMode : uint8_t
    {
        None = 0,
        Single = 1,
        Multiple = 2
    };

    struct TextureImportData
    {
        AssetHandle TextureHandle = 0;
        TextureSpriteMode SpriteMode = TextureSpriteMode::Single;
        uint32_t PixelsPerUnit = 100;
        glm::ivec2 GridCellSize{0, 0};
        glm::ivec2 GridOffset{0, 0};
        glm::ivec2 GridPadding{0, 0};
        std::vector<SpriteDefinition> Sprites;
    };

    struct SpriteResolved
    {
        Ref<Texture2D> Texture;
        std::array<glm::vec2, 4> UVs{};
        glm::vec2 Pivot{0.5f, 0.5f};
        glm::ivec2 PixelSize{0, 0};
        uint32_t PixelsPerUnit = 100;
        bool IsValid = false;
    };

} // namespace Himii
