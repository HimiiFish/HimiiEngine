#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Scene/Entity.h"
#include <entt/entt.hpp>
#include <string>

namespace Himii {

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name) {
    Entity entity(m_Registry.create(), this);
    // 先默认构造 ID，再赋值，避免依赖特定构造函数签名
    auto &id = entity.AddComponent<ID>();
    id.id = uuid;
    entity.AddComponent<Transform>();
    auto &tag = entity.AddComponent<Tag>();
    tag.name = name.empty() ? "Entity" : name;

    m_EntityMap[uuid] = entity.Raw();

    return entity;
}

Entity Scene::CreateEntity(const std::string& name) {
    return CreateEntityWithUUID(UUID(), name);
}

void Scene::DestroyEntity(entt::entity e) {
    if (!m_Registry.valid(e)) return;
    // 尝试移除 UUID -> entt::entity 映射，防止悬挂引用
    if (auto *pid = m_Registry.try_get<ID>(e)) {
        auto it = m_EntityMap.find(pid->id);
        if (it != m_EntityMap.end()) m_EntityMap.erase(it);
    }
    m_Registry.destroy(e);
}

void Scene::OnUpdate(Timestep /*ts*/) {
    // Transform + SpriteRenderer
    auto group = m_Registry.group<Himii::Transform>(entt::get<Himii::SpriteRenderer>);
    for (auto entity : group) {
        auto &tr = group.get<Himii::Transform>(entity);
        if (auto *sr = m_Registry.try_get<Himii::SpriteRenderer>(entity)) {
            const glm::mat4 transform = tr.GetTransform();
            if (sr->texture) {
                // 如果有自定义UV，则用图集分片绘制，否则标准全贴图
                if (sr->uvs[0] != glm::vec2(0.0f) || sr->uvs[1] != glm::vec2(1.0f, 0.0f)
                    || sr->uvs[2] != glm::vec2(1.0f, 1.0f) || sr->uvs[3] != glm::vec2(0.0f, 1.0f)) {
                    Himii::Renderer2D::DrawQuadUV(transform, sr->texture, sr->uvs, sr->tiling, sr->color);
                } else {
                    Himii::Renderer2D::DrawQuad(transform, sr->texture, sr->tiling, sr->color);
                }
            } else {
                Himii::Renderer2D::DrawQuad(transform, sr->color);
            }
        }
    }
}

} // namespace Himii
