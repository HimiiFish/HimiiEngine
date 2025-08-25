#pragma once
#include <string>
namespace Himii {
class Scene;

// 简单的 YAML 场景序列化/反序列化器
class SceneSerializer {
public:
    explicit SceneSerializer(Scene* scene) : m_Scene(scene) {}

    // 保存到 .yaml 文件
    bool Serialize(const std::string& filepath) const;
    // 从 .yaml 文件读取
    bool Deserialize(const std::string& filepath);

private:
    Scene* m_Scene{};
};
}
