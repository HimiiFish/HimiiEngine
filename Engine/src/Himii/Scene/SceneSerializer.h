#pragma once
#include "Himii/Scene/Scene.h"

namespace Himii {

// 简单的 YAML 场景序列化/反序列化器
class SceneSerializer {
public:
    SceneSerializer(const Ref<Scene> &scene);

    void Serialize(const std::string &filepath);
    void SerializeRuntime(const std::string &filepath);

    bool Deserialize(const std::string &filepath);
    bool DeserializeRuntime(const std::string &filepath);

private:
    Ref<Scene> m_Scene{};
};
}
