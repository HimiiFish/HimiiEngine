#include "Hepch.h"
#include "Module/Animation/SpriteAnimationSerializer.h"
#include "Module/Animation/SpriteAnimationUtility.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{

    static void WriteSpriteAnimationClip(YAML::Emitter& out, const SpriteAnimationClip& clip,
                                       AssetHandle sharedAtlasTextureHandle)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << clip.Name;
        out << YAML::Key << "FrameRate" << YAML::Value << clip.FrameRate;
        out << YAML::Key << "LoopMode" << YAML::Value << SpriteAnimationLoopModeToString(clip.LoopMode);

        if (clip.UsesAtlasFrames(sharedAtlasTextureHandle))
        {
            out << YAML::Key << "AtlasFrameCoordinates" << YAML::Value << YAML::BeginSeq;
            for (const glm::ivec2& coordinates : clip.AtlasFrameCoordinates)
            {
                out << YAML::Flow << YAML::BeginSeq << coordinates.x << coordinates.y << YAML::EndSeq;
            }
            out << YAML::EndSeq;
        }

        out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
        for (const AssetHandle frameHandle : clip.Frames)
            out << (uint64_t)frameHandle;
        out << YAML::EndSeq;

        out << YAML::EndMap;
    }

    static void ReadSpriteAnimationClipAtlasCoordinates(const YAML::Node& clipNode,
                                                        SpriteAnimationClip& clip)
    {
        if (!clipNode["AtlasFrameCoordinates"])
            return;

        for (const YAML::Node coordinateNode : clipNode["AtlasFrameCoordinates"])
        {
            if (coordinateNode.IsSequence() && coordinateNode.size() >= 2)
                clip.AddAtlasFrame({coordinateNode[0].as<int>(), coordinateNode[1].as<int>()});
        }
    }

    static void ReadSpriteAnimationClipFrames(const YAML::Node& clipNode, SpriteAnimationClip& clip)
    {
        if (!clipNode["Frames"])
            return;

        for (const YAML::Node frameNode : clipNode["Frames"])
        {
            const uint64_t handle = frameNode.as<uint64_t>();
            if (handle != 0)
                clip.AddFrame(handle);
        }
    }

    void SpriteAnimationSerializer::Serialize(const std::filesystem::path &filepath,
                                              const Ref<SpriteAnimation> &animation)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "SpriteFrames";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)animation->Handle;

        if (animation->UsesAtlasTexture())
        {
            out << YAML::Key << "AtlasTextureHandle" << YAML::Value << (uint64_t)animation->GetAtlasTextureHandle();
            out << YAML::Key << "AtlasGridCellSize" << YAML::Value << animation->GetAtlasGridCellSize();
        }

        out << YAML::Key << "Animations" << YAML::Value << YAML::BeginSeq;
        for (const SpriteAnimationClip& clip : animation->GetNamedAnimations())
            WriteSpriteAnimationClip(out, clip, animation->GetAtlasTextureHandle());
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
        if (!data["AssetType"])
        {
            HIMII_CORE_ERROR("Invalid animation asset (missing AssetType): {0}", filepath.string());
            return nullptr;
        }

        const std::string assetType = data["AssetType"].as<std::string>();
        if (assetType != "SpriteFrames" && assetType != "SpriteAnimation")
        {
            HIMII_CORE_ERROR("Invalid animation asset type '{0}': {1}", assetType, filepath.string());
            return nullptr;
        }

        Ref<SpriteAnimation> animation = CreateRef<SpriteAnimation>();

        if (data["Handle"])
            animation->Handle = data["Handle"].as<uint64_t>();

        if (assetType == "SpriteFrames")
        {
            AssetHandle atlasTextureHandle = 0;
            uint32_t gridCellSize = 16;
            if (data["AtlasTextureHandle"])
                atlasTextureHandle = data["AtlasTextureHandle"].as<uint64_t>();
            if (data["AtlasGridCellSize"])
                gridCellSize = data["AtlasGridCellSize"].as<uint32_t>();
            if (atlasTextureHandle != 0)
                animation->SetAtlasTexture(atlasTextureHandle, gridCellSize);

            if (data["Animations"])
            {
                for (const YAML::Node clipNode : data["Animations"])
                {
                    SpriteAnimationClip clip;
                    if (clipNode["Name"])
                        clip.Name = clipNode["Name"].as<std::string>();
                    if (clip.Name.empty())
                        clip.Name = SpriteAnimationDefaultClipName;

                    if (clipNode["FrameRate"])
                        clip.FrameRate = clipNode["FrameRate"].as<float>();
                    if (clipNode["LoopMode"])
                        clip.LoopMode = SpriteAnimationLoopModeFromString(
                                clipNode["LoopMode"].as<std::string>().c_str());

                    ReadSpriteAnimationClipAtlasCoordinates(clipNode, clip);
                    ReadSpriteAnimationClipFrames(clipNode, clip);

                    animation->GetNamedAnimationsMutable().push_back(clip);
                }
            }

            if (animation->GetNamedAnimations().empty())
                animation->EnsureClip(SpriteAnimationDefaultClipName);

            return animation;
        }

        float defaultFrameRate = 10.0f;
        SpriteAnimationLoopMode loopMode = SpriteAnimationLoopMode::Loop;
        if (data["DefaultFrameRate"])
            defaultFrameRate = data["DefaultFrameRate"].as<float>();
        if (data["LoopMode"])
            loopMode = SpriteAnimationLoopModeFromString(data["LoopMode"].as<std::string>().c_str());

        AssetHandle atlasTextureHandle = 0;
        uint32_t gridCellSize = 16;
        std::vector<glm::ivec2> atlasCoordinates;
        std::vector<AssetHandle> frames;

        if (data["UsesAtlasFrames"] && data["UsesAtlasFrames"].as<bool>())
        {
            if (data["AtlasTextureHandle"])
                atlasTextureHandle = data["AtlasTextureHandle"].as<uint64_t>();
            if (data["AtlasGridCellSize"])
                gridCellSize = data["AtlasGridCellSize"].as<uint32_t>();

            if (data["AtlasFrameCoordinates"])
            {
                for (const YAML::Node coordinateNode : data["AtlasFrameCoordinates"])
                {
                    if (coordinateNode.IsSequence() && coordinateNode.size() >= 2)
                        atlasCoordinates.push_back(
                                {coordinateNode[0].as<int>(), coordinateNode[1].as<int>()});
                }
            }
        }
        else if (data["Frames"])
        {
            for (const YAML::Node frameNode : data["Frames"])
            {
                const uint64_t handle = frameNode.as<uint64_t>();
                if (handle != 0)
                    frames.push_back(handle);
            }
        }

        if (atlasTextureHandle != 0)
            animation->SetAtlasTexture(atlasTextureHandle, gridCellSize);

        animation->MigrateLegacySingleAnimation(defaultFrameRate, loopMode, frames, atlasCoordinates);
        return animation;
    }

} // namespace Himii

