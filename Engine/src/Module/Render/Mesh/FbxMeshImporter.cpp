#include "Hepch.h"
#include "Module/Render/Mesh/FbxMeshImporter.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Project/Project.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>

#include "Module/Render/Mesh/ufbx.h"

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

        std::string ToStdString(const ufbx_string &value)
        {
            if (!value.data || value.length == 0)
                return {};
            return std::string(value.data, value.length);
        }

        std::string ToLowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return value;
        }

        bool IsImageFilePath(const std::filesystem::path &path)
        {
            const std::string extension = ToLowerAscii(path.extension().string());
            return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp"
                   || extension == ".tga" || extension == ".tif" || extension == ".tiff" || extension == ".psd";
        }

        bool WriteEmbeddedBlob(const ufbx_blob &blob, std::filesystem::path &inputOutputPath)
        {
            if (!blob.data || blob.size == 0)
                return false;

            if (blob.size >= 3
                && static_cast<const unsigned char *>(blob.data)[0] == 0xFFu
                && static_cast<const unsigned char *>(blob.data)[1] == 0xD8u)
            {
                inputOutputPath.replace_extension(".jpg");
            }
            else
            {
                inputOutputPath.replace_extension(".png");
            }

            std::ofstream outputFile(inputOutputPath, std::ios::binary);
            if (!outputFile.is_open())
                return false;
            outputFile.write(static_cast<const char *>(blob.data),
                             static_cast<std::streamsize>(blob.size));
            return outputFile.good();
        }

        AssetHandle ImportOrGetTexture(AssetManager &assetManager,
                                       const std::filesystem::path &relativeTexturePath)
        {
            return assetManager.ImportAsset(relativeTexturePath.generic_string());
        }

        bool IsPathInsideDirectory(const std::filesystem::path &path,
                                   const std::filesystem::path &directory)
        {
            std::error_code errorCode;
            const std::filesystem::path relativePath =
                    std::filesystem::relative(path, directory, errorCode);
            if (errorCode || relativePath.empty())
                return path == directory;
            const std::string relativeString = relativePath.generic_string();
            return relativeString != ".." && relativeString.rfind("../", 0) != 0;
        }

        const ufbx_texture *ResolveFileTexture(const ufbx_texture *texture)
        {
            if (!texture)
                return nullptr;
            if (texture->type == UFBX_TEXTURE_FILE)
                return texture;
            if (texture->file_textures.count > 0)
                return texture->file_textures.data[0];
            return nullptr;
        }

        /// Extract a usable file name from any FBX-recorded path (absolute or relative).
        std::filesystem::path ExtractTextureFileName(const std::string &recordedPath)
        {
            if (recordedPath.empty())
                return {};
            return std::filesystem::path(recordedPath).filename();
        }

        std::filesystem::path FindTextureByFileName(const std::filesystem::path &searchRoot,
                                                    const std::filesystem::path &fileName,
                                                    uint32_t maximumDepth)
        {
            if (searchRoot.empty() || fileName.empty() || !std::filesystem::exists(searchRoot))
                return {};

            const std::string targetNameLower = ToLowerAscii(fileName.string());
            std::error_code errorCode;
            const auto directoryOptions = std::filesystem::directory_options::skip_permission_denied;
            for (std::filesystem::recursive_directory_iterator iterator(searchRoot, directoryOptions, errorCode),
                 end;
                 iterator != end && !errorCode; iterator.increment(errorCode))
            {
                if (errorCode)
                    break;
                if (iterator.depth() > static_cast<int>(maximumDepth))
                {
                    iterator.disable_recursion_pending();
                    continue;
                }
                if (!iterator->is_regular_file())
                    continue;

                const std::filesystem::path candidate = iterator->path();
                if (ToLowerAscii(candidate.filename().string()) == targetNameLower)
                    return candidate;
            }
            return {};
        }

        std::filesystem::path ResolveExternalTexturePath(const ufbx_texture *fileTexture,
                                                         const std::filesystem::path &absoluteMeshPath)
        {
            const std::filesystem::path meshDirectory = absoluteMeshPath.parent_path();
            const std::string absoluteFilename = ToStdString(fileTexture->absolute_filename);
            const std::string relativeFilename = ToStdString(fileTexture->relative_filename);
            const std::string filename = ToStdString(fileTexture->filename);

            std::vector<std::filesystem::path> candidates;
            auto AddCandidate = [&](const std::filesystem::path &candidate)
            {
                if (candidate.empty())
                    return;
                candidates.push_back(candidate);
            };

            if (!absoluteFilename.empty())
                AddCandidate(absoluteFilename);
            if (!relativeFilename.empty())
            {
                AddCandidate(meshDirectory / relativeFilename);
                AddCandidate(std::filesystem::path(relativeFilename));
            }
            if (!filename.empty())
                AddCandidate(meshDirectory / filename);

            const std::filesystem::path bareFileName = ExtractTextureFileName(
                    !filename.empty() ? filename
                                      : (!relativeFilename.empty() ? relativeFilename : absoluteFilename));
            if (!bareFileName.empty())
            {
                static const char *kCommonTextureFolders[] = {
                        "textures", "Textures", "texture", "Texture", "maps", "Maps",
                        "materials", "Materials", "images", "Images"};
                AddCandidate(meshDirectory / bareFileName);
                for (const char *folderName : kCommonTextureFolders)
                    AddCandidate(meshDirectory / folderName / bareFileName);
            }

            for (const std::filesystem::path &candidate : candidates)
            {
                std::error_code errorCode;
                if (std::filesystem::exists(candidate, errorCode) && !errorCode && IsImageFilePath(candidate))
                    return std::filesystem::weakly_canonical(candidate, errorCode);
            }

            // Last resort: search by file name under the FBX folder (limited depth).
            if (!bareFileName.empty())
            {
                const std::filesystem::path found =
                        FindTextureByFileName(meshDirectory, bareFileName, 4);
                if (!found.empty())
                    return found;
            }

            return {};
        }

        AssetHandle ImportResolvedTextureFile(AssetManager &assetManager,
                                              const std::filesystem::path &absoluteTexturePath,
                                              const std::filesystem::path &assetDirectory,
                                              const std::filesystem::path &companionAbsoluteDirectory)
        {
            if (absoluteTexturePath.empty() || !std::filesystem::exists(absoluteTexturePath))
                return 0;

            if (IsPathInsideDirectory(absoluteTexturePath, assetDirectory))
            {
                const std::filesystem::path relativeTexturePath =
                        std::filesystem::relative(absoluteTexturePath, assetDirectory);
                return ImportOrGetTexture(assetManager, relativeTexturePath);
            }

            const std::filesystem::path copiedPath =
                    companionAbsoluteDirectory / absoluteTexturePath.filename();
            try
            {
                std::filesystem::copy_file(absoluteTexturePath, copiedPath,
                                           std::filesystem::copy_options::overwrite_existing);
            }
            catch (const std::filesystem::filesystem_error &exception)
            {
                HIMII_CORE_WARNING("Failed to copy FBX texture '{0}': {1}", absoluteTexturePath.string(),
                                   exception.what());
                return 0;
            }

            const std::filesystem::path relativeTexturePath =
                    std::filesystem::relative(copiedPath, assetDirectory);
            return ImportOrGetTexture(assetManager, relativeTexturePath);
        }

        struct ImportedTextureReference
        {
            AssetHandle Handle = 0;
            std::string RelativePath;
        };

        ImportedTextureReference ImportTextureFromFbx(AssetManager &assetManager,
                                                      const ufbx_texture *texture,
                                                      const std::filesystem::path &absoluteMeshPath,
                                                      const std::filesystem::path &assetDirectory,
                                                      const std::filesystem::path &companionAbsoluteDirectory,
                                                      uint32_t textureIndex)
        {
            ImportedTextureReference reference;
            const ufbx_texture *fileTexture = ResolveFileTexture(texture);
            if (!fileTexture)
                return reference;

            if (fileTexture->content.data && fileTexture->content.size > 0)
            {
                std::filesystem::path absoluteTexturePath =
                        companionAbsoluteDirectory / ("texture_" + std::to_string(textureIndex) + ".png");
                if (!WriteEmbeddedBlob(fileTexture->content, absoluteTexturePath))
                    return reference;
                const std::filesystem::path relativeTexturePath =
                        std::filesystem::relative(absoluteTexturePath, assetDirectory);
                reference.RelativePath = relativeTexturePath.generic_string();
                reference.Handle = ImportOrGetTexture(assetManager, relativeTexturePath);
                return reference;
            }

            const std::filesystem::path absoluteTexturePath =
                    ResolveExternalTexturePath(fileTexture, absoluteMeshPath);
            if (absoluteTexturePath.empty())
            {
                HIMII_CORE_WARNING(
                        "FBX texture not found on disk (recorded absolute='{0}', relative='{1}', file='{2}')",
                        ToStdString(fileTexture->absolute_filename),
                        ToStdString(fileTexture->relative_filename),
                        ToStdString(fileTexture->filename));
                return reference;
            }

            reference.Handle = ImportResolvedTextureFile(assetManager, absoluteTexturePath, assetDirectory,
                                                         companionAbsoluteDirectory);
            if (reference.Handle != 0)
            {
                const auto &registry = assetManager.GetAssetRegistry();
                const auto iterator = registry.find(reference.Handle);
                if (iterator != registry.end())
                    reference.RelativePath = iterator->second.FilePath.generic_string();
            }
            return reference;
        }

        void ApplyAlbedoColor(const ufbx_material_map &materialMap, MaterialAsset &materialAsset)
        {
            if (!(materialMap.has_value || materialMap.value_components >= 3))
                return;
            materialAsset.SetColorParameter(
                    "u_AlbedoColor",
                    {materialMap.value_vec4.x, materialMap.value_vec4.y, materialMap.value_vec4.z,
                     materialMap.value_components >= 4 ? materialMap.value_vec4.w : 1.0f});
        }

        bool MaterialMapHasTexture(const ufbx_material_map &materialMap)
        {
            return materialMap.texture != nullptr;
        }

        bool TryBindAlbedoFromMap(AssetManager &assetManager, const ufbx_material_map &materialMap,
                                  MaterialAsset &materialAsset, const std::filesystem::path &absoluteMeshPath,
                                  const std::filesystem::path &assetDirectory,
                                  const std::filesystem::path &companionAbsoluteDirectory,
                                  uint32_t &textureIndex)
        {
            ApplyAlbedoColor(materialMap, materialAsset);
            if (!MaterialMapHasTexture(materialMap))
            {
                AssetHandle textureHandle = 0;
                std::string textureRelativePath;
                return materialAsset.TryGetTextureParameter("u_AlbedoTexture", textureHandle,
                                                              textureRelativePath)
                       && textureHandle != 0;
            }

            const ImportedTextureReference textureReference = ImportTextureFromFbx(
                    assetManager, materialMap.texture, absoluteMeshPath, assetDirectory,
                    companionAbsoluteDirectory, textureIndex++);
            if (textureReference.Handle == 0)
                return false;

            materialAsset.SetTextureParameter("u_AlbedoTexture", textureReference.Handle,
                                              textureReference.RelativePath);
            return true;
        }

        bool IsLikelyAlbedoPropertyName(const std::string &propertyNameLower)
        {
            return propertyNameLower.find("diffuse") != std::string::npos
                   || propertyNameLower.find("base_color") != std::string::npos
                   || propertyNameLower.find("basecolor") != std::string::npos
                   || propertyNameLower.find("albedo") != std::string::npos
                   || propertyNameLower == "color";
        }

        bool TryBindAlbedoFromMaterialTextures(AssetManager &assetManager, const ufbx_material *material,
                                               MaterialAsset &materialAsset,
                                               const std::filesystem::path &absoluteMeshPath,
                                               const std::filesystem::path &assetDirectory,
                                               const std::filesystem::path &companionAbsoluteDirectory,
                                               uint32_t &textureIndex)
        {
            if (!material)
                return false;

            for (size_t textureEntryIndex = 0; textureEntryIndex < material->textures.count; ++textureEntryIndex)
            {
                const ufbx_material_texture &materialTexture = material->textures.data[textureEntryIndex];
                const std::string propertyNameLower = ToLowerAscii(ToStdString(materialTexture.material_prop));
                if (!IsLikelyAlbedoPropertyName(propertyNameLower))
                    continue;
                if (!materialTexture.texture)
                    continue;

                const ImportedTextureReference textureReference = ImportTextureFromFbx(
                        assetManager, materialTexture.texture, absoluteMeshPath, assetDirectory,
                        companionAbsoluteDirectory, textureIndex++);
                if (textureReference.Handle == 0)
                    continue;

                materialAsset.SetTextureParameter("u_AlbedoTexture", textureReference.Handle,
                                                  textureReference.RelativePath);
                return true;
            }
            return false;
        }
    }

    bool FbxMeshImporter::ImportCompanionAssets(AssetManager &assetManager,
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
            HIMII_CORE_ERROR("FBX import failed, file missing: {0}", absoluteMeshPath.string());
            return false;
        }

        ufbx_load_opts loadOptions = {};
        loadOptions.generate_missing_normals = true;
        ufbx_error error = {};
        ufbx_scene *scene =
                ufbx_load_file(absoluteMeshPath.string().c_str(), &loadOptions, &error);
        if (!scene)
        {
            char errorBuffer[512];
            ufbx_format_error(errorBuffer, sizeof(errorBuffer), &error);
            HIMII_CORE_ERROR("FBX companion parse failed '{0}': {1}", absoluteMeshPath.string(),
                             errorBuffer);
            return false;
        }

        const std::filesystem::path assetDirectory = Project::GetAssetDirectory();
        const std::string meshStem = SanitizeFileStem(relativeMeshPath.stem().string());
        const std::filesystem::path companionRelativeDirectory =
                relativeMeshPath.parent_path() / (meshStem + "_imported");
        const std::filesystem::path companionAbsoluteDirectory =
                assetDirectory / companionRelativeDirectory;
        std::filesystem::create_directories(companionAbsoluteDirectory);

        uint32_t nextTextureIndex = 0;
        std::vector<AssetHandle> materialHandles;
        std::vector<std::string> materialSlotNames;
        const size_t materialCount = scene->materials.count > 0 ? scene->materials.count : 1;
        materialHandles.reserve(materialCount);
        materialSlotNames.reserve(materialCount);

        for (size_t materialIndex = 0; materialIndex < materialCount; ++materialIndex)
        {
            Ref<MaterialAsset> materialAsset = MaterialAssetSerializer::CreateDefaultMaterialInstance(
                    BuiltinShaderRegistry::GetDefaultLitShaderHandle());
            materialAsset->Handle = AssetHandle();

            std::string slotName = "Slot " + std::to_string(materialIndex);
            if (scene->materials.count > 0)
            {
                const ufbx_material *material = scene->materials.data[materialIndex];
                const std::string materialName = ToStdString(material->name);
                if (!materialName.empty())
                    slotName = materialName;

                bool albedoBound = false;

                // Prefer PBR base color, then classic FBX diffuse; bind texture whenever a path resolves.
                if (material->pbr.base_color.texture || material->pbr.base_color.has_value)
                {
                    albedoBound = TryBindAlbedoFromMap(
                            assetManager, material->pbr.base_color, *materialAsset, absoluteMeshPath,
                            assetDirectory, companionAbsoluteDirectory, nextTextureIndex);
                }
                if (!albedoBound
                    && (material->fbx.diffuse_color.texture || material->fbx.diffuse_color.has_value))
                {
                    albedoBound = TryBindAlbedoFromMap(
                            assetManager, material->fbx.diffuse_color, *materialAsset, absoluteMeshPath,
                            assetDirectory, companionAbsoluteDirectory, nextTextureIndex);
                }
                if (!albedoBound)
                {
                    albedoBound = TryBindAlbedoFromMaterialTextures(
                            assetManager, material, *materialAsset, absoluteMeshPath, assetDirectory,
                            companionAbsoluteDirectory, nextTextureIndex);
                }

                if (!albedoBound)
                {
                    HIMII_CORE_WARNING(
                            "FBX material '{0}' imported without albedo texture; bind manually if needed",
                            ToStdString(material->name));
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

        ufbx_free_scene(scene);

        const size_t importedMaterialCount = materialHandles.size();
        if (outResult)
        {
            outResult->MaterialHandles = std::move(materialHandles);
            outResult->MaterialSlotNames = std::move(materialSlotNames);
        }

        HIMII_CORE_INFO("FBX companion import done for {0}: {1} materials", relativeMeshPath.generic_string(),
                        importedMaterialCount);
        return true;
    }
}
