#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Himii/Scene/Entity.h"
#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/KeyCodes.h"
#include <iostream>

namespace Himii {

    static bool IsSceneValid(Scene *scene, uint64_t entityID, Entity &outEntity)
    {
        if (!scene)
            return false;

        outEntity = scene->GetEntityByUUID(entityID);
        if (!outEntity)
        {
            return false;
        }
        return true;
    }

	static void NativeLog(char* message)
	{
		if (message)
            HIMII_INFO("{0}", message); 
	}

    static uint64_t Scene_CreatEntity(char* name)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->CreateEntity(name ? std::string(name) : "Unnamed");
        return entity.GetUUID();
    }

    static void Scene_DestroyEntity(uint64_t entityID)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity)
            scene->DestroyEntity(entity);
    }

    static uint64_t Scene_FindEntityByName(char *name)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene || !name)
            return 0;

        Entity entity = scene->FindEntityByName(std::string(name)); // 需要在 Scene 类中实现此方法
        return entity ? entity.GetUUID() : 0;
    }

	static bool Entity_HasComponent(uint64_t entityID, void* scenePtr)
	{
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return false;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return false;

        // TODO: 实现 Managed Type 到 Native Component 的映射系统
        return true; 
	}

	static void Transform_GetTranslation(uint64_t entityID, glm::vec3 *outTranslation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            *outTranslation = glm::vec3(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity) // <--- 关键修复：检查实体是否有效
        {
            *outTranslation = glm::vec3(0.0f);
            return;
        }

        *outTranslation = entity.GetComponent<TransformComponent>().Position;
    }

	// 设置位置
    static void Transform_SetTranslation(uint64_t entityID, glm::vec3 *translation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return; // <--- 关键修复

        entity.GetComponent<TransformComponent>().Position = *translation;
    }

    static void Transform_GetRotation(uint64_t entityID, glm::vec3 *outRotation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            *outRotation = glm::vec3(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
        {
            *outRotation = glm::vec3(0.0f);
            return;
        }
        *outRotation = entity.GetComponent<TransformComponent>().Rotation;
    }

    static void Transform_SetRotation(uint64_t entityID, glm::vec3 *rotation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return;

        entity.GetComponent<TransformComponent>().Rotation = *rotation;
    }

    static void Transform_GetScale(uint64_t entityID, glm::vec3 *outScale)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            return;
        } // Scale 默认是 1

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
        {
            return;
        }
        *outScale = entity.GetComponent<TransformComponent>().Scale;
    }

    static void Transform_SetScale(uint64_t entityID, glm::vec3 *scale)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return;
        entity.GetComponent<TransformComponent>().Scale = *scale;
    }

    static bool Input_IsKeyDown(int keycode)
    {
        // 调用引擎底层的 Input 静态类
        return Input::IsKeyPressed(keycode);
    }

    static bool Input_IsMouseButtonDown(int button)
    {
        return Input::IsMouseButtonPressed(button);
    }

    static void Input_GetMousePosition(glm::vec2 *outPos)
    {
        auto pos = Input::GetMousePosition(); // 假设返回 std::pair 或 glm::vec2
        outPos->x = pos.x;
        outPos->y = pos.y;
    }

    static void Rigidbody2D_ApplyLinearImpulse(uint64_t entityID, glm::vec2 *impulse, glm::vec2 *point, bool wake)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return;

        if (entity.HasComponent<Rigidbody2DComponent>())
        {
            auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();
            b2BodyId bodyId = b2BodyId{(int32_t)(uintptr_t)rb2d.RuntimeBody}; // 这里的转换取决于你的 ToBodyId 实现
            if (b2Body_IsValid(bodyId))
            {
                b2Body_ApplyLinearImpulse(bodyId, {impulse->x, impulse->y}, {point->x, point->y}, wake);
            }
        }
    }

    static void Rigidbody2D_ApplyLinearImpulseToCenter(uint64_t entityID, glm::vec2 *impulse, bool wake)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return;

        if (entity.HasComponent<Rigidbody2DComponent>())
        {
            auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();
            // 同样的获取 Body 逻辑...
            b2BodyId bodyId = *(b2BodyId *)&rb2d.RuntimeBody; // 简化转换
            if (b2Body_IsValid(bodyId))
            {
                b2Body_ApplyLinearImpulseToCenter(bodyId, {impulse->x, impulse->y}, wake);
            }
        }
    }

    static void Rigidbody2D_GetLinearVelocity(uint64_t entityID, glm::vec2 *outVelocity)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();
        b2BodyId bodyId = *(b2BodyId *)&rb2d.RuntimeBody;
        if (b2Body_IsValid(bodyId))
        {
            b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
            outVelocity->x = vel.x;
            outVelocity->y = vel.y;
        }
    }

    static void Rigidbody2D_SetLinearVelocity(uint64_t entityID, glm::vec2 *velocity)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();
        b2BodyId bodyId = *(b2BodyId *)&rb2d.RuntimeBody;
        if (b2Body_IsValid(bodyId))
        {
            b2Body_SetLinearVelocity(bodyId, {velocity->x, velocity->y});
        }
    }

    ScriptEngineData ScriptGlue::GetNativeFunctions()
    {
        ScriptEngineData data;

        // Core
        data.LogFunc = (void *)&NativeLog;

        // Entity/Scene
        data.Entity_HasComponent = (void *)&Entity_HasComponent;
        data.Scene_CreateEntity = (void *)&Scene_CreatEntity;
        data.Scene_DestroyEntity = (void *)&Scene_DestroyEntity;
        data.Scene_FindEntityByName = (void *)&Scene_FindEntityByName;

        // Transform
        data.Transform_GetTranslation = (void *)&Transform_GetTranslation;
        data.Transform_SetTranslation = (void *)&Transform_SetTranslation;
        data.Transform_GetRotation = (void *)&Transform_GetRotation;
        data.Transform_SetRotation = (void *)&Transform_SetRotation;
        data.Transform_GetScale = (void *)&Transform_GetScale;
        data.Transform_SetScale = (void *)&Transform_SetScale;

        // Input
        data.Input_IsKeyDown = (void *)&Input_IsKeyDown;
        data.Input_IsMouseButtonDown = (void *)&Input_IsMouseButtonDown;
        data.Input_GetMousePosition = (void *)&Input_GetMousePosition;

        // Rigidbody2D
        data.Rigidbody2D_ApplyLinearImpulse = (void *)&Rigidbody2D_ApplyLinearImpulse;
        data.Rigidbody2D_ApplyLinearImpulseToCenter = (void *)&Rigidbody2D_ApplyLinearImpulseToCenter;
        data.Rigidbody2D_GetLinearVelocity = (void *)&Rigidbody2D_GetLinearVelocity;
        data.Rigidbody2D_SetLinearVelocity = (void *)&Rigidbody2D_SetLinearVelocity;

        return data;
    }
}