#pragma once

#include "Himii/Core/UUID.h"
#include "Scene.h"
#include "Himii/Scene/Components.h"

#include "entt/entt.hpp"

namespace Himii
{

class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene *scene);
    Entity(const Entity &other) = default;

    // Add a component of type T to this entity and return a reference to it
    template<typename T, typename... Args>
    T &AddComponent(Args &&...args)
    {
        return m_Scene->Registry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template<typename T>
    bool HasComponent() const
    {
        return m_Scene->Registry().all_of<T>(m_EntityHandle);
    }

    template<typename T>
    T &GetComponent()
    {
        return m_Scene->Registry().get<T>(m_EntityHandle);
    }

    template<typename T>
    void RemoveComponent()
    {
        m_Scene->Registry().remove<T>(m_EntityHandle);
    }

    operator bool() const
    {
        return m_EntityHandle != entt::null;
    }

    operator uint32_t() const
    {
        return static_cast<uint32_t>(m_EntityHandle);
    }

    operator entt::entity() const
    {
        return m_EntityHandle;
    }

    UUID GetUUID()
    {
        return GetComponent<IDComponent>().id;
    }

    const std::string &GetName()
    {
        return GetComponent<TagComponent>().name;
    }

    bool operator==(const Entity &other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }

    bool operator!=(const Entity &other) const
    {
        return !(*this == other);
    }

private:
    entt::entity m_EntityHandle{entt::null};
    Scene *m_Scene{};
};

} // namespace Himii
  