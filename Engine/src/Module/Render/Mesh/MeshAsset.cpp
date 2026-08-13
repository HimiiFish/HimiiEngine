#include "Hepch.h"
#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/RHI/RHI.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    void MeshAsset::EnsureGpuResources()
    {
        if (m_GpuReady)
            return;

        m_GpuSubmeshes.clear();
        if (Vertices.empty() || Indices.empty() || Submeshes.empty())
        {
            HIMII_CORE_WARNING("MeshAsset has empty geometry; skipping GPU upload.");
            m_GpuReady = true;
            return;
        }

        Ref<VertexBuffer> sharedVertexBuffer =
                RHI::CreateVertexBuffer(static_cast<uint32_t>(Vertices.size() * sizeof(MeshVertex)));
        sharedVertexBuffer->SetData(Vertices.data(), static_cast<uint32_t>(Vertices.size() * sizeof(MeshVertex)));
        sharedVertexBuffer->SetLayout({{ShaderDataType::Float3, "a_Position"},
                                       {ShaderDataType::Float3, "a_Normal"},
                                       {ShaderDataType::Float2, "a_TextureCoordinate"},
                                       {ShaderDataType::Float4, "a_Tangent"}});

        for (const MeshSubmesh &submesh : Submeshes)
        {
            if (submesh.IndexCount == 0)
                continue;
            if (submesh.IndexStart + submesh.IndexCount > Indices.size())
            {
                HIMII_CORE_ERROR("MeshAsset submesh index range out of bounds.");
                continue;
            }

            std::vector<uint32_t> submeshIndices(Indices.begin() + static_cast<std::ptrdiff_t>(submesh.IndexStart),
                                                 Indices.begin()
                                                         + static_cast<std::ptrdiff_t>(submesh.IndexStart
                                                                                       + submesh.IndexCount));

            MeshSubmeshGpu gpuSubmesh;
            gpuSubmesh.VertexArray = RHI::CreateVertexArray();
            gpuSubmesh.VertexArray->AddVertexBuffer(sharedVertexBuffer);
            Ref<IndexBuffer> indexBuffer =
                    RHI::CreateIndexBuffer(submeshIndices.data(), static_cast<uint32_t>(submeshIndices.size()));
            gpuSubmesh.VertexArray->SetIndexBuffer(indexBuffer);
            gpuSubmesh.IndexCount = static_cast<uint32_t>(submeshIndices.size());
            gpuSubmesh.MaterialSlotIndex = submesh.MaterialSlotIndex;
            m_GpuSubmeshes.push_back(std::move(gpuSubmesh));
        }

        m_GpuReady = true;
    }
}
