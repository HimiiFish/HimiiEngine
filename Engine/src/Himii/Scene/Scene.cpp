#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include "Himii/Renderer/Renderer2D.h"
#include <entt/entt.hpp>
#include <string>

namespace Himii {

entt::entity Scene::CreateEntity() {
    auto e = m_Registry.create();
    // 默认附加 Transform
    m_Registry.emplace<ID>(e, ID{ UUID{} });
    m_Registry.emplace<Tag>(e, Tag{ "Entity" });
    m_Registry.emplace<Himii::Transform>(e);
    return e;
}

entt::entity Scene::CreateEntity(const std::string& name) {
    auto e = CreateEntity();
    // 覆盖默认名称
    if (auto* tag = m_Registry.try_get<Tag>(e))
        tag->name = name;
    return e;
}

void Scene::DestroyEntity(entt::entity e) {
    if (m_Registry.valid(e)) m_Registry.destroy(e);
}

void Scene::OnUpdate(Timestep /*ts*/) {
    // 示例：如果有 Transform + SpriteRenderer，就提交一个彩色方块
    auto group = m_Registry.group<Himii::Transform>(entt::get<Himii::SpriteRenderer>);
    for (auto entity : group) {
        auto &tr = group.get<Himii::Transform>(entity);
        if (auto *sr = m_Registry.try_get<Himii::SpriteRenderer>(entity)) {
            // 使用 Transform 矩阵应用 Position/Rotation/Scale
            const glm::mat4 transform = tr.GetTransform();
            Himii::Renderer2D::DrawQuad(transform, sr->color);
        }
    }
}

} // namespace Himii
