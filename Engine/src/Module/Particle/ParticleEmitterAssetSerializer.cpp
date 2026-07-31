#include "Hepch.h"
#include "Module/Particle/ParticleEmitterAssetSerializer.h"
#include "EngineCore/Core/Log.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{

    static void WriteVec3(YAML::Emitter& out, const char* key, const glm::vec3& v)
    {
        out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    }
    static void WriteVec4(YAML::Emitter& out, const char* key, const glm::vec4& v)
    {
        out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    }
    static glm::vec3 ReadVec3(const YAML::Node& node, const char* key, const glm::vec3& def = {})
    {
        if (!node[key] || !node[key].IsSequence() || node[key].size() < 3) return def;
        return { node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>() };
    }
    static glm::vec4 ReadVec4(const YAML::Node& node, const char* key, const glm::vec4& def = {})
    {
        if (!node[key] || !node[key].IsSequence() || node[key].size() < 4) return def;
        return { node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>(), node[key][3].as<float>() };
    }

    void ParticleEmitterAssetSerializer::Serialize(const std::filesystem::path& filepath, const Ref<ParticleEmitterAsset>& asset)
    {
        if (!asset) return;
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "ParticleEmitter";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)asset->Handle;

        const auto& p = asset->TemplateProps;
        WriteVec3(out, "Position", p.position);
        WriteVec3(out, "Velocity", p.velocity);
        WriteVec3(out, "VelocityVariation", p.velocityVariation);
        out << YAML::Key << "Lifetime" << YAML::Value << p.lifetime;
        WriteVec4(out, "ColorBegin", p.colorBegin);
        WriteVec4(out, "ColorEnd", p.colorEnd);
        out << YAML::Key << "SizeBegin" << YAML::Value << p.sizeBegin;
        out << YAML::Key << "SizeEnd" << YAML::Value << p.sizeEnd;
        out << YAML::Key << "Shape" << YAML::Value << static_cast<int>(p.shape);
        out << YAML::Key << "TextureHandle" << YAML::Value << static_cast<uint64_t>(p.textureHandle);

        out << YAML::Key << "EmissionRate" << YAML::Value << asset->EmissionRate;
        out << YAML::Key << "Looping" << YAML::Value << asset->Looping;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<ParticleEmitterAsset> ParticleEmitterAssetSerializer::Deserialize(const std::filesystem::path& filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open ParticleEmitterAsset file: {0}", filepath.string());
                return nullptr;
            }
            std::stringstream strStream;
            strStream << stream.rdbuf();
            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "ParticleEmitter")
            {
                HIMII_CORE_ERROR("Invalid ParticleEmitterAsset: {0}", filepath.string());
                return nullptr;
            }

            Ref<ParticleEmitterAsset> asset = CreateRef<ParticleEmitterAsset>();
            if (data["Handle"])
                asset->Handle = data["Handle"].as<uint64_t>();

            asset->TemplateProps.position = ReadVec3(data, "Position");
            asset->TemplateProps.velocity = ReadVec3(data, "Velocity");
            asset->TemplateProps.velocityVariation = ReadVec3(data, "VelocityVariation");
            if (data["Lifetime"]) asset->TemplateProps.lifetime = data["Lifetime"].as<float>();
            asset->TemplateProps.colorBegin = ReadVec4(data, "ColorBegin", { 1, 1, 1, 1 });
            asset->TemplateProps.colorEnd = ReadVec4(data, "ColorEnd", { 0, 0, 0, 0 });
            if (data["SizeBegin"]) asset->TemplateProps.sizeBegin = data["SizeBegin"].as<float>();
            if (data["SizeEnd"]) asset->TemplateProps.sizeEnd = data["SizeEnd"].as<float>();
            if (data["Shape"]) asset->TemplateProps.shape = static_cast<ParticleShape>(std::clamp(data["Shape"].as<int>(), 0, 1));
            if (data["TextureHandle"]) asset->TemplateProps.textureHandle = data["TextureHandle"].as<uint64_t>();

            if (data["EmissionRate"]) asset->EmissionRate = data["EmissionRate"].as<float>();
            if (data["Looping"]) asset->Looping = data["Looping"].as<bool>();

            return asset;
        }
        catch (const YAML::Exception& e)
        {
            HIMII_CORE_ERROR("Failed to deserialize ParticleEmitterAsset '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

} // namespace Himii

