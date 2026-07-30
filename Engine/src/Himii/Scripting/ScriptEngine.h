#pragma once

#include <cstring>
#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include "glm/glm.hpp"

#include "Himii/Core/Timestep.h"
#include "Himii/Core/UUID.h"

namespace Himii {

    class Scene;
    class Entity;

    enum class ScriptFieldType
    {
        None = 0,
        Float,
        Double,
        Bool,
        Char,
        Byte,
        Short,
        Int,
        Long,
        UByte,
        UShort,
        UInt,
        ULong,
        Vector2,
        Vector3,
        Vector4,
        Entity,
        String,
        KeyCode
    };

    enum class ScriptInstanceFlags : int
    {
        None = 0,
        InvokeOnCreate = 1
    };

    struct ScriptFieldInstance
    {
        ScriptFieldType Type = ScriptFieldType::None;

        uint8_t m_Buffer[16];

        std::string StringValue;

        template<typename T>
        T GetValue() const
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            return *(const T*)m_Buffer;
        }

        template<typename T>
        void SetValue(T value)
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            std::memcpy(m_Buffer, &value, sizeof(T));
        }
    };

    template<>
    inline std::string ScriptFieldInstance::GetValue<std::string>() const
    {
        return StringValue;
    }

    template<>
    inline void ScriptFieldInstance::SetValue<std::string>(std::string value)
    {
        StringValue = std::move(value);
    }

    using ScriptFieldMap = std::unordered_map<std::string, ScriptFieldInstance>;

    /// 与 C# Collision2DInfo 布局一致，供脚本碰撞回调使用。
    struct Collision2DInterop
    {
        uint64_t OtherEntityID = 0;
        glm::vec2 Normal{0.0f, 0.0f};
        glm::vec2 Point{0.0f, 0.0f};
    };

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& filepath);
        static bool RequestCompileAndReload(const std::filesystem::path &projectPath);
        static std::filesystem::path GetGameAssemblyDllPath();
		static bool LoadAppAssembly(const std::filesystem::path& filepath);

		static void OnCreateEntity(Entity entity);
        static void OnRuntimeStart(Scene* scene);
        static void OnRuntimeStop();

		static void OnUpdateScript(Entity entity, Timestep ts);
        static void OnFixedUpdateScript(Entity entity, Timestep ts);
        static float GetScriptDeltaTime();

		static bool EntityClassExists(const std::string &fullClassName);

        static void* InstantiateClass(UUID entityID, const std::string& className,
                                      ScriptInstanceFlags flags = ScriptInstanceFlags::InvokeOnCreate);
        
        static ScriptFieldMap& GetScriptFieldMap(Entity entity);

        static std::string GetFields(void *instanceHandle);
        static bool GetFloat(void *instanceHandle, const std::string &fieldName, float &outValue);
        static void SetFloat(void *instanceHandle, const std::string &fieldName, float value);

        static bool GetInt(void *instanceHandle, const std::string &fieldName, int &outValue);
        static void SetInt(void *instanceHandle, const std::string &fieldName, int value);

        static bool GetBool(void *instanceHandle, const std::string &fieldName, bool &outValue);
        static void SetBool(void *instanceHandle, const std::string &fieldName, bool value);

        static bool GetVector2(void *instanceHandle, const std::string &fieldName, glm::vec2 &outValue);
        static void SetVector2(void *instanceHandle, const std::string &fieldName, const glm::vec2 &value);
        static bool GetVector3(void *instanceHandle, const std::string &fieldName, glm::vec3 &outValue);
        static void SetVector3(void *instanceHandle, const std::string &fieldName, const glm::vec3 &value);
        static bool GetVector4(void *instanceHandle, const std::string &fieldName, glm::vec4 &outValue);
        static void SetVector4(void *instanceHandle, const std::string &fieldName, const glm::vec4 &value);

        static bool GetString(void *instanceHandle, const std::string &fieldName, std::string &outValue);
        static void SetString(void *instanceHandle, const std::string &fieldName, const std::string &value);

        static bool GetEntityField(void *instanceHandle, const std::string &fieldName, UUID &outEntityID);
        static void SetEntityField(void *instanceHandle, const std::string &fieldName, UUID entityID);

        static void OnCollisionEnter2D(Entity entity, Entity other, const Collision2DInterop &collision);
        static void OnCollisionExit2D(Entity entity, Entity other);
        static void OnTriggerEnter2D(Entity entity, Entity other, const Collision2DInterop &collision);
        static void OnTriggerExit2D(Entity entity, Entity other);

        enum class ScriptPointerEventType
        {
            Enter = 0,
            Exit = 1,
            Down = 2,
            Up = 3,
            Click = 4
        };
        static void OnPointerEvent(Entity entity, ScriptPointerEventType eventType);

        static void* GetEntityScriptInstance(UUID entityID);

        static std::string Serialize(void* instanceHandle);
        static void Deserialize(void* instanceHandle, const std::string& json);

		static Scene* GetSceneContext();
        static bool IsRuntimeActive();
        static void SetActiveSceneRelativePath(const std::string& relativePath);
        static const std::string& GetActiveSceneRelativePath();

    private:
        static void InitInterop();
        static void ReleaseAllInstances();
        static void DestroyInstance(void* handle);

    private:
        static Scene* s_SceneContext;
        static std::string s_ActiveSceneRelativePath;
        static std::unordered_map<UUID, void*> s_EntityInstanceMap;
	};


}
