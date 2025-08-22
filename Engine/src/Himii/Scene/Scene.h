#pragma once
#include <entt/entt.hpp>
#include <string>
#include "Himii/Core/Timestep.h"

namespace Himii {
class Scene {
public:
    Scene() = default;

    entt::entity CreateEntity();
    entt::entity CreateEntity(const std::string& name);
    void DestroyEntity(entt::entity e);

    entt::registry& Registry() { return m_Registry; }

    void OnUpdate(Timestep ts);

private:
    entt::registry m_Registry;
};
} // namespace Himii
