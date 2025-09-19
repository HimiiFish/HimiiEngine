#pragma once
#include <entt/entt.hpp>
#include <utility>
#include <type_traits>
#include "Himii/Scene/Scene.h"

namespace Himii
{

    class Entity {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene *scene);

        // Convenience aliases with correct spelling
        template<typename T, typename... Args>
        decltype(auto) AddComponent(Args &&...args)
        {
            if constexpr (std::is_empty_v<T>) {
                m_Scene->Registry().emplace<T>(m_EntityHandle);
                return; // deduce void
            } else {
                return m_Scene->Registry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            }
        }
        template<typename T>
        bool HasComponent() const
        {
            return m_Scene->Registry().all_of<T>(m_EntityHandle);
        }
        template<typename T>
        decltype(auto) GetComponent()
        {
            if constexpr (std::is_empty_v<T>) {
                // no-op for empty components; use HasComponent<T>() instead
                return; // deduce void
            } else {
                return m_Scene->Registry().get<T>(m_EntityHandle);
            }
        }
        template<typename T>
        void RemoveComponent()
        {
            m_Scene->Registry().remove<T>(m_EntityHandle);
        }

        bool Valid() const
        {
            if (!m_Scene)
                return false;
            return m_Scene->Registry().valid(m_EntityHandle);
        }
        entt::entity Raw() const
        {
            return m_EntityHandle;
        }
        explicit operator bool() const
        {
            return Valid();
        }

    private:
        entt::entity m_EntityHandle{entt::null};
        Scene *m_Scene{};
    };
} // namespace Himii
