#include "Hepch.h"
#include "Module/Render/Mesh/HmeshAssetSerializer.h"
#include "EngineCore/Core/Log.h"

#include <cstring>
#include <fstream>

namespace Himii
{
    constexpr uint32_t HmeshFormatVersion = 2u;

    namespace
    {
        constexpr char HmeshMagic[4] = {'H', 'M', 'S', 'H'};

#pragma pack(push, 1)
        struct HmeshFileHeader
        {
            char Magic[4];
            uint32_t Version = HmeshFormatVersion;
            uint32_t VertexCount = 0;
            uint32_t IndexCount = 0;
            uint32_t SubmeshCount = 0;
        };
#pragma pack(pop)

        static bool ReadExact(std::ifstream &inputStream, void *buffer, std::size_t byteCount)
        {
            inputStream.read(static_cast<char *>(buffer), static_cast<std::streamsize>(byteCount));
            return inputStream.good();
        }

        static bool WriteExact(std::ofstream &outputStream, const void *buffer, std::size_t byteCount)
        {
            outputStream.write(static_cast<const char *>(buffer), static_cast<std::streamsize>(byteCount));
            return outputStream.good();
        }
    }

    uint32_t HmeshAssetSerializer::GetCurrentFormatVersion()
    {
        return HmeshFormatVersion;
    }

    bool HmeshAssetSerializer::Serialize(const std::filesystem::path &filepath, const MeshAsset &meshAsset)
    {
        HmeshFileHeader header = {};
        std::memcpy(header.Magic, HmeshMagic, sizeof(HmeshMagic));
        header.Version = HmeshFormatVersion;
        header.VertexCount = static_cast<uint32_t>(meshAsset.Vertices.size());
        header.IndexCount = static_cast<uint32_t>(meshAsset.Indices.size());
        header.SubmeshCount = static_cast<uint32_t>(meshAsset.Submeshes.size());

        std::ofstream outputStream(filepath, std::ios::binary | std::ios::trunc);
        if (!outputStream.is_open())
        {
            HIMII_CORE_ERROR("Failed to write .hmesh file: {0}", filepath.string());
            return false;
        }

        if (!WriteExact(outputStream, &header, sizeof(HmeshFileHeader)))
            return false;

        if (header.VertexCount > 0
            && !WriteExact(outputStream, meshAsset.Vertices.data(),
                           header.VertexCount * sizeof(MeshVertex)))
            return false;

        if (header.IndexCount > 0
            && !WriteExact(outputStream, meshAsset.Indices.data(),
                           header.IndexCount * sizeof(uint32_t)))
            return false;

        if (header.SubmeshCount > 0
            && !WriteExact(outputStream, meshAsset.Submeshes.data(),
                           header.SubmeshCount * sizeof(MeshSubmesh)))
            return false;

        return true;
    }

    Ref<MeshAsset> HmeshAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        std::ifstream inputStream(filepath, std::ios::binary);
        if (!inputStream.is_open())
        {
            HIMII_CORE_ERROR("Failed to open .hmesh file: {0}", filepath.string());
            return nullptr;
        }

        HmeshFileHeader header = {};
        if (!ReadExact(inputStream, &header, sizeof(HmeshFileHeader)))
        {
            HIMII_CORE_ERROR("Failed to read .hmesh header: {0}", filepath.string());
            return nullptr;
        }

        if (std::memcmp(header.Magic, HmeshMagic, sizeof(HmeshMagic)) != 0)
        {
            HIMII_CORE_ERROR("Invalid .hmesh magic: {0}", filepath.string());
            return nullptr;
        }

        if (header.Version != HmeshFormatVersion)
        {
            HIMII_CORE_ERROR(
                    "Unsupported .hmesh version {0} in {1}. Reimport the source mesh to upgrade to version {2}.",
                    header.Version, filepath.string(), HmeshFormatVersion);
            return nullptr;
        }

        Ref<MeshAsset> meshAsset = CreateRef<MeshAsset>();
        meshAsset->Vertices.resize(header.VertexCount);
        meshAsset->Indices.resize(header.IndexCount);
        meshAsset->Submeshes.resize(header.SubmeshCount);

        if (header.VertexCount > 0
            && !ReadExact(inputStream, meshAsset->Vertices.data(),
                          header.VertexCount * sizeof(MeshVertex)))
        {
            HIMII_CORE_ERROR("Failed to read .hmesh vertices: {0}", filepath.string());
            return nullptr;
        }

        if (header.IndexCount > 0
            && !ReadExact(inputStream, meshAsset->Indices.data(),
                          header.IndexCount * sizeof(uint32_t)))
        {
            HIMII_CORE_ERROR("Failed to read .hmesh indices: {0}", filepath.string());
            return nullptr;
        }

        if (header.SubmeshCount > 0
            && !ReadExact(inputStream, meshAsset->Submeshes.data(),
                          header.SubmeshCount * sizeof(MeshSubmesh)))
        {
            HIMII_CORE_ERROR("Failed to read .hmesh submeshes: {0}", filepath.string());
            return nullptr;
        }

        return meshAsset;
    }
}
