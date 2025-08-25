#pragma once
#include <entt/entt.hpp>
#include "Himii/Core/UUID.h"
#include <string>
#include "Himii/Core/Timestep.h"
#include <unordered_map>
#include <glm/glm.hpp>
namespace Himii { class Entity; }

namespace Himii {
class Scene {
public:
    Scene() = default;

    Entity CreateEntityWithUUID(UUID uuid, const std::string& name);
    Entity CreateEntity(const std::string& name);
    void DestroyEntity(entt::entity e);

    entt::registry& Registry() { return m_Registry; }

    void OnUpdate(Timestep ts);

    // 供编辑器渲染路径设置：使用外部提供的 ViewProjection 矩阵进行一次渲染
    // 传入 nullptr 关闭外部相机覆盖
    void SetExternalViewProjection(const glm::mat4* vp) {
        if (vp) { m_UseExternalVP = true; m_ExternalVP = *vp; }
        else { m_UseExternalVP = false; }
    }

    // 清空场景：安全销毁所有实体（走 DestroyEntity，确保脚本生命周期与映射清理）
    void Clear();

private:
    entt::registry m_Registry;
    // 只存句柄，避免头文件循环和不完整类型
    std::unordered_map<UUID, entt::entity> m_EntityMap; // 用于快速查找实体
    // 外部相机覆盖
    bool m_UseExternalVP{false};
    glm::mat4 m_ExternalVP{1.0f};
};
} // namespace Himii
