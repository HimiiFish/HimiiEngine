#pragma once

#include <array>
#include <algorithm>
#include <string>

namespace Himii
{
    static constexpr int SortingLayerCount = 32;

    struct SortingLayerSettings
    {
        std::array<std::string, SortingLayerCount> LayerNames{};

        SortingLayerSettings()
        {
            LayerNames[0] = "Default";
            for (int layerIndex = 1; layerIndex < SortingLayerCount; ++layerIndex)
                LayerNames[layerIndex] = "Layer " + std::to_string(layerIndex);
        }
    };
}
