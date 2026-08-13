#include "Hepch.h"
#include "Resource/AssetSerializerRegistry.h"
#include "Resource/IAssetSerializer.h"

#include "Module/Animation/SpriteAnimationSerializer.h"
#include "Module/Audio/SoundAsset.h"
#include "Module/Particle/ParticleEmitterAssetSerializer.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Environment/EnvironmentMapAsset.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Module/Render/Shader/ShaderAssetSerializer.h"
#include "Module/Render/Renderer/Font.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Tilemap/TileMapDataSerializer.h"
#include "Module/Tilemap/TileSetSerializer.h"

namespace Himii
{
    namespace
    {
        template <typename AssetClass, AssetType TypeValue>
        class FunctionAssetSerializer final : public IAssetSerializer
        {
        public:
            using DeserializeFunction = Ref<AssetClass> (*)(const std::filesystem::path &);

            explicit FunctionAssetSerializer(DeserializeFunction deserializeFunction)
                : m_DeserializeFunction(deserializeFunction)
            {
            }

            AssetType GetAssetType() const override { return TypeValue; }

            Ref<Asset> Deserialize(const std::filesystem::path &filepath) override
            {
                return m_DeserializeFunction(filepath);
            }

        private:
            DeserializeFunction m_DeserializeFunction = nullptr;
        };

        class Texture2DAssetSerializer final : public IAssetSerializer
        {
        public:
            AssetType GetAssetType() const override { return AssetType::Texture2D; }

            Ref<Asset> Deserialize(const std::filesystem::path &filepath) override
            {
                return Texture2D::Create(filepath.string());
            }
        };

        class FontAssetSerializer final : public IAssetSerializer
        {
        public:
            AssetType GetAssetType() const override { return AssetType::Font; }

            Ref<Asset> Deserialize(const std::filesystem::path &filepath) override
            {
                FontSpecification specification;
                specification.FilePath = filepath;
                specification.FaceIndex = 0;
                return CreateRef<Font>(specification);
            }
        };

        class EnvironmentMapAssetSerializer final : public IAssetSerializer
        {
        public:
            AssetType GetAssetType() const override { return AssetType::EnvironmentMap; }

            Ref<Asset> Deserialize(const std::filesystem::path &filepath) override
            {
                Ref<EnvironmentMapAsset> asset = CreateRef<EnvironmentMapAsset>();
                asset->SourceFilePath = filepath;
                EnvironmentMapImportSerializer::EnsureDefaultMeta(filepath);
                EnvironmentMapImportSerializer::Deserialize(filepath, asset->ImportSettings);
                return asset;
            }
        };

        class SoundAssetSerializer final : public IAssetSerializer
        {
        public:
            AssetType GetAssetType() const override { return AssetType::SoundAsset; }

            Ref<Asset> Deserialize(const std::filesystem::path &filepath) override
            {
                return CreateRef<SoundAsset>(filepath);
            }
        };
    }

    void RegisterBuiltinAssetSerializers()
    {
        AssetSerializerRegistry::Clear();

        AssetSerializerRegistry::RegisterExtension(".png", AssetType::Texture2D);
        AssetSerializerRegistry::RegisterExtension(".jpg", AssetType::Texture2D);
        AssetSerializerRegistry::RegisterExtension(".jpeg", AssetType::Texture2D);
        AssetSerializerRegistry::RegisterExtension(".bmp", AssetType::Texture2D);
        AssetSerializerRegistry::RegisterExtension(".hdr", AssetType::EnvironmentMap);
        AssetSerializerRegistry::RegisterExtension(".anim", AssetType::SpriteAnimation);
        AssetSerializerRegistry::RegisterExtension(".himii", AssetType::Scene);
        AssetSerializerRegistry::RegisterExtension(".tileset", AssetType::TileSet);
        AssetSerializerRegistry::RegisterExtension(".tilemap", AssetType::TileMap);
        AssetSerializerRegistry::RegisterExtension(".particle", AssetType::ParticleEmitter);
        AssetSerializerRegistry::RegisterExtension(".hprefab", AssetType::Prefab);
        AssetSerializerRegistry::RegisterExtension(".ttf", AssetType::Font);
        AssetSerializerRegistry::RegisterExtension(".otf", AssetType::Font);
        AssetSerializerRegistry::RegisterExtension(".ttc", AssetType::Font);
        AssetSerializerRegistry::RegisterExtension(".wav", AssetType::SoundAsset);
        AssetSerializerRegistry::RegisterExtension(".ogg", AssetType::SoundAsset);
        AssetSerializerRegistry::RegisterExtension(".mp3", AssetType::SoundAsset);
        AssetSerializerRegistry::RegisterExtension(".hmesh", AssetType::Mesh);
        AssetSerializerRegistry::RegisterExtension(".hmaterial", AssetType::Material);
        AssetSerializerRegistry::RegisterExtension(".hshader", AssetType::Shader);

        AssetSerializerRegistry::Register(CreateScope<Texture2DAssetSerializer>());
        AssetSerializerRegistry::Register(CreateScope<EnvironmentMapAssetSerializer>());
        AssetSerializerRegistry::Register(CreateScope<FontAssetSerializer>());
        AssetSerializerRegistry::Register(CreateScope<SoundAssetSerializer>());
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<SpriteAnimation, AssetType::SpriteAnimation>>(
                        &SpriteAnimationSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<TileSet, AssetType::TileSet>>(
                        &TileSetSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<TileMapData, AssetType::TileMap>>(
                        &TileMapDataSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<ParticleEmitterAsset, AssetType::ParticleEmitter>>(
                        &ParticleEmitterAssetSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<MeshAsset, AssetType::Mesh>>(
                        &MeshAssetSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<MaterialAsset, AssetType::Material>>(
                        &MaterialAssetSerializer::Deserialize));
        AssetSerializerRegistry::Register(
                CreateScope<FunctionAssetSerializer<ShaderAsset, AssetType::Shader>>(
                        &ShaderAssetSerializer::Deserialize));
    }
}
