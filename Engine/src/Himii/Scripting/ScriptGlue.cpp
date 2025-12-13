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
        {
            // 如果这里触发，说明 OnRuntimeStart 没有正确设置 s_SceneContext
            // std::cout << "[ScriptGlue] Error: No Active Scene Context!" << std::endl;
            return false;
        }

        outEntity = scene->GetEntityByUUID(entityID);
        if (!outEntity)
        {
            // 实体可能已被销毁，但 C# 还在尝试更新它
            // std::cout << "[ScriptGlue] Error: Entity not found " << entityID << std::endl;
            return false;
        }
        return true;
    }

	static void NativeLog(char* message)
	{
		// 暂时使用 cout，后面可以换成 spdlog
		if (message)
			std::cout << "[ScriptCore] " << message << std::endl;
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
        // 获取当前场景的 Entity
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);

        *outTranslation = entity.GetComponent<TransformComponent>().Position;
    }

	// 设置位置
    static void Transform_SetTranslation(uint64_t entityID, glm::vec3 *translation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);

        entity.GetComponent<TransformComponent>().Position = *translation;
    }

    static bool Input_IsKeyDown(int keycode)
    {
        // 调用引擎底层的 Input 静态类
        return Input::IsKeyPressed(keycode);
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
            // 这里需要你的 ToBodyId 辅助函数，参考 Scene.cpp
            // 假设你已经能获取到 b2BodyId
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


	void ScriptGlue::RegisterFunctions()
	{
        /*data->LogFunc = (void *)&NativeLog;
        data->Transform_GetTranslation = (void *)&Transform_GetTranslation;
        data->Transform_SetTranslation = (void *)&Transform_SetTranslation;*/
	}

    ScriptEngineData ScriptGlue::GetNativeFunctions()
    {
        ScriptEngineData data;
        data.LogFunc = (void *)&NativeLog;
        data.Entity_HasComponent = (void *)&Entity_HasComponent;

        data.Transform_GetTranslation = (void *)&Transform_GetTranslation;
        data.Transform_SetTranslation = (void *)&Transform_SetTranslation;

        data.Input_IsKeyDown = (void *)&Input_IsKeyDown;

        data.Rigidbody2D_ApplyLinearImpulse = (void *)&Rigidbody2D_ApplyLinearImpulse;
        data.Rigidbody2D_ApplyLinearImpulseToCenter = (void *)&Rigidbody2D_ApplyLinearImpulseToCenter;

        return data;
    }
}