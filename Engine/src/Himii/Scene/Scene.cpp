#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/Renderer.h"
#include "Himii/Scene/Entity.h"
#include "glad/glad.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Himii {

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name) 
{
    Entity entity(m_Registry.create(), this);

    entity.AddComponent<ID>(uuid);
    entity.AddComponent<TransformComponent>();
    auto &tag = entity.AddComponent<TagComponent>();
    tag.name = name.empty() ? "Entity" : name;

    m_EntityMap[uuid] = entity;

    return entity;
}

Entity Scene::CreateEntity(const std::string& name) 
{
    return CreateEntityWithUUID(UUID(), name);
}

void Scene::DestroyEntity(entt::entity e) {
    if (!m_Registry.valid(e)) return;
    // 尝试移除 UUID -> entt::entity 映射，防止悬挂引用
    if (auto *pid = m_Registry.try_get<ID>(e)) {
        auto it = m_EntityMap.find(pid->id);
        if (it != m_EntityMap.end()) m_EntityMap.erase(it);
    }
    // 脚本析构
    if (auto *nsc = m_Registry.try_get<NativeScriptComponent>(e)) {
        if (nsc->Instance) { nsc->Instance->OnDestroy(); }
        if (nsc->DestroyScript) { nsc->DestroyScript(nsc); }
    }
    m_Registry.destroy(e);
}

void Scene::OnUpdate(Timestep ts) {
    
    Camera *mainCamera = nullptr;
    glm::mat4 cameraTransform{1.0f};
    {
        auto group = m_Registry.group<TransformComponent, CameraComponent>();
        for (auto entity : group)
        {
            auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);
            if (camera.primary)
            {
                mainCamera = &camera.camera;
                cameraTransform = transform.GetTransform();
            }
        }
    }

    if (mainCamera)
    {
        Renderer2D::BeginScene(*mainCamera, cameraTransform);
        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
        for (auto entity: group)
        {
            auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
            Himii::Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
        }
        Renderer2D::EndScene();
    }

    // 如果没有可用摄像机，保留上层（例如 CubeLayer）外部调用 BeginScene 的能力

    // 3D MeshRenderer - Skybox pass first (depth: LEQUAL, no depth write)
    /*{
        auto skyView = m_Registry.view<Himii::Transform, Himii::MeshRenderer, Himii::SkyboxTag>();
    if (skyView.begin() != skyView.end()) {
            if (viewProj != glm::mat4(1.0f)) Himii::Renderer::BeginScene(viewProj);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);
            for (auto e : skyView) {
                auto &tr = skyView.get<Himii::Transform>(e);
                const auto &mr = skyView.get<Himii::MeshRenderer>(e);
                if (mr.vertexArray && mr.shader) {
                    if (mr.texture) mr.texture->Bind(0);
                    Himii::Renderer::Submit(mr.shader, mr.vertexArray, tr.GetTransform());
                }
            }
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
            if (viewProj != glm::mat4(1.0f)) Himii::Renderer::EndScene();
        }
    }*/

    // 3D MeshRenderer - regular pass (exclude skybox)
   /* {
        auto meshView = m_Registry.view<Himii::Transform, Himii::MeshRenderer>(entt::exclude<Himii::SkyboxTag>);
        if (meshView.begin() != meshView.end() && viewProj != glm::mat4(1.0f)) Himii::Renderer::BeginScene(viewProj);
        for (auto e : meshView) {
            auto &tr = meshView.get<Himii::Transform>(e);
            const auto &mr = meshView.get<Himii::MeshRenderer>(e);
            if (mr.vertexArray && mr.shader) {
                if (mr.texture) mr.texture->Bind(0);
                Himii::Renderer::Submit(mr.shader, mr.vertexArray, tr.GetTransform());
            }
        }
        if (meshView.begin() != meshView.end() && viewProj != glm::mat4(1.0f)) Himii::Renderer::EndScene();
    }*/

    // 2D SpriteRenderer 实体：Renderer2D 批渲染
    

    // 原生脚本更新
    {
        m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto &nsc) {
            // 如果没有实例化，尝试实例化
            if (!nsc.Instance) {
                    nsc.Instance = nsc.InstantiateScript();
                    nsc.Instance->m_Entity = Entity{entity, this};
                    nsc.Instance->OnCreate();
            }
            // 更新
            if (nsc.Instance) {
                nsc.Instance->OnUpdate(ts);
            }
        });
    }
}

void Scene::Clear() {
    // 收集后再销毁，避免遍历时失效
    std::vector<entt::entity> toDelete;
    auto view = m_Registry.view<TransformComponent>();
    for (auto e : view) toDelete.push_back(e);
    for (auto e : toDelete) DestroyEntity(e);
}

} // namespace Himii
