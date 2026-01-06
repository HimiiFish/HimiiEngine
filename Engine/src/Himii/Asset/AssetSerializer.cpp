#include "Himii/Asset/AssetSerializer.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include "Himii/Core/Log.h"

namespace Himii
{

    void SpriteAnimationSerializer::Serialize(const std::filesystem::path &filepath,
                                              const Ref<SpriteAnimation> &animation)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "SpriteAnimation";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)animation->Handle;

        out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
        for (const auto &frameHandle: animation->GetFrames())
        {
            out << (uint64_t)frameHandle;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<SpriteAnimation> SpriteAnimationSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        std::ifstream stream(filepath);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["AssetType"] || data["AssetType"].as<std::string>() != "SpriteAnimation")
        {
            HIMII_CORE_ERROR("Invalid SpriteAnimation asset: {0}", filepath.string());
            return nullptr;
        }

        Ref<SpriteAnimation> animation = std::make_shared<SpriteAnimation>();

        if (data["Handle"])
            animation->Handle = data["Handle"].as<uint64_t>();

        if (data["Frames"])
        {
            for (auto frameNode: data["Frames"])
            {
                uint64_t handle = frameNode.as<uint64_t>();
                animation->AddFrame(handle);
            }
        }

        return animation;
    }

} // namespace Himii
