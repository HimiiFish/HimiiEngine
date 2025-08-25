#include "Himii/Scene/SceneSerializer.h"
#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include "Himii/Scene/Entity.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML {
template<>
struct convert<glm::vec3> {
    static Node encode(const glm::vec3& v) {
        Node n; n.push_back(v.x); n.push_back(v.y); n.push_back(v.z); return n;
    }
    static bool decode(const Node& node, glm::vec3& v) {
        if (!node.IsSequence() || node.size()!=3) return false; v.x=node[0].as<float>(); v.y=node[1].as<float>(); v.z=node[2].as<float>(); return true;
    }
};
template<>
struct convert<glm::vec4> {
    static Node encode(const glm::vec4& v) {
        Node n; n.push_back(v.x); n.push_back(v.y); n.push_back(v.z); n.push_back(v.w); return n;
    }
    static bool decode(const Node& node, glm::vec4& v) {
        if (!node.IsSequence() || node.size()!=4) return false; v.x=node[0].as<float>(); v.y=node[1].as<float>(); v.z=node[2].as<float>(); v.w=node[3].as<float>(); return true;
    }
};
}

namespace YAML {
// Emitter helpers for GLM types (ADL-friendly)
inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}
inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v) {
    out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}
inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v) {
    out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}
} // namespace YAML

namespace Himii {

bool SceneSerializer::Serialize(const std::string& filepath) const {
    if (!m_Scene) return false;
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    auto& reg = m_Scene->Registry();
    // 仅迭代拥有 Transform 的实体（保证最小有效实体）
    auto view = reg.view<Transform>();
    for (auto e : view) {
        out << YAML::BeginMap; // Entity
        // UUID（如果有）
        if (auto* id = reg.try_get<ID>(e)) out << YAML::Key << "ID" << YAML::Value << (uint64_t)id->id;
        if (auto* tag = reg.try_get<Tag>(e)) out << YAML::Key << "Tag" << YAML::Value << tag->name;

        // Transform
        if (auto* t = reg.try_get<Transform>(e)) {
            out << YAML::Key << "Transform" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Position" << YAML::Value << t->Position;
            out << YAML::Key << "Rotation" << YAML::Value << t->Rotation;
            out << YAML::Key << "Scale" << YAML::Value << t->Scale;
            out << YAML::EndMap;
        }

        // SpriteRenderer
        if (auto* sr = reg.try_get<SpriteRenderer>(e)) {
            out << YAML::Key << "SpriteRenderer" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Color" << YAML::Value << sr->color;
            out << YAML::Key << "Tiling" << YAML::Value << sr->tiling;
            out << YAML::EndMap;
        }

        // Camera
        if (auto* cc = reg.try_get<CameraComponent>(e)) {
            out << YAML::Key << "Camera" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Primary" << YAML::Value << cc->primary;
            out << YAML::Key << "Projection" << YAML::Value << (cc->projection == ProjectionType::Perspective ? "Perspective" : "Orthographic");
            out << YAML::Key << "FovYDeg" << YAML::Value << cc->fovYDeg;
            out << YAML::Key << "OrthoSize" << YAML::Value << cc->orthoSize;
            out << YAML::Key << "NearZ" << YAML::Value << cc->nearZ;
            out << YAML::Key << "FarZ" << YAML::Value << cc->farZ;
            out << YAML::Key << "UseLookAt" << YAML::Value << cc->useLookAt;
            out << YAML::Key << "LookAtTarget" << YAML::Value << cc->lookAtTarget;
            out << YAML::Key << "Up" << YAML::Value << cc->up;
            out << YAML::EndMap;
        }

        out << YAML::EndMap; // Entity
    }

    out << YAML::EndSeq; // Entities
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    if (!fout.is_open()) return false;
    fout << out.c_str();
    return true;
}

bool SceneSerializer::Deserialize(const std::string& filepath) {
    if (!m_Scene) return false;
    YAML::Node data;
    try { data = YAML::LoadFile(filepath); }
    catch (...) { return false; }
    if (!data["Entities"]) return false;

    for (auto entNode : data["Entities"]) {
        std::string name = entNode["Tag"] ? entNode["Tag"].as<std::string>() : "Entity";
        // 如果存在 ID，则使用稳定 UUID 创建实体（对齐 Hazel 的做法）
        Entity e = [&]() -> Entity {
            if (auto idNode = entNode["ID"]) {
                uint64_t id64 = idNode.as<uint64_t>();
                return m_Scene->CreateEntityWithUUID(UUID{id64}, name);
            }
            return m_Scene->CreateEntity(name);
        }();

        if (auto tNode = entNode["Transform"]) {
            auto& t = e.GetComponent<Transform>();
            t.Position = tNode["Position"].as<glm::vec3>(t.Position);
            t.Rotation = tNode["Rotation"].as<glm::vec3>(t.Rotation);
            t.Scale    = tNode["Scale"].as<glm::vec3>(t.Scale);
        }
        if (auto srNode = entNode["SpriteRenderer"]) {
            if (!e.HasComponent<SpriteRenderer>()) e.AddComponent<SpriteRenderer>();
            auto& sr = e.GetComponent<SpriteRenderer>();
            sr.color  = srNode["Color"].as<glm::vec4>(sr.color);
            sr.tiling = srNode["Tiling"].as<float>(sr.tiling);
        }
        if (auto camNode = entNode["Camera"]) {
            if (!e.HasComponent<CameraComponent>()) e.AddComponent<CameraComponent>();
            auto& cc = e.GetComponent<CameraComponent>();
            cc.primary     = camNode["Primary"].as<bool>(cc.primary);
            std::string projStr = camNode["Projection"].as<std::string>("Perspective");
            cc.projection  = (projStr=="Orthographic") ? ProjectionType::Orthographic : ProjectionType::Perspective;
            cc.fovYDeg     = camNode["FovYDeg"].as<float>(cc.fovYDeg);
            cc.orthoSize   = camNode["OrthoSize"].as<float>(cc.orthoSize);
            cc.nearZ       = camNode["NearZ"].as<float>(cc.nearZ);
            cc.farZ        = camNode["FarZ"].as<float>(cc.farZ);
            cc.useLookAt   = camNode["UseLookAt"].as<bool>(cc.useLookAt);
            cc.lookAtTarget= camNode["LookAtTarget"].as<glm::vec3>(cc.lookAtTarget);
            cc.up          = camNode["Up"].as<glm::vec3>(cc.up);
        }
    }
    return true;
}

} // namespace Himii
