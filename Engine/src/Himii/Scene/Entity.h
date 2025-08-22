#pragma once
#include <entt/entt.hpp>
#include "Himii/Scene/Scene.h"

namespace Himii {

class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);

    template<typename T, typename... Args>
    T& AddCompnent(Args&&... args) {
        return m_Scene->Registry().emplace<T>(m_Handle, std::forward<Args>(args)...);
    }
    template<typename T>
    bool HasCompnent() const {
        return m_Scene->Registry().all_of<T>(m_Handle);
    }
    template<typename T>
    T& GetCompnent() {
        return m_Scene->Registry().get<T>(m_Handle);
    }
    template<typename T>
    void RemoveCompnent() {
        m_Scene->Registry().remove<T>(m_Handle);
    }

    bool Valid() const {
        if (!m_Scene) return false;
        return m_Scene->Registry().valid(m_Handle);
    }
    entt::entity Raw() const { return m_Handle; }
    explicit operator bool() const { return Valid(); }

private:
    entt::entity m_Handle{entt::null};
    Scene* m_Scene{};
};

} // namespace Himii::ECS
