#pragma once

#include "Resource/Asset.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "EngineCore/Core/Core.h"
#include <glm/glm.hpp>
#include <vector>

namespace Himii
{
    struct MeshVertex
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Normal{0.0f, 1.0f, 0.0f};
        glm::vec2 TextureCoordinate{0.0f};
    };

    struct MeshSubmesh
    {
        uint32_t IndexStart = 0;
        uint32_t IndexCount = 0;
        uint32_t MaterialSlotIndex = 0;
    };

    struct MeshSubmeshGpu
    {
        Ref<VertexArray> VertexArray;
        uint32_t IndexCount = 0;
        uint32_t MaterialSlotIndex = 0;
    };

    /// 静态网格资产：CPU 几何 + 按需上传的 GPU 子网格。
    class MeshAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::Mesh; }

        std::vector<MeshVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<MeshSubmesh> Submeshes;
        std::vector<AssetHandle> DefaultMaterialHandles;

        void EnsureGpuResources();
        const std::vector<MeshSubmeshGpu> &GetGpuSubmeshes() const { return m_GpuSubmeshes; }
        bool HasGpuResources() const { return m_GpuReady; }

    private:
        std::vector<MeshSubmeshGpu> m_GpuSubmeshes;
        bool m_GpuReady = false;
    };
}
