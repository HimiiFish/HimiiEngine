#include "Hepch.h"
#include "Module/Render/Mesh/GltfMeshImporter.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Project/Project.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <cstring>

#include "Module/Render/Mesh/cgltf.h"

namespace Himii
{
    namespace
    {
        std::string SanitizeFileStem(const std::string &stem)
        {
            std::string result = stem;
            for (char &character : result)
            {
                if (character == ' ' || character == ':' || character == '\\' || character == '/')
                    character = '_';
            }
            return result;
        }

        bool WriteEmbeddedImageFile(const cgltf_image *image, std::filesystem::path &inputOutputPath)
        {
            if (!image)
                return false;

            if (image->uri && std::strlen(image->uri) > 0 && std::strncmp(image->uri, "data:", 5) != 0)
            {
                // 外部文件：由调用方走相对路径 Import，这里不写出。
                return false;
            }

            const cgltf_buffer_view *bufferView = image->buffer_view;
            if (!bufferView || !bufferView->buffer || !bufferView->buffer->data)
                return false;

            const uint8_t *bytes =
                    static_cast<const uint8_t *>(bufferView->buffer->data) + bufferView->offset;
            const int byteLength = static_cast<int>(bufferView->size);

            const char *mimeType = image->mime_type ? image->mime_type : "";
            if (std::strstr(mimeType, "jpeg") || std::strstr(mimeType, "jpg"))
                inputOutputPath.replace_extension(".jpg");
            else
                inputOutputPath.replace_extension(".png");

            std::ofstream outputFile(inputOutputPath, std::ios::binary);
            if (!outputFile.is_open())
                return false;
            outputFile.write(reinterpret_cast<const char *>(bytes), byteLength);
            return outputFile.good();
        }

        AssetHandle ImportOrGetTexture(AssetManager &assetManager,
                                       const std::filesystem::path &relativeTexturePath)
        {
            return assetManager.ImportAsset(relativeTexturePath.generic_string());
        }
    }

    bool GltfMeshImporter::ImportCompanionAssets(AssetManager &assetManager,
                                                 const std::filesystem::path &relativeMeshPath,
                                                 MeshCompanionImportResult *outResult)
    {
        if (outResult)
        {
            outResult->MaterialHandles.clear();
            outResult->MaterialSlotNames.clear();
        }

        const std::filesystem::path absoluteMeshPath = Project::GetAssetFileSystemPath(relativeMeshPath);
        if (!std::filesystem::exists(absoluteMeshPath))
        {
            HIMII_CORE_ERROR("glTF import failed, file missing: {0}", absoluteMeshPath.string());
            return false;
        }

        cgltf_options options = {};
        cgltf_data *data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, absoluteMeshPath.string().c_str(), &data);
        if (result != cgltf_result_success || !data)
        {
            HIMII_CORE_ERROR("glTF import parse failed: {0}", absoluteMeshPath.string());
            return false;
        }

        result = cgltf_load_buffers(&options, data, absoluteMeshPath.string().c_str());
        if (result != cgltf_result_success)
        {
            HIMII_CORE_ERROR("glTF import buffer load failed: {0}", absoluteMeshPath.string());
            cgltf_free(data);
            return false;
        }

        const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
        const std::string meshStem = SanitizeFileStem(relativeMeshPath.stem().string());
        const std::filesystem::path companionRelativeDirectory =
                relativeMeshPath.parent_path() / (meshStem + "_imported");
        const std::filesystem::path companionAbsoluteDirectory =
                assetDirectory / companionRelativeDirectory;
        std::filesystem::create_directories(companionAbsoluteDirectory);

        std::vector<AssetHandle> imageHandles(data->images_count, 0);
        std::vector<std::string> imageRelativePaths(data->images_count);
        for (cgltf_size imageIndex = 0; imageIndex < data->images_count; ++imageIndex)
        {
            const cgltf_image *image = &data->images[imageIndex];
            std::filesystem::path relativeTexturePath;

            if (image->uri && std::strlen(image->uri) > 0 && std::strncmp(image->uri, "data:", 5) != 0)
            {
                // 相对 URI：相对 glTF 文件所在目录。
                std::filesystem::path uriPath = image->uri;
                std::filesystem::path absoluteTexturePath = absoluteMeshPath.parent_path() / uriPath;
                if (std::filesystem::exists(absoluteTexturePath))
                {
                    relativeTexturePath = std::filesystem::relative(absoluteTexturePath, assetDirectory);
                    imageHandles[imageIndex] = ImportOrGetTexture(assetManager, relativeTexturePath);
                    imageRelativePaths[imageIndex] = relativeTexturePath.generic_string();
                }
                continue;
            }

            std::filesystem::path absoluteTexturePath =
                    companionAbsoluteDirectory / ("texture_" + std::to_string(imageIndex) + ".png");
            if (!WriteEmbeddedImageFile(image, absoluteTexturePath))
            {
                HIMII_CORE_WARNING("Failed to extract glTF image {0}", static_cast<uint32_t>(imageIndex));
                continue;
            }

            relativeTexturePath = std::filesystem::relative(absoluteTexturePath, assetDirectory);
            imageHandles[imageIndex] = ImportOrGetTexture(assetManager, relativeTexturePath);
            imageRelativePaths[imageIndex] = relativeTexturePath.generic_string();
        }

        auto ResolveTextureHandle = [&](const cgltf_texture *texture) -> AssetHandle
        {
            if (!texture || !texture->image)
                return 0;
            const cgltf_size imageIndex = static_cast<cgltf_size>(texture->image - data->images);
            if (imageIndex >= imageHandles.size())
                return 0;
            return imageHandles[imageIndex];
        };

        auto ResolveTextureRelativePath = [&](const cgltf_texture *texture) -> std::string
        {
            if (!texture || !texture->image)
                return {};
            const cgltf_size imageIndex = static_cast<cgltf_size>(texture->image - data->images);
            if (imageIndex >= imageRelativePaths.size())
                return {};
            return imageRelativePaths[imageIndex];
        };

        std::vector<AssetHandle> materialHandles;
        std::vector<std::string> materialSlotNames;
        const cgltf_size materialCount = data->materials_count > 0 ? data->materials_count : 1;
        materialHandles.reserve(materialCount);
        materialSlotNames.reserve(materialCount);

        for (cgltf_size materialIndex = 0; materialIndex < materialCount; ++materialIndex)
        {
            Ref<MaterialAsset> materialAsset = MaterialAssetSerializer::CreateDefaultMaterialInstance(
                    BuiltinShaderRegistry::GetDefaultLitShaderHandle());
            materialAsset->Handle = AssetHandle();

            std::string slotName = "Slot " + std::to_string(materialIndex);
            if (data->materials_count > 0)
            {
                const cgltf_material &material = data->materials[materialIndex];
                if (material.name && material.name[0] != '\0')
                    slotName = material.name;
                if (material.has_pbr_metallic_roughness)
                {
                    const cgltf_float *factor = material.pbr_metallic_roughness.base_color_factor;
                    materialAsset->SetColorParameter("u_AlbedoColor",
                                                     {factor[0], factor[1], factor[2], factor[3]});
                    const cgltf_texture *baseColorTexture =
                            material.pbr_metallic_roughness.base_color_texture.texture;
                    materialAsset->SetTextureParameter("u_AlbedoTexture", ResolveTextureHandle(baseColorTexture),
                                                     ResolveTextureRelativePath(baseColorTexture));
                }
            }

            const std::string materialFileName = "material_" + std::to_string(materialIndex) + ".hmaterial";
            const std::filesystem::path absoluteMaterialPath = companionAbsoluteDirectory / materialFileName;
            MaterialAssetSerializer::Serialize(absoluteMaterialPath, materialAsset);

            const std::filesystem::path relativeMaterialPath =
                    std::filesystem::relative(absoluteMaterialPath, assetDirectory);
            AssetHandle materialHandle = assetManager.ImportAsset(relativeMaterialPath.generic_string());
            if (materialHandle != 0)
            {
                materialAsset->Handle = materialHandle;
                MaterialAssetSerializer::Serialize(absoluteMaterialPath, materialAsset);
            }
            materialHandles.push_back(materialHandle);
            materialSlotNames.push_back(slotName);
        }

        cgltf_free(data);

        const size_t importedMaterialCount = materialHandles.size();
        if (outResult)
        {
            outResult->MaterialHandles = std::move(materialHandles);
            outResult->MaterialSlotNames = std::move(materialSlotNames);
        }

        HIMII_CORE_INFO("glTF companion import done for {0}: {1} materials", relativeMeshPath.generic_string(),
                        importedMaterialCount);
        return true;
    }
}
