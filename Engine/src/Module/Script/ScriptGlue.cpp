#include "Hepch.h"
#include "ScriptGlue.h"
#include "ScriptEngine.h"

#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"
#include "World/Scene/Components.h"
#include "Module/Tilemap/TileMapData.h"
#include "World/Scene/SceneSerializer.h"
#include "World/Scene/PrefabSerializer.h"
#include "Module/Animation/SpriteAnimationUtility.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Resource/AssetManager.h"
#include "EngineCore/Core/Input.h"
#include "EngineCore/Core/KeyCodes.h"
#include "EngineCore/Core/Log.h"
#include "EngineCore/Core/JobSystem.h"
#include "EngineCore/Math/Math.h"
#include "Module/Render/Font.h"
#include "Module/Audio/SoundPlayerUtility.h"
#include <box2d/box2d.h>
#include <iostream>
#include <thread>
#include <chrono>

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

	static void NativeLog(int level, char* message)
	{
		if (!message)
			return;

		Log::PrintMessage(static_cast<LogLevel>(level), message, "Script");
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
        return entity ? (uint64_t)entity.GetUUID() : 0;
    }

    // 稳定 FNV-1a 32-bit（与 ScriptCore/HashUtils.cs 保持一致）
    static uint32_t Fnv1A32(std::string_view text)
    {
        constexpr uint32_t FnvOffsetBasis = 2166136261u;
        constexpr uint32_t FnvPrime = 16777619u;

        uint32_t hash = FnvOffsetBasis;
        for (char c : text)
        {
            hash ^= (uint8_t)c;
            hash *= FnvPrime;
        }
        return hash;
    }

	// 重要：跨语言返回值用 byte(0/1)，避免 bool ABI/封送问题
	static uint8_t Entity_HasComponent(uint64_t entityID, int typeHashCode)
	{
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return 0;

        const uint32_t typeId = (uint32_t)typeHashCode;

        // ScriptCore 侧的类型全名（namespace + class）
        // 这里先做“最小可用”的映射：满足 Tilemap 脚本用例
        static const uint32_t TilemapId = Fnv1A32("HimiiEngine.Tilemap");
        static const uint32_t Rigidbody2DId = Fnv1A32("HimiiEngine.Rigidbody2D");
        static const uint32_t TransformId = Fnv1A32("HimiiEngine.Transform");
        static const uint32_t SpriteRendererId = Fnv1A32("HimiiEngine.SpriteRenderer");
        static const uint32_t SpriteAnimationId = Fnv1A32("HimiiEngine.SpriteAnimation");
        static const uint32_t BoxCollider2DId = Fnv1A32("HimiiEngine.BoxCollider2D");
        static const uint32_t CircleCollider2DId = Fnv1A32("HimiiEngine.CircleCollider2D");
        static const uint32_t CameraId = Fnv1A32("HimiiEngine.Camera");
        static const uint32_t UITextId = Fnv1A32("HimiiEngine.UIText");
        static const uint32_t UIButtonId = Fnv1A32("HimiiEngine.UIButton");
        static const uint32_t SoundPlayerId = Fnv1A32("HimiiEngine.SoundPlayer");

        if (typeId == TilemapId)
            return entity.HasComponent<TilemapComponent>() ? 1 : 0;
        if (typeId == Rigidbody2DId)
            return entity.HasComponent<Rigidbody2DComponent>() ? 1 : 0;
        if (typeId == TransformId)
            return entity.HasComponent<TransformComponent>() ? 1 : 0;
        if (typeId == SpriteRendererId)
            return entity.HasComponent<SpriteRendererComponent>() ? 1 : 0;
        if (typeId == SpriteAnimationId)
            return entity.HasComponent<SpriteAnimationComponent>() ? 1 : 0;
        if (typeId == BoxCollider2DId)
            return entity.HasComponent<BoxCollider2DComponent>() ? 1 : 0;
        if (typeId == CircleCollider2DId)
            return entity.HasComponent<CircleCollider2DComponent>() ? 1 : 0;
        if (typeId == CameraId)
            return entity.HasComponent<CameraComponent>() ? 1 : 0;
        if (typeId == UITextId)
            return entity.HasComponent<UITextComponent>() ? 1 : 0;
        if (typeId == UIButtonId)
            return entity.HasComponent<UIButtonComponent>() ? 1 : 0;
        if (typeId == SoundPlayerId)
            return entity.HasComponent<SoundPlayerComponent>() ? 1 : 0;

        // 未注册的组件类型：默认认为不存在
        return 0;
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
        scene->NotifyEntityLocalTransformChanged(entity);
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
        scene->NotifyEntityLocalTransformChanged(entity);
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
        scene->NotifyEntityLocalTransformChanged(entity);
    }

    static void Transform_GetWorldTranslation(uint64_t entityID, glm::vec3* outTranslation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || !outTranslation)
        {
            if (outTranslation)
                *outTranslation = glm::vec3(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
        {
            *outTranslation = glm::vec3(0.0f);
            return;
        }

        *outTranslation = scene->GetEntityWorldTranslation(entity);
    }

    static void Transform_SetWorldTranslation(uint64_t entityID, glm::vec3* translation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || !translation)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<TransformComponent>())
            return;

        glm::vec3 worldRotation = scene->GetEntityWorldRotation(entity);
        glm::vec3 worldScale = scene->GetEntityWorldScale(entity);
        glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), *translation)
                                * glm::toMat4(glm::quat(worldRotation))
                                * glm::scale(glm::mat4(1.0f), worldScale);
        scene->ApplyWorldMatrixAsLocalTransform(entity, worldMatrix);
        scene->NotifyEntityLocalTransformChanged(entity);
    }

    static void Transform_GetWorldRotation(uint64_t entityID, glm::vec3* outRotation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || !outRotation)
        {
            if (outRotation)
                *outRotation = glm::vec3(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
        {
            *outRotation = glm::vec3(0.0f);
            return;
        }

        *outRotation = scene->GetEntityWorldRotation(entity);
    }

    static uint64_t Entity_GetParent(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return 0;

        Entity parentEntity = scene->GetParentEntity(entity);
        return parentEntity ? (uint64_t)parentEntity.GetUUID() : 0;
    }

    static void Entity_SetParent(uint64_t childEntityID, uint64_t parentEntityID, uint8_t keepWorldPosition)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity childEntity = scene->GetEntityByUUID(childEntityID);
        if (!childEntity)
            return;

        Entity parentEntity = parentEntityID != 0 ? scene->GetEntityByUUID(parentEntityID) : Entity{};
        scene->SetEntityParent(childEntity, parentEntity, keepWorldPosition != 0);
    }

    static int Entity_GetChildCount(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return 0;

        return static_cast<int>(scene->GetEntityChildren(entity).size());
    }

    static uint64_t Entity_GetChildAt(uint64_t entityID, int childIndex)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || childIndex < 0)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity)
            return 0;

        const std::vector<UUID>& children = scene->GetEntityChildren(entity);
        if (childIndex >= static_cast<int>(children.size()))
            return 0;

        return children[static_cast<size_t>(childIndex)];
    }

    static const char* UIText_GetText(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return "";

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return "";

        return entity.GetComponent<UITextComponent>().TextString.c_str();
    }

    static void UIText_SetText(uint64_t entityID, const char* text)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || !text)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return;

        entity.GetComponent<UITextComponent>().TextString = text;
    }

    static void UIText_GetColor(uint64_t entityID, glm::vec4* outColor)
    {
        if (!outColor)
            return;
        *outColor = glm::vec4(1.0f);

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return;

        *outColor = entity.GetComponent<UITextComponent>().Color;
    }

    static void UIText_SetColor(uint64_t entityID, glm::vec4* color)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene || !color)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return;

        entity.GetComponent<UITextComponent>().Color = *color;
    }

    static float UIText_GetFontSize(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 48.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return 48.0f;

        return entity.GetComponent<UITextComponent>().FontSize;
    }

    static void UIText_SetFontSize(uint64_t entityID, float fontSize)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UITextComponent>())
            return;

        entity.GetComponent<UITextComponent>().FontSize = fontSize > 0.0f ? fontSize : 1.0f;
    }

    static uint8_t UIButton_GetInteractable(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UIButtonComponent>())
            return 0;
        return entity.GetComponent<UIButtonComponent>().Interactable ? 1 : 0;
    }

    static void UIButton_SetInteractable(uint64_t entityID, uint8_t interactable)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UIButtonComponent>())
            return;
        entity.GetComponent<UIButtonComponent>().Interactable = interactable != 0;
    }

    static uint8_t UIButton_GetIsPointerInside(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UIButtonComponent>())
            return 0;
        return entity.GetComponent<UIButtonComponent>().IsPointerInside ? 1 : 0;
    }

    static uint8_t UIButton_GetIsPressed(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UIButtonComponent>())
            return 0;
        return entity.GetComponent<UIButtonComponent>().IsPressed ? 1 : 0;
    }

    static uint8_t UIButton_GetWasClickedThisFrame(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<UIButtonComponent>())
            return 0;
        return entity.GetComponent<UIButtonComponent>().WasClickedThisFrame ? 1 : 0;
    }

    static void SoundPlayer_Play(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        SoundPlayerUtility::Play(entity.GetComponent<SoundPlayerComponent>());
    }

    static void SoundPlayer_Stop(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        SoundPlayerUtility::Stop(entity.GetComponent<SoundPlayerComponent>());
    }

    static void SoundPlayer_Pause(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        SoundPlayerUtility::Pause(entity.GetComponent<SoundPlayerComponent>());
    }

    static void SoundPlayer_Resume(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        SoundPlayerUtility::Resume(entity.GetComponent<SoundPlayerComponent>());
    }

    static void SoundPlayer_PlayOneShot(uint64_t entityID, uint64_t soundHandle)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        SoundPlayerUtility::PlayOneShot(entity.GetComponent<SoundPlayerComponent>(), soundHandle);
    }

    static float SoundPlayer_GetVolume(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return 0.0f;
        return entity.GetComponent<SoundPlayerComponent>().Volume;
    }

    static void SoundPlayer_SetVolume(uint64_t entityID, float volume)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        auto& soundPlayer = entity.GetComponent<SoundPlayerComponent>();
        soundPlayer.Volume = volume;
        SoundPlayerUtility::ApplyVolume(soundPlayer);
    }

    static uint8_t SoundPlayer_GetMute(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return 0;
        return entity.GetComponent<SoundPlayerComponent>().Mute ? 1 : 0;
    }

    static void SoundPlayer_SetMute(uint64_t entityID, uint8_t mute)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        auto& soundPlayer = entity.GetComponent<SoundPlayerComponent>();
        soundPlayer.Mute = mute != 0;
        SoundPlayerUtility::ApplyVolume(soundPlayer);
    }

    static uint8_t SoundPlayer_GetLoop(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return 0;
        return entity.GetComponent<SoundPlayerComponent>().Loop ? 1 : 0;
    }

    static void SoundPlayer_SetLoop(uint64_t entityID, uint8_t loop)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        entity.GetComponent<SoundPlayerComponent>().Loop = loop != 0;
    }

    static uint8_t SoundPlayer_GetPlayOnStart(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return 0;
        return entity.GetComponent<SoundPlayerComponent>().PlayOnStart ? 1 : 0;
    }

    static void SoundPlayer_SetPlayOnStart(uint64_t entityID, uint8_t playOnStart)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        entity.GetComponent<SoundPlayerComponent>().PlayOnStart = playOnStart != 0;
    }

    static uint64_t SoundPlayer_GetSoundHandle(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return 0;
        return static_cast<uint64_t>(entity.GetComponent<SoundPlayerComponent>().SoundHandle);
    }

    static void SoundPlayer_SetSoundHandle(uint64_t entityID, uint64_t soundHandle)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;
        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SoundPlayerComponent>())
            return;
        auto& soundPlayer = entity.GetComponent<SoundPlayerComponent>();
        soundPlayer.SoundHandle = soundHandle;
        SoundPlayerUtility::ResolveSoundAsset(soundPlayer);
    }

    static Ref<Font> ResolveFontAssetByHandle(uint64_t handle)
    {
        Ref<Font> defaultFont = Font::GetDefault();
        if (defaultFont && static_cast<uint64_t>(defaultFont->Handle) == handle)
            return defaultFont;

        if (Project::GetActive())
        {
            if (auto assetManager = ResourceSystem::GetAssetManager())
            {
                if (Ref<Asset> asset = assetManager->GetAsset(handle))
                    return std::dynamic_pointer_cast<Font>(asset);
            }
        }
        return nullptr;
    }

    static uint64_t FontAsset_GetDefaultHandle()
    {
        Ref<Font> defaultFont = Font::GetDefault();
        return defaultFont ? static_cast<uint64_t>(defaultFont->Handle) : 0;
    }

    static void FontAsset_PreloadCharacters(uint64_t handle, const char *text)
    {
        Ref<Font> font = ResolveFontAssetByHandle(handle);
        if (!font || !text)
            return;
        font->PreloadCharacters(text);
    }

    static void FontAsset_PreloadTextAsync(uint64_t handle, const char *text)
    {
        Ref<Font> font = ResolveFontAssetByHandle(handle);
        if (!font || !text)
            return;
        font->PreloadTextAsync(text);
    }

    static void FontAsset_WaitForPendingGenerations(uint64_t handle)
    {
        Ref<Font> font = ResolveFontAssetByHandle(handle);
        if (!font)
            return;

        for (int attempt = 0; attempt < 2000; ++attempt)
        {
            JobSystem::PumpMainThreadCompletions();
            font->ProcessCompletedGenerations();
            if (JobSystem::GetPendingWorkerTaskCount() == 0
                && JobSystem::GetPendingMainThreadCompletionCount() == 0)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        font->ProcessCompletedGenerations();
    }

    static uint8_t Input_IsKeyDown(int keycode)
    {
        return Input::IsKeyPressed(keycode) ? 1 : 0;
    }

    static uint8_t Input_IsKeyPressed(int keycode)
    {
        return Input::IsKeyJustPressed(keycode) ? 1 : 0;
    }

    static uint8_t Input_IsKeyReleased(int keycode)
    {
        return Input::IsKeyJustReleased(keycode) ? 1 : 0;
    }

    static float Input_GetAxisHorizontal()
    {
        return Input::GetAxisHorizontal();
    }

    static float Input_GetAxisVertical()
    {
        return Input::GetAxisVertical();
    }

    static uint8_t Input_IsMouseButtonDown(int button)
    {
        return Input::IsMouseButtonPressed(button) ? 1 : 0;
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

    static void BoxCollider2D_GetOffset(uint64_t entityID, glm::vec2* outOffset)
    {
        if (!outOffset)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            *outOffset = glm::vec2(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
        {
            *outOffset = glm::vec2(0.0f);
            return;
        }

        *outOffset = entity.GetComponent<BoxCollider2DComponent>().Offset;
    }

    static void BoxCollider2D_SetOffset(uint64_t entityID, glm::vec2* offset)
    {
        if (!offset)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Offset = *offset;
    }

    static void BoxCollider2D_GetSize(uint64_t entityID, glm::vec2* outSize)
    {
        if (!outSize)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            *outSize = glm::vec2(1.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
        {
            *outSize = glm::vec2(1.0f);
            return;
        }

        *outSize = entity.GetComponent<BoxCollider2DComponent>().Size;
    }

    static void BoxCollider2D_SetSize(uint64_t entityID, glm::vec2* size)
    {
        if (!size)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Size = *size;
    }

    static float BoxCollider2D_GetDensity(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<BoxCollider2DComponent>().Density;
    }

    static void BoxCollider2D_SetDensity(uint64_t entityID, float density)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Density = density;
    }

    static float BoxCollider2D_GetFriction(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<BoxCollider2DComponent>().Friction;
    }

    static void BoxCollider2D_SetFriction(uint64_t entityID, float friction)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Friction = friction;
    }

    static float BoxCollider2D_GetRestitution(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<BoxCollider2DComponent>().Restitution;
    }

    static void BoxCollider2D_SetRestitution(uint64_t entityID, float restitution)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Restitution = restitution;
    }

    static void CircleCollider2D_GetOffset(uint64_t entityID, glm::vec2* outOffset)
    {
        if (!outOffset)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
        {
            *outOffset = glm::vec2(0.0f);
            return;
        }

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
        {
            *outOffset = glm::vec2(0.0f);
            return;
        }

        *outOffset = entity.GetComponent<CircleCollider2DComponent>().Offset;
    }

    static void CircleCollider2D_SetOffset(uint64_t entityID, glm::vec2* offset)
    {
        if (!offset)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Offset = *offset;
    }

    static float CircleCollider2D_GetRadius(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<CircleCollider2DComponent>().Radius;
    }

    static void CircleCollider2D_SetRadius(uint64_t entityID, float radius)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Radius = radius;
    }

    static float CircleCollider2D_GetDensity(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<CircleCollider2DComponent>().Density;
    }

    static void CircleCollider2D_SetDensity(uint64_t entityID, float density)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Density = density;
    }

    static float CircleCollider2D_GetFriction(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<CircleCollider2DComponent>().Friction;
    }

    static void CircleCollider2D_SetFriction(uint64_t entityID, float friction)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Friction = friction;
    }

    static float CircleCollider2D_GetRestitution(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0.0f;

        return entity.GetComponent<CircleCollider2DComponent>().Restitution;
    }

    static void CircleCollider2D_SetRestitution(uint64_t entityID, float restitution)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Restitution = restitution;
    }

    static b2BodyId GetRuntimeBodyId(void* runtimeBodyPointer)
    {
        if (!runtimeBodyPointer)
            return {};

        return *reinterpret_cast<b2BodyId*>(runtimeBodyPointer);
    }

    static b2BodyType ToBoxBodyType(Rigidbody2DComponent::BodyType bodyType)
    {
        switch (bodyType)
        {
            case Rigidbody2DComponent::BodyType::Static:
                return b2BodyType::b2_staticBody;
            case Rigidbody2DComponent::BodyType::Dynamic:
                return b2BodyType::b2_dynamicBody;
            case Rigidbody2DComponent::BodyType::Kinematic:
                return b2BodyType::b2_kinematicBody;
        }

        return b2BodyType::b2_staticBody;
    }

    static int Rigidbody2D_GetBodyType(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return 0;

        return static_cast<int>(entity.GetComponent<Rigidbody2DComponent>().Type);
    }

    static void Rigidbody2D_SetBodyType(uint64_t entityID, int bodyType)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto& rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
        rigidbody2D.Type = static_cast<Rigidbody2DComponent::BodyType>(bodyType);

        b2BodyId bodyId = GetRuntimeBodyId(rigidbody2D.RuntimeBody);
        if (b2Body_IsValid(bodyId))
            b2Body_SetType(bodyId, ToBoxBodyType(rigidbody2D.Type));
    }

    static uint8_t Rigidbody2D_GetFixedRotation(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return 0;

        return entity.GetComponent<Rigidbody2DComponent>().FixedRotation ? 1 : 0;
    }

    static void Rigidbody2D_SetFixedRotation(uint64_t entityID, uint8_t fixedRotation)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto& rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
        rigidbody2D.FixedRotation = fixedRotation != 0;

        b2BodyId bodyId = GetRuntimeBodyId(rigidbody2D.RuntimeBody);
        if (b2Body_IsValid(bodyId))
            b2Body_SetFixedRotation(bodyId, rigidbody2D.FixedRotation);
    }

    static uint8_t BoxCollider2D_GetIsTrigger(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return 0;

        return entity.GetComponent<BoxCollider2DComponent>().IsTrigger ? 1 : 0;
    }

    static void BoxCollider2D_SetIsTrigger(uint64_t entityID, uint8_t isTrigger)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().IsTrigger = isTrigger != 0;
    }

    static int BoxCollider2D_GetLayer(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return 0;

        return entity.GetComponent<BoxCollider2DComponent>().Layer;
    }

    static void BoxCollider2D_SetLayer(uint64_t entityID, int layer)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<BoxCollider2DComponent>())
            return;

        entity.GetComponent<BoxCollider2DComponent>().Layer = layer;
    }

    static uint8_t CircleCollider2D_GetIsTrigger(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0;

        return entity.GetComponent<CircleCollider2DComponent>().IsTrigger ? 1 : 0;
    }

    static void CircleCollider2D_SetIsTrigger(uint64_t entityID, uint8_t isTrigger)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().IsTrigger = isTrigger != 0;
    }

    static int CircleCollider2D_GetLayer(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return 0;

        return entity.GetComponent<CircleCollider2DComponent>().Layer;
    }

    static void CircleCollider2D_SetLayer(uint64_t entityID, int layer)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CircleCollider2DComponent>())
            return;

        entity.GetComponent<CircleCollider2DComponent>().Layer = layer;
    }

    // 辅助：从实体的 TilemapComponent 获取 TileMapData 资源
    static Ref<TileMapData> GetTileMapDataFromEntity(Entity entity)
    {
        if (!entity || !entity.HasComponent<TilemapComponent>()) return nullptr;
        auto &tc = entity.GetComponent<TilemapComponent>();
        if (tc.TileMapHandle == 0) return nullptr;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager) return nullptr;

        auto asset = assetManager->GetAsset(tc.TileMapHandle);
        if (!asset) return nullptr;
        return std::static_pointer_cast<TileMapData>(asset);
    }

    // Tilemap Internal Calls
    static void Tilemap_GetSize(uint64_t entityID, uint32_t* outWidth, uint32_t* outHeight)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene) return;
        Entity entity = scene->GetEntityByUUID(entityID);

        auto mapData = GetTileMapDataFromEntity(entity);
        if (!mapData) { *outWidth = 0; *outHeight = 0; return; }

        *outWidth = mapData->GetWidth();
        *outHeight = mapData->GetHeight();
    }

    static void Tilemap_SetSize(uint64_t entityID, uint32_t width, uint32_t height)
    {
        (void)entityID;
        (void)width;
        (void)height;
        HIMII_CORE_WARNING(
                "Tilemap_SetSize is deprecated: tilemaps auto-expand. Use paint/SetTile instead.");
    }

    static uint16_t Tilemap_GetTile(uint64_t entityID, int32_t x, int32_t y)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene) return 0;
        Entity entity = scene->GetEntityByUUID(entityID);

        auto mapData = GetTileMapDataFromEntity(entity);
        if (!mapData) return 0;

        return mapData->GetTile(x, y);
    }

    static void Tilemap_SetTile(uint64_t entityID, int32_t x, int32_t y, uint16_t tileID)
    {
        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene) return;
        Entity entity = scene->GetEntityByUUID(entityID);

        auto mapData = GetTileMapDataFromEntity(entity);
        if (!mapData) return;

        mapData->SetTile(x, y, tileID);
    }

    static void Tilemap_GetBounds(uint64_t entityID, int32_t* outMinX, int32_t* outMinY,
                                int32_t* outMaxX, int32_t* outMaxY)
    {
        if (outMinX) *outMinX = 0;
        if (outMinY) *outMinY = 0;
        if (outMaxX) *outMaxX = 0;
        if (outMaxY) *outMaxY = 0;

        Scene *scene = ScriptEngine::GetSceneContext();
        if (!scene) return;
        Entity entity = scene->GetEntityByUUID(entityID);

        auto mapData = GetTileMapDataFromEntity(entity);
        if (!mapData || !mapData->HasBounds())
            return;

        mapData->GetBounds(*outMinX, *outMinY, *outMaxX, *outMaxY);
    }

    static void Physics2D_Raycast(glm::vec2* start, glm::vec2* end, Scene::RaycastHit2D* outHit)
    {
         Scene *scene = ScriptEngine::GetSceneContext();
         if (!scene) return;
         
         *outHit = scene->Raycast2D(*start, *end);
    }

    static float Time_GetDeltaTime()
    {
        return ScriptEngine::GetScriptDeltaTime();
    }

    static void SpriteRenderer_GetColor(uint64_t entityID, glm::vec4* outColor)
    {
        if (!outColor)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity && entity.HasComponent<SpriteRendererComponent>())
            *outColor = entity.GetComponent<SpriteRendererComponent>().Color;
    }

    static void SpriteRenderer_SetColor(uint64_t entityID, const glm::vec4* color)
    {
        if (!color)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity && entity.HasComponent<SpriteRendererComponent>())
            entity.GetComponent<SpriteRendererComponent>().Color = *color;
    }

    static uint64_t SpriteRenderer_GetSpriteHandle(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (entity && entity.HasComponent<SpriteRendererComponent>())
            return static_cast<uint64_t>(entity.GetComponent<SpriteRendererComponent>().SpriteAssetHandle);
        return 0;
    }

    static void SpriteRenderer_SetSpriteHandle(uint64_t entityID, uint64_t spriteHandle)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return;

        entity.GetComponent<SpriteRendererComponent>().SpriteAssetHandle = spriteHandle;
    }

    static uint64_t SpriteRenderer_GetTextureHandle(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return 0;

        const AssetHandle spriteAssetHandle =
            entity.GetComponent<SpriteRendererComponent>().SpriteAssetHandle;
        if (spriteAssetHandle == 0 || !Project::GetActive())
            return 0;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return 0;

        return static_cast<uint64_t>(
            assetManager->GetTextureHandleForSprite(spriteAssetHandle));
    }

    static uint8_t SpriteRenderer_GetFlipHorizontal(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return 0;

        return entity.GetComponent<SpriteRendererComponent>().FlipHorizontal ? 1 : 0;
    }

    static void SpriteRenderer_SetFlipHorizontal(uint64_t entityID, uint8_t flipHorizontal)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return;

        entity.GetComponent<SpriteRendererComponent>().FlipHorizontal = flipHorizontal != 0;
    }

    static int SpriteRenderer_GetSortingLayer(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return 0;

        return entity.GetComponent<SpriteRendererComponent>().SortingLayer;
    }

    static void SpriteRenderer_SetSortingLayer(uint64_t entityID, int sortingLayer)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return;

        entity.GetComponent<SpriteRendererComponent>().SortingLayer = sortingLayer;
    }

    static int SpriteRenderer_GetSortingOrder(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return 0;

        return entity.GetComponent<SpriteRendererComponent>().SortingOrder;
    }

    static void SpriteRenderer_SetSortingOrder(uint64_t entityID, int sortingOrder)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return;

        entity.GetComponent<SpriteRendererComponent>().SortingOrder = sortingOrder;
    }

    static float Camera_GetOrthographicSize(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 10.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return 10.0f;

        return entity.GetComponent<CameraComponent>().Camera.GetOrthographicSize();
    }

    static void Camera_SetOrthographicSize(uint64_t entityID, float orthographicSize)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return;

        entity.GetComponent<CameraComponent>().Camera.SetOrthographicSize(orthographicSize);
    }

    static void Camera_GetBackgroundColor(uint64_t entityID, glm::vec4* outColor)
    {
        if (!outColor)
            return;

        *outColor = glm::vec4(0.0f);

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return;

        *outColor = entity.GetComponent<CameraComponent>().Camera.GetBackgroundColor();
    }

    static void Camera_SetBackgroundColor(uint64_t entityID, const glm::vec4* color)
    {
        if (!color)
            return;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return;

        entity.GetComponent<CameraComponent>().Camera.SetBackgroundColor(*color);
    }

    static uint8_t Camera_GetPrimary(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return 0;

        return entity.GetComponent<CameraComponent>().Primary ? 1 : 0;
    }

    static void Camera_SetPrimary(uint64_t entityID, uint8_t primary)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<CameraComponent>())
            return;

        entity.GetComponent<CameraComponent>().Primary = primary != 0;
    }

    static void SpriteRenderer_SetTextureHandle(uint64_t entityID, uint64_t textureHandle)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteRendererComponent>())
            return;

        if (!Project::GetActive() || textureHandle == 0)
        {
            entity.GetComponent<SpriteRendererComponent>().SpriteAssetHandle = 0;
            return;
        }

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(textureHandle))
            return;

        entity.GetComponent<SpriteRendererComponent>().SpriteAssetHandle =
            assetManager->GetDefaultSpriteHandleForTexture(textureHandle);
    }

    static uint8_t SpriteAnimation_GetPlaying(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return 0;

        return entity.GetComponent<SpriteAnimationComponent>().Playing ? 1 : 0;
    }

    static void SpriteAnimation_SetPlaying(uint64_t entityID, uint8_t playing)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return;

        entity.GetComponent<SpriteAnimationComponent>().Playing = playing != 0;
    }

    static float SpriteAnimation_GetFrameRate(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0.0f;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return 0.0f;

        return entity.GetComponent<SpriteAnimationComponent>().FrameRate;
    }

    static void SpriteAnimation_SetFrameRate(uint64_t entityID, float frameRate)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return;

        entity.GetComponent<SpriteAnimationComponent>().FrameRate = frameRate;
    }

    static void SpriteAnimation_ResetPlayback(uint64_t entityID)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return;

        ResetSpriteAnimationPlayback(entity.GetComponent<SpriteAnimationComponent>());
    }

    static void SpriteAnimation_Play(uint64_t entityID, const char* animationName)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return;

        SpriteAnimationComponent& animationComponent = entity.GetComponent<SpriteAnimationComponent>();
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || animationComponent.AnimationHandle == 0
            || !assetManager->IsAssetHandleValid(animationComponent.AnimationHandle))
        {
            ResetSpriteAnimationPlayback(animationComponent);
            animationComponent.Playing = true;
            return;
        }

        Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                assetManager->GetAsset(animationComponent.AnimationHandle));

        if (animation && animationName && animationName[0] != '\0'
            && animation->HasAnimation(animationName))
        {
            animationComponent.CurrentAnimationName = animationName;
        }
        else if (animation)
        {
            if (const SpriteAnimationClip* primaryClip = animation->GetPrimaryClip())
                animationComponent.CurrentAnimationName = primaryClip->Name;
        }

        ResetSpriteAnimationPlayback(animationComponent);
        animationComponent.Playing = true;
    }

    static const char* SpriteAnimation_GetCurrentAnimationName(uint64_t entityID)
    {
        static thread_local char animationNameBuffer[256] = {};

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return "";

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return "";

        const std::string& currentName = entity.GetComponent<SpriteAnimationComponent>().CurrentAnimationName;
        std::snprintf(animationNameBuffer, sizeof(animationNameBuffer), "%s", currentName.c_str());
        return animationNameBuffer;
    }

    static void SpriteAnimation_SetCurrentAnimationName(uint64_t entityID, const char* animationName)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return;

        Entity entity = scene->GetEntityByUUID(entityID);
        if (!entity || !entity.HasComponent<SpriteAnimationComponent>())
            return;

        if (!animationName || animationName[0] == '\0')
            return;

        SpriteAnimationComponent& animationComponent = entity.GetComponent<SpriteAnimationComponent>();
        auto assetManager = ResourceSystem::GetAssetManager();
        if (assetManager && animationComponent.AnimationHandle != 0
            && assetManager->IsAssetHandleValid(animationComponent.AnimationHandle))
        {
            Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                    assetManager->GetAsset(animationComponent.AnimationHandle));
            if (animation && !animation->HasAnimation(animationName))
                return;
        }

        animationComponent.CurrentAnimationName = animationName;
        ResetSpriteAnimationPlayback(animationComponent);
    }

    static uint8_t SceneManager_LoadScene(const char* scenePath)
    {
        if (!scenePath || scenePath[0] == '\0')
            return 0;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        std::filesystem::path fullPath = Project::GetActive()
            ? Project::GetAssetFileSystemPath(scenePath)
            : std::filesystem::path(scenePath);

        if (!std::filesystem::exists(fullPath))
        {
            HIMII_CORE_ERROR("SceneManager.LoadScene: file not found: {0}", fullPath.string());
            return 0;
        }

        ScriptEngine::OnRuntimeStop();
        scene->ClearEntities();

        Ref<Scene> sceneReference(scene, [](Scene*) {});
        SceneSerializer serializer(sceneReference);
        if (!serializer.Deserialize(fullPath.string()))
        {
            HIMII_CORE_ERROR("SceneManager.LoadScene: failed to deserialize: {0}", fullPath.string());
            return 0;
        }

        ScriptEngine::SetActiveSceneRelativePath(scenePath);
        scene->OnRuntimeStart();
        return 1;
    }

    static const char* SceneManager_GetActiveScenePath()
    {
        static thread_local char activeScenePathBuffer[512] = {};
        const std::string& activePath = ScriptEngine::GetActiveSceneRelativePath();
        std::snprintf(activeScenePathBuffer, sizeof(activeScenePathBuffer), "%s", activePath.c_str());
        return activeScenePathBuffer;
    }

    static uint64_t Scene_InstantiatePrefab(const char* prefabPath)
    {
        if (!prefabPath || prefabPath[0] == '\0')
            return 0;

        Scene* scene = ScriptEngine::GetSceneContext();
        if (!scene)
            return 0;

        std::filesystem::path fullPath = Project::GetActive()
            ? Project::GetAssetFileSystemPath(prefabPath)
            : std::filesystem::path(prefabPath);

        Ref<Scene> sceneReference(scene, [](Scene*) {});
        Entity instantiatedEntity = PrefabSerializer::Instantiate(sceneReference, fullPath);
        if (!instantiatedEntity)
            return 0;

        if (ScriptEngine::IsRuntimeActive())
        {
            if (instantiatedEntity.HasComponent<ScriptComponent>())
                ScriptEngine::OnCreateEntity(instantiatedEntity);
        }

        return instantiatedEntity.GetUUID();
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

        data.BoxCollider2D_GetOffset = (void *)&BoxCollider2D_GetOffset;
        data.BoxCollider2D_SetOffset = (void *)&BoxCollider2D_SetOffset;
        data.BoxCollider2D_GetSize = (void *)&BoxCollider2D_GetSize;
        data.BoxCollider2D_SetSize = (void *)&BoxCollider2D_SetSize;
        data.BoxCollider2D_GetDensity = (void *)&BoxCollider2D_GetDensity;
        data.BoxCollider2D_SetDensity = (void *)&BoxCollider2D_SetDensity;
        data.BoxCollider2D_GetFriction = (void *)&BoxCollider2D_GetFriction;
        data.BoxCollider2D_SetFriction = (void *)&BoxCollider2D_SetFriction;
        data.BoxCollider2D_GetRestitution = (void *)&BoxCollider2D_GetRestitution;
        data.BoxCollider2D_SetRestitution = (void *)&BoxCollider2D_SetRestitution;

        data.CircleCollider2D_GetOffset = (void *)&CircleCollider2D_GetOffset;
        data.CircleCollider2D_SetOffset = (void *)&CircleCollider2D_SetOffset;
        data.CircleCollider2D_GetRadius = (void *)&CircleCollider2D_GetRadius;
        data.CircleCollider2D_SetRadius = (void *)&CircleCollider2D_SetRadius;
        data.CircleCollider2D_GetDensity = (void *)&CircleCollider2D_GetDensity;
        data.CircleCollider2D_SetDensity = (void *)&CircleCollider2D_SetDensity;
        data.CircleCollider2D_GetFriction = (void *)&CircleCollider2D_GetFriction;
        data.CircleCollider2D_SetFriction = (void *)&CircleCollider2D_SetFriction;
        data.CircleCollider2D_GetRestitution = (void *)&CircleCollider2D_GetRestitution;
        data.CircleCollider2D_SetRestitution = (void *)&CircleCollider2D_SetRestitution;

        // Tilemap
        data.Tilemap_GetSize = (void *)&Tilemap_GetSize;
        data.Tilemap_SetSize = (void *)&Tilemap_SetSize;
        data.Tilemap_GetTile = (void *)&Tilemap_GetTile;
        data.Tilemap_SetTile = (void *)&Tilemap_SetTile;

        data.Physics2D_Raycast = (void *)&Physics2D_Raycast;

        data.Time_GetDeltaTime = (void *)&Time_GetDeltaTime;
        data.SceneManager_LoadScene = (void *)&SceneManager_LoadScene;
        data.SpriteRenderer_GetColor = (void *)&SpriteRenderer_GetColor;
        data.SpriteRenderer_SetColor = (void *)&SpriteRenderer_SetColor;
        data.SpriteRenderer_GetSpriteHandle = (void *)&SpriteRenderer_GetSpriteHandle;
        data.SpriteRenderer_SetSpriteHandle = (void *)&SpriteRenderer_SetSpriteHandle;
        data.SpriteRenderer_GetTextureHandle = (void *)&SpriteRenderer_GetTextureHandle;
        data.SpriteRenderer_SetTextureHandle = (void *)&SpriteRenderer_SetTextureHandle;
        data.SpriteRenderer_GetFlipHorizontal = (void *)&SpriteRenderer_GetFlipHorizontal;
        data.SpriteRenderer_SetFlipHorizontal = (void *)&SpriteRenderer_SetFlipHorizontal;

        data.SpriteAnimation_GetPlaying = (void *)&SpriteAnimation_GetPlaying;
        data.SpriteAnimation_SetPlaying = (void *)&SpriteAnimation_SetPlaying;
        data.SpriteAnimation_GetFrameRate = (void *)&SpriteAnimation_GetFrameRate;
        data.SpriteAnimation_SetFrameRate = (void *)&SpriteAnimation_SetFrameRate;
        data.SpriteAnimation_ResetPlayback = (void *)&SpriteAnimation_ResetPlayback;
        data.SpriteAnimation_Play = (void *)&SpriteAnimation_Play;
        data.SpriteAnimation_GetCurrentAnimationName = (void *)&SpriteAnimation_GetCurrentAnimationName;
        data.SpriteAnimation_SetCurrentAnimationName = (void *)&SpriteAnimation_SetCurrentAnimationName;

        data.Tilemap_GetBounds = (void *)&Tilemap_GetBounds;

        data.Input_IsKeyPressed = (void *)&Input_IsKeyPressed;
        data.Input_IsKeyReleased = (void *)&Input_IsKeyReleased;
        data.Input_GetAxisHorizontal = (void *)&Input_GetAxisHorizontal;
        data.Input_GetAxisVertical = (void *)&Input_GetAxisVertical;

        data.Rigidbody2D_GetBodyType = (void *)&Rigidbody2D_GetBodyType;
        data.Rigidbody2D_SetBodyType = (void *)&Rigidbody2D_SetBodyType;
        data.Rigidbody2D_GetFixedRotation = (void *)&Rigidbody2D_GetFixedRotation;
        data.Rigidbody2D_SetFixedRotation = (void *)&Rigidbody2D_SetFixedRotation;

        data.BoxCollider2D_GetIsTrigger = (void *)&BoxCollider2D_GetIsTrigger;
        data.BoxCollider2D_SetIsTrigger = (void *)&BoxCollider2D_SetIsTrigger;
        data.BoxCollider2D_GetLayer = (void *)&BoxCollider2D_GetLayer;
        data.BoxCollider2D_SetLayer = (void *)&BoxCollider2D_SetLayer;

        data.CircleCollider2D_GetIsTrigger = (void *)&CircleCollider2D_GetIsTrigger;
        data.CircleCollider2D_SetIsTrigger = (void *)&CircleCollider2D_SetIsTrigger;
        data.CircleCollider2D_GetLayer = (void *)&CircleCollider2D_GetLayer;
        data.CircleCollider2D_SetLayer = (void *)&CircleCollider2D_SetLayer;

        data.SpriteRenderer_GetSortingLayer = (void *)&SpriteRenderer_GetSortingLayer;
        data.SpriteRenderer_SetSortingLayer = (void *)&SpriteRenderer_SetSortingLayer;
        data.SpriteRenderer_GetSortingOrder = (void *)&SpriteRenderer_GetSortingOrder;
        data.SpriteRenderer_SetSortingOrder = (void *)&SpriteRenderer_SetSortingOrder;

        data.Camera_GetOrthographicSize = (void *)&Camera_GetOrthographicSize;
        data.Camera_SetOrthographicSize = (void *)&Camera_SetOrthographicSize;
        data.Camera_GetBackgroundColor = (void *)&Camera_GetBackgroundColor;
        data.Camera_SetBackgroundColor = (void *)&Camera_SetBackgroundColor;
        data.Camera_GetPrimary = (void *)&Camera_GetPrimary;
        data.Camera_SetPrimary = (void *)&Camera_SetPrimary;

        data.SceneManager_GetActiveScenePath = (void *)&SceneManager_GetActiveScenePath;
        data.Scene_InstantiatePrefab = (void *)&Scene_InstantiatePrefab;

        data.Entity_GetParent = (void *)&Entity_GetParent;
        data.Entity_SetParent = (void *)&Entity_SetParent;
        data.Entity_GetChildCount = (void *)&Entity_GetChildCount;
        data.Entity_GetChildAt = (void *)&Entity_GetChildAt;
        data.Transform_GetWorldTranslation = (void *)&Transform_GetWorldTranslation;
        data.Transform_SetWorldTranslation = (void *)&Transform_SetWorldTranslation;
        data.Transform_GetWorldRotation = (void *)&Transform_GetWorldRotation;

        data.UIText_GetText = (void *)&UIText_GetText;
        data.UIText_SetText = (void *)&UIText_SetText;
        data.UIText_GetColor = (void *)&UIText_GetColor;
        data.UIText_SetColor = (void *)&UIText_SetColor;
        data.UIText_GetFontSize = (void *)&UIText_GetFontSize;
        data.UIText_SetFontSize = (void *)&UIText_SetFontSize;

        data.UIButton_GetInteractable = (void *)&UIButton_GetInteractable;
        data.UIButton_SetInteractable = (void *)&UIButton_SetInteractable;
        data.UIButton_GetIsPointerInside = (void *)&UIButton_GetIsPointerInside;
        data.UIButton_GetIsPressed = (void *)&UIButton_GetIsPressed;
        data.UIButton_GetWasClickedThisFrame = (void *)&UIButton_GetWasClickedThisFrame;

        data.SoundPlayer_Play = (void *)&SoundPlayer_Play;
        data.SoundPlayer_Stop = (void *)&SoundPlayer_Stop;
        data.SoundPlayer_Pause = (void *)&SoundPlayer_Pause;
        data.SoundPlayer_Resume = (void *)&SoundPlayer_Resume;
        data.SoundPlayer_PlayOneShot = (void *)&SoundPlayer_PlayOneShot;
        data.SoundPlayer_GetVolume = (void *)&SoundPlayer_GetVolume;
        data.SoundPlayer_SetVolume = (void *)&SoundPlayer_SetVolume;
        data.SoundPlayer_GetMute = (void *)&SoundPlayer_GetMute;
        data.SoundPlayer_SetMute = (void *)&SoundPlayer_SetMute;
        data.SoundPlayer_GetLoop = (void *)&SoundPlayer_GetLoop;
        data.SoundPlayer_SetLoop = (void *)&SoundPlayer_SetLoop;
        data.SoundPlayer_GetPlayOnStart = (void *)&SoundPlayer_GetPlayOnStart;
        data.SoundPlayer_SetPlayOnStart = (void *)&SoundPlayer_SetPlayOnStart;
        data.SoundPlayer_GetSoundHandle = (void *)&SoundPlayer_GetSoundHandle;
        data.SoundPlayer_SetSoundHandle = (void *)&SoundPlayer_SetSoundHandle;

        data.FontAsset_GetDefaultHandle = (void *)&FontAsset_GetDefaultHandle;
        data.FontAsset_PreloadCharacters = (void *)&FontAsset_PreloadCharacters;
        data.FontAsset_PreloadTextAsync = (void *)&FontAsset_PreloadTextAsync;
        data.FontAsset_WaitForPendingGenerations = (void *)&FontAsset_WaitForPendingGenerations;

        return data;
    }
}