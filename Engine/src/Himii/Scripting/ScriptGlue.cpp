#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "Himii/Scene/Entity.h"
#include "Himii/Scene/Scene.h"
#include "Himii/Scene/Components.h"
#include <iostream>

namespace Himii {

	static void NativeLog(char* message)
	{
		// 暂时使用 cout，后面可以换成 spdlog
		if (message)
			std::cout << "[ScriptCore] " << message << std::endl;
	}

	static bool Entity_HasComponent(uint64_t entityID, void* scenePtr)
	{
        return true;
	}

	static void Transform_GetTranslation(uint64_t entityID, glm::vec3 *outTranslation)
    {
        // 获取当前场景的 Entity
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity)
        {
            auto &pos = entity.GetComponent<TransformComponent>().Position;
            outTranslation->x = pos.x;
            outTranslation->y = pos.y;
            outTranslation->z = pos.z;
        }
    }

	// 设置位置
    static void Transform_SetTranslation(uint64_t entityID, glm::vec3 *translation)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity)
        {
            auto &pos = entity.GetComponent<TransformComponent>().Position;
            pos.x = translation->x;
            pos.y = translation->y;
            pos.z = translation->z;
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
        return data;
    }
}