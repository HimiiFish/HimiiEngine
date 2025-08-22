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

        template<typename T, typename... Args>
        decltype(auto) AddCompnent(Args &&...args)
        {
            if constexpr (std::is_empty_v<T>) {
                m_Scene->Registry().emplace<T>(m_Handle);
                return; // deduce void
            } else {
                return m_Scene->Registry().emplace<T>(m_Handle, std::forward<Args>(args)...);
            }
        }
        // Convenience aliases with correct spelling
        template<typename T, typename... Args>
        decltype(auto) AddComponent(Args &&...args)
        {
            return AddCompnent<T>(std::forward<Args>(args)...);
        }
        template<typename T>
        bool HasCompnent() const
        {
            return m_Scene->Registry().all_of<T>(m_Handle);
        }
        template<typename T>
        bool HasComponent() const
        {
            return HasCompnent<T>();
        }
        template<typename T>
        decltype(auto) GetCompnent()
        {
            if constexpr (std::is_empty_v<T>) {
                // no-op for empty components; use HasComponent<T>() instead
                return; // deduce void
            } else {
                return m_Scene->Registry().get<T>(m_Handle);
            }
        }
        template<typename T>
        decltype(auto) GetComponent()
        {
            return GetCompnent<T>();
        }
        template<typename T>
        void RemoveCompnent()
        {
            m_Scene->Registry().remove<T>(m_Handle);
        }
        template<typename T>
        void RemoveComponent()
        {
            RemoveCompnent<T>();
        }

        bool Valid() const
        {
            if (!m_Scene)
                return false;
            return m_Scene->Registry().valid(m_Handle);
        }
        entt::entity Raw() const
        {
            return m_Handle;
        }
        explicit operator bool() const
        {
            return Valid();
        }

    private:
        entt::entity m_Handle{entt::null};
        Scene *m_Scene{};
    };
} // namespace Himii
