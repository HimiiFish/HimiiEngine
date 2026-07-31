#pragma once

#include <array>
#include <algorithm>
#include <string>

#include <box2d/box2d.h>

namespace Himii
{
    static constexpr int Physics2DLayerCount = 8;

    struct Physics2DLayerSettings
    {
        std::array<std::string, Physics2DLayerCount> LayerNames{};
        std::array<std::array<bool, Physics2DLayerCount>, Physics2DLayerCount> CollisionMatrix{};

        Physics2DLayerSettings()
        {
            LayerNames[0] = "Default";
            for (int layerIndex = 1; layerIndex < Physics2DLayerCount; ++layerIndex)
                LayerNames[layerIndex] = "Layer " + std::to_string(layerIndex);

            for (int row = 0; row < Physics2DLayerCount; ++row)
                for (int column = 0; column < Physics2DLayerCount; ++column)
                    CollisionMatrix[row][column] = true;
        }

        b2Filter BuildShapeFilter(int layerIndex) const
        {
            b2Filter filter = b2DefaultFilter();
            layerIndex = std::clamp(layerIndex, 0, Physics2DLayerCount - 1);

            filter.categoryBits = 1u << static_cast<unsigned>(layerIndex);

            uint32_t maskBits = 0;
            for (int column = 0; column < Physics2DLayerCount; ++column)
            {
                if (CollisionMatrix[layerIndex][column])
                    maskBits |= (1u << static_cast<unsigned>(column));
            }

            filter.maskBits = maskBits;
            return filter;
        }
    };
}
