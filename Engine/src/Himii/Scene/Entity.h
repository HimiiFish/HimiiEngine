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
        Entity(const Entity &other) = default;

        /// <summary>
        /// 添加组件，完美转发参数
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <typeparam name="...Args"></typeparam>
        /// <param name="...args"></param>
        /// <returns></returns>
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

        /// <summary>
        /// 检测是否含有组件
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        template<typename T>
        bool HasComponent() const
        {
            return m_Scene->Registry().all_of<T>(m_EntityHandle);
        }

        /// <summary>
        /// 获取组件引用
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
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

        /// <summary>
        /// 移除组件
        /// </summary>
        /// <typeparam name="T"></typeparam>
        template<typename T>
        void RemoveComponent()
        {
            m_Scene->Registry().remove<T>(m_EntityHandle);
        }
        entt::entity Raw() const
        {
            return m_EntityHandle;
        }

        operator bool() const
        {
            return m_EntityHandle != entt::null;
        }

        operator uint32_t() const
        {
            return (uint32_t)m_EntityHandle;
        }

        operator entt::entity() const
        {
            return m_EntityHandle;
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
