#pragma once
#include <entt/entt.hpp>
#include "Himii/Core/UUID.h"
#include <string>
#include "Himii/Core/Timestep.h"
#include <unordered_map>
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

private:
    entt::registry m_Registry;
    // 只存句柄，避免头文件循环和不完整类型
    std::unordered_map<UUID, entt::entity> m_EntityMap; // 用于快速查找实体
};
} // namespace Himii
