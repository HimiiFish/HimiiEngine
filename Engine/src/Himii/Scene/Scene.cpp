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

Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name) {
    Entity entity(m_Registry.create(), this);
    // 先默认构造 ID，再赋值，避免依赖特定构造函数签名
    auto &id = entity.AddComponent<ID>();
    id.id = uuid;
    entity.AddComponent<TransformComponent>();
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
    // 脚本析构
    if (auto *nsc = m_Registry.try_get<NativeScriptComponent>(e)) {
        if (nsc->Instance) { nsc->Instance->OnDestroy(); }
        if (nsc->DestroyScript) { nsc->DestroyScript(nsc); }
    }
    m_Registry.destroy(e);
}

void Scene::OnUpdate(Timestep ts) {
    // 选择一个主摄像机或使用外部提供的 ViewProjection
    glm::mat4 viewProj(1.0f);
    if (m_UseExternalVP)
    {
        viewProj = m_ExternalVP;
    }
    //} else {
    //{
    //    entt::entity camEntity = entt::null;
    //    auto viewCam = m_Registry.view<TransformComponent, Himii::CameraComponent>();
    //    for (auto e : viewCam) { if (viewCam.get<Himii::CameraComponent>(e).primary) { camEntity = e; break; } }
    //    if (camEntity == entt::null && viewCam.begin() != viewCam.end()) camEntity = *viewCam.begin();
    //    if (camEntity != entt::null)
    //    {
    //        auto &tr = viewCam.get<Himii::TransformComponent>(camEntity);
    //        auto &cc = viewCam.get<Himii::CameraComponent>(camEntity);
    //        // 设置投影（含正交缩放）
    //        if (cc.projection == ProjectionType::Perspective) {
    //            cc.camera.SetFovYDeg(cc.fovYDeg);
    //        } else {
    //            cc.camera.SetOrthographicBySize(cc.orthoSize, cc.nearZ, cc.farZ);
    //        }

    //        // 设置视图：两种驱动方式
    //        if (cc.useLookAt)
    //        {
    //            cc.camera.SetPosition(tr.Position);
    //            // 将旋转与位置解耦，使用 lookAt 目标来构造朝向
    //            glm::mat4 V = glm::lookAt(tr.Position, cc.lookAtTarget, cc.up);
    //            viewProj = cc.camera.GetProjection() * V;
    //        }
    //        else
    //        {
    //            // 显式设置位置与欧拉角，不再因 SetPosition 改变旋转
    //            cc.camera.SetRotationEuler(tr.Rotation);
    //            cc.camera.SetPosition(tr.Position);
    //            viewProj = cc.camera.GetViewProjection();
    //        }
    //    }
    //} }

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
    auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
    for (auto entity : group) {
        auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
        Himii::Renderer2D::DrawSprite(transform, sprite, (int)entity);
    }

    // 原生脚本更新
    {
        auto viewScript = m_Registry.view<NativeScriptComponent>();
        for (auto e : viewScript) {
            auto &nsc = viewScript.get<NativeScriptComponent>(e);
            if (!nsc.Instance && nsc.InstantiateScript) {
                nsc.Instance = nsc.InstantiateScript();
                nsc.Instance->m_Entity = Entity(e, this);
                nsc.Instance->OnCreate();
            }
            if (nsc.Instance) nsc.Instance->OnUpdate(ts);
        }
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
