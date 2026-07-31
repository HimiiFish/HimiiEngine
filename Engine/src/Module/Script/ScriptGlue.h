#pragma once
#include "glm/glm.hpp"

namespace Himii
{

    struct ScriptEngineData {
        void *LogFunc;

        //Entity
        void *Entity_HasComponent;

        //Scene
        void *Scene_CreateEntity;
        void *Scene_DestroyEntity;
        void *Scene_FindEntityByName;

        // Transform
        void *Transform_GetTranslation;
        void *Transform_SetTranslation;
        void *Transform_GetRotation;
        void *Transform_SetRotation;
        void *Transform_GetScale;
        void *Transform_SetScale;

        // Input
        void *Input_IsKeyDown;
        void *Input_IsMouseButtonDown;
        void *Input_GetMousePosition;

        // Rigidbody2D
        void *Rigidbody2D_ApplyLinearImpulse;
        void *Rigidbody2D_ApplyLinearImpulseToCenter;
        void *Rigidbody2D_GetLinearVelocity;
        void *Rigidbody2D_SetLinearVelocity;

        void *BoxCollider2D_GetOffset;
        void *BoxCollider2D_SetOffset;
        void *BoxCollider2D_GetSize;
        void *BoxCollider2D_SetSize;
        void *BoxCollider2D_GetDensity;
        void *BoxCollider2D_SetDensity;
        void *BoxCollider2D_GetFriction;
        void *BoxCollider2D_SetFriction;
        void *BoxCollider2D_GetRestitution;
        void *BoxCollider2D_SetRestitution;

        void *CircleCollider2D_GetOffset;
        void *CircleCollider2D_SetOffset;
        void *CircleCollider2D_GetRadius;
        void *CircleCollider2D_SetRadius;
        void *CircleCollider2D_GetDensity;
        void *CircleCollider2D_SetDensity;
        void *CircleCollider2D_GetFriction;
        void *CircleCollider2D_SetFriction;
        void *CircleCollider2D_GetRestitution;
        void *CircleCollider2D_SetRestitution;

        // Tilemap
        void *Tilemap_GetSize;
        void *Tilemap_SetSize;
        void *Tilemap_GetTile;
        void *Tilemap_SetTile;

        // Physics2D
        void *Physics2D_Raycast;

        // Time
        void *Time_GetDeltaTime;

        // Scene
        void *SceneManager_LoadScene;

        // SpriteRenderer
        void *SpriteRenderer_GetColor;
        void *SpriteRenderer_SetColor;
        void *SpriteRenderer_GetSpriteHandle;
        void *SpriteRenderer_SetSpriteHandle;
        void *SpriteRenderer_GetTextureHandle;
        void *SpriteRenderer_SetTextureHandle;
        void *SpriteRenderer_GetFlipHorizontal;
        void *SpriteRenderer_SetFlipHorizontal;

        void *SpriteAnimation_GetPlaying;
        void *SpriteAnimation_SetPlaying;
        void *SpriteAnimation_GetFrameRate;
        void *SpriteAnimation_SetFrameRate;
        void *SpriteAnimation_ResetPlayback;
        void *SpriteAnimation_Play;
        void *SpriteAnimation_GetCurrentAnimationName;
        void *SpriteAnimation_SetCurrentAnimationName;

        void *Tilemap_GetBounds;

        void *Input_IsKeyPressed;
        void *Input_IsKeyReleased;
        void *Input_GetAxisHorizontal;
        void *Input_GetAxisVertical;

        void *Rigidbody2D_GetBodyType;
        void *Rigidbody2D_SetBodyType;
        void *Rigidbody2D_GetFixedRotation;
        void *Rigidbody2D_SetFixedRotation;

        void *BoxCollider2D_GetIsTrigger;
        void *BoxCollider2D_SetIsTrigger;
        void *BoxCollider2D_GetLayer;
        void *BoxCollider2D_SetLayer;

        void *CircleCollider2D_GetIsTrigger;
        void *CircleCollider2D_SetIsTrigger;
        void *CircleCollider2D_GetLayer;
        void *CircleCollider2D_SetLayer;

        void *SpriteRenderer_GetSortingLayer;
        void *SpriteRenderer_SetSortingLayer;
        void *SpriteRenderer_GetSortingOrder;
        void *SpriteRenderer_SetSortingOrder;

        void *Camera_GetOrthographicSize;
        void *Camera_SetOrthographicSize;
        void *Camera_GetBackgroundColor;
        void *Camera_SetBackgroundColor;
        void *Camera_GetPrimary;
        void *Camera_SetPrimary;

        void *SceneManager_GetActiveScenePath;
        void *Scene_InstantiatePrefab;

        void *Entity_GetParent;
        void *Entity_SetParent;
        void *Entity_GetChildCount;
        void *Entity_GetChildAt;
        void *Transform_GetWorldTranslation;
        void *Transform_SetWorldTranslation;
        void *Transform_GetWorldRotation;

        void *UIText_GetText;
        void *UIText_SetText;
        void *UIText_GetColor;
        void *UIText_SetColor;
        void *UIText_GetFontSize;
        void *UIText_SetFontSize;

        void *UIButton_GetInteractable;
        void *UIButton_SetInteractable;
        void *UIButton_GetIsPointerInside;
        void *UIButton_GetIsPressed;
        void *UIButton_GetWasClickedThisFrame;

        void *SoundPlayer_Play;
        void *SoundPlayer_Stop;
        void *SoundPlayer_Pause;
        void *SoundPlayer_Resume;
        void *SoundPlayer_PlayOneShot;
        void *SoundPlayer_GetVolume;
        void *SoundPlayer_SetVolume;
        void *SoundPlayer_GetMute;
        void *SoundPlayer_SetMute;
        void *SoundPlayer_GetLoop;
        void *SoundPlayer_SetLoop;
        void *SoundPlayer_GetPlayOnStart;
        void *SoundPlayer_SetPlayOnStart;
        void *SoundPlayer_GetSoundHandle;
        void *SoundPlayer_SetSoundHandle;

        void *FontAsset_GetDefaultHandle;
        void *FontAsset_PreloadCharacters;
        void *FontAsset_PreloadTextAsync;
        void *FontAsset_WaitForPendingGenerations;
    };

    class ScriptGlue {
    public:
        //static void RegisterFunctions();
        static ScriptEngineData GetNativeFunctions();
    };

}
