using System;
using System.Runtime.InteropServices;

namespace HimiiEngine
{
    public static class InternalCalls
    {
        // delegate definitions
        internal delegate void LogFuncDelegate(int level, IntPtr msg);

        // Interop 注意：不要直接用 bool 作为返回值（跨语言 ABI 容易踩坑）
        // 这里用 byte (0/1) 更稳
        internal delegate byte EntityHasComponentDelegate(ulong entityID, int typeHashCode);
        internal delegate ulong SceneCreateEntityDelegate(IntPtr name);
        internal delegate void SceneDestroyEntityDelegate(ulong entityID);
        internal delegate ulong SceneFindEntityDelegate(IntPtr name);

        internal delegate void TransformPosDelegate(ulong entityID, out Vector3 vec);
        internal delegate void TransformSetPosDelegate(ulong entityID, ref Vector3 vec);

        internal delegate byte InputGetKeyDelegate(KeyCode keycode);
        internal delegate byte InputGetMouseBtnDelegate(int button);
        internal delegate void InputGetMousePosDelegate(out Vector2 pos);

        internal delegate void Rigidbody2DApplyImpulseDelegate(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);
        internal delegate void Rigidbody2DApplyImpulseCenterDelegate(ulong entityID, ref Vector2 impulse, bool wake);
        internal delegate void Rigidbody2DGetVelocityDelegate(ulong entityID, out Vector2 velocity);
        internal delegate void Rigidbody2DSetVelocityDelegate(ulong entityID, ref Vector2 velocity);

        internal delegate void Collider2DGetOffsetDelegate(ulong entityID, out Vector2 offset);
        internal delegate void Collider2DSetOffsetDelegate(ulong entityID, ref Vector2 offset);
        internal delegate void BoxCollider2DGetSizeDelegate(ulong entityID, out Vector2 size);
        internal delegate void BoxCollider2DSetSizeDelegate(ulong entityID, ref Vector2 size);
        internal delegate float Collider2DGetFloatDelegate(ulong entityID);
        internal delegate void Collider2DSetFloatDelegate(ulong entityID, float value);
        internal delegate float InputGetAxisDelegate();
        internal delegate int Rigidbody2DGetBodyTypeDelegate(ulong entityID);
        internal delegate void Rigidbody2DSetBodyTypeDelegate(ulong entityID, int bodyType);
        internal delegate byte Rigidbody2DGetFixedRotationDelegate(ulong entityID);
        internal delegate void Rigidbody2DSetFixedRotationDelegate(ulong entityID, byte fixedRotation);
        internal delegate byte Collider2DGetBoolDelegate(ulong entityID);
        internal delegate void Collider2DSetBoolDelegate(ulong entityID, byte value);
        internal delegate int Collider2DGetIntDelegate(ulong entityID);
        internal delegate void Collider2DSetIntDelegate(ulong entityID, int value);

        internal delegate void TilemapGetSizeDelegate(ulong entityID, out uint width, out uint height);
        internal delegate void TilemapSetSizeDelegate(ulong entityID, uint width, uint height);
        internal delegate ushort TilemapGetTileDelegate(ulong entityID, int x, int y);
        internal delegate void TilemapSetTileDelegate(ulong entityID, int x, int y, ushort tileID);
        internal delegate void TilemapGetBoundsDelegate(ulong entityID, out int minX, out int minY,
                                                        out int maxX, out int maxY);

        internal delegate void Physics2DRaycastDelegate(ref Vector2 start, ref Vector2 end, out RaycastHit2D hit);

        internal delegate float TimeGetDeltaTimeDelegate();
        internal delegate byte SceneManagerLoadSceneDelegate(IntPtr scenePath);
        internal delegate IntPtr SceneManagerGetActiveScenePathDelegate();
        internal delegate ulong SceneInstantiatePrefabDelegate(IntPtr prefabPath);

        internal delegate ulong EntityGetParentDelegate(ulong entityID);
        internal delegate void EntitySetParentDelegate(ulong childEntityID, ulong parentEntityID, byte keepWorldPosition);
        internal delegate int EntityGetChildCountDelegate(ulong entityID);
        internal delegate ulong EntityGetChildAtDelegate(ulong entityID, int childIndex);

        internal delegate IntPtr UITextGetTextDelegate(ulong entityID);
        internal delegate void UITextSetTextDelegate(ulong entityID, IntPtr text);
        internal delegate void UITextGetColorDelegate(ulong entityID, out Vector4 color);
        internal delegate void UITextSetColorDelegate(ulong entityID, ref Vector4 color);
        internal delegate float UITextGetFontSizeDelegate(ulong entityID);
        internal delegate void UITextSetFontSizeDelegate(ulong entityID, float fontSize);

        internal delegate byte UIButtonGetBoolDelegate(ulong entityID);
        internal delegate void UIButtonSetInteractableDelegate(ulong entityID, byte interactable);

        internal delegate void SoundPlayerVoidDelegate(ulong entityID);
        internal delegate void SoundPlayerPlayOneShotDelegate(ulong entityID, ulong soundHandle);
        internal delegate float SoundPlayerGetFloatDelegate(ulong entityID);
        internal delegate void SoundPlayerSetFloatDelegate(ulong entityID, float value);
        internal delegate byte SoundPlayerGetBoolDelegate(ulong entityID);
        internal delegate void SoundPlayerSetBoolDelegate(ulong entityID, byte value);
        internal delegate ulong SoundPlayerGetHandleDelegate(ulong entityID);
        internal delegate void SoundPlayerSetHandleDelegate(ulong entityID, ulong soundHandle);

        internal delegate ulong FontAssetGetDefaultHandleDelegate();
        internal delegate void FontAssetPreloadCharactersDelegate(ulong handle, IntPtr text);
        internal delegate void FontAssetPreloadTextAsyncDelegate(ulong handle, IntPtr text);
        internal delegate void FontAssetWaitForPendingGenerationsDelegate(ulong handle);

        internal delegate void SpriteRendererGetColorDelegate(ulong entityID, out Vector4 color);
        internal delegate void SpriteRendererSetColorDelegate(ulong entityID, ref Vector4 color);
        internal delegate ulong SpriteRendererGetHandleDelegate(ulong entityID);
        internal delegate void SpriteRendererSetHandleDelegate(ulong entityID, ulong handle);
        internal delegate byte SpriteRendererGetFlipHorizontalDelegate(ulong entityID);
        internal delegate void SpriteRendererSetFlipHorizontalDelegate(ulong entityID, byte flipHorizontal);
        internal delegate int SpriteRendererGetSortingLayerDelegate(ulong entityID);
        internal delegate void SpriteRendererSetSortingLayerDelegate(ulong entityID, int sortingLayer);
        internal delegate int SpriteRendererGetSortingOrderDelegate(ulong entityID);
        internal delegate void SpriteRendererSetSortingOrderDelegate(ulong entityID, int sortingOrder);

        internal delegate float CameraGetOrthographicSizeDelegate(ulong entityID);
        internal delegate void CameraSetOrthographicSizeDelegate(ulong entityID, float orthographicSize);
        internal delegate void CameraGetBackgroundColorDelegate(ulong entityID, out Vector4 color);
        internal delegate void CameraSetBackgroundColorDelegate(ulong entityID, ref Vector4 color);
        internal delegate byte CameraGetPrimaryDelegate(ulong entityID);
        internal delegate void CameraSetPrimaryDelegate(ulong entityID, byte primary);

        internal delegate byte SpriteAnimationGetPlayingDelegate(ulong entityID);
        internal delegate void SpriteAnimationSetPlayingDelegate(ulong entityID, byte playing);
        internal delegate float SpriteAnimationGetFrameRateDelegate(ulong entityID);
        internal delegate void SpriteAnimationSetFrameRateDelegate(ulong entityID, float frameRate);
        internal delegate void SpriteAnimationResetPlaybackDelegate(ulong entityID);
        internal delegate void SpriteAnimationPlayDelegate(ulong entityID, IntPtr animationName);
        internal delegate IntPtr SpriteAnimationGetCurrentAnimationNameDelegate(ulong entityID);
        internal delegate void SpriteAnimationSetCurrentAnimationNameDelegate(ulong entityID, IntPtr animationName);

        // static fields to hold the delegates
        internal static LogFuncDelegate NativeLog;

        internal static EntityHasComponentDelegate Entity_HasComponent;
        internal static SceneCreateEntityDelegate Scene_CreateEntity;
        internal static SceneDestroyEntityDelegate Scene_DestroyEntity;
        internal static SceneFindEntityDelegate Scene_FindEntityByName;

        internal static TransformPosDelegate Transform_GetTranslation;
        internal static TransformSetPosDelegate Transform_SetTranslation;
        internal static TransformPosDelegate Transform_GetRotation;
        internal static TransformSetPosDelegate Transform_SetRotation;
        internal static TransformPosDelegate Transform_GetScale;
        internal static TransformSetPosDelegate Transform_SetScale;

        internal static InputGetKeyDelegate Input_IsKeyDown;
        internal static InputGetMouseBtnDelegate Input_IsMouseButtonDown;
        internal static InputGetMousePosDelegate Input_GetMousePosition;

        internal static Rigidbody2DApplyImpulseDelegate Rigidbody2D_ApplyLinearImpulse;
        internal static Rigidbody2DApplyImpulseCenterDelegate Rigidbody2D_ApplyLinearImpulseToCenter;
        internal static Rigidbody2DGetVelocityDelegate Rigidbody2D_GetLinearVelocity;
        internal static Rigidbody2DSetVelocityDelegate Rigidbody2D_SetLinearVelocity;

        internal static Collider2DGetOffsetDelegate BoxCollider2D_GetOffset;
        internal static Collider2DSetOffsetDelegate BoxCollider2D_SetOffset;
        internal static BoxCollider2DGetSizeDelegate BoxCollider2D_GetSize;
        internal static BoxCollider2DSetSizeDelegate BoxCollider2D_SetSize;
        internal static Collider2DGetFloatDelegate BoxCollider2D_GetDensity;
        internal static Collider2DSetFloatDelegate BoxCollider2D_SetDensity;
        internal static Collider2DGetFloatDelegate BoxCollider2D_GetFriction;
        internal static Collider2DSetFloatDelegate BoxCollider2D_SetFriction;
        internal static Collider2DGetFloatDelegate BoxCollider2D_GetRestitution;
        internal static Collider2DSetFloatDelegate BoxCollider2D_SetRestitution;

        internal static Collider2DGetOffsetDelegate CircleCollider2D_GetOffset;
        internal static Collider2DSetOffsetDelegate CircleCollider2D_SetOffset;
        internal static Collider2DGetFloatDelegate CircleCollider2D_GetRadius;
        internal static Collider2DSetFloatDelegate CircleCollider2D_SetRadius;
        internal static Collider2DGetFloatDelegate CircleCollider2D_GetDensity;
        internal static Collider2DSetFloatDelegate CircleCollider2D_SetDensity;
        internal static Collider2DGetFloatDelegate CircleCollider2D_GetFriction;
        internal static Collider2DSetFloatDelegate CircleCollider2D_SetFriction;
        internal static Collider2DGetFloatDelegate CircleCollider2D_GetRestitution;
        internal static Collider2DSetFloatDelegate CircleCollider2D_SetRestitution;

        internal static TilemapGetSizeDelegate Tilemap_GetSize;
        internal static TilemapSetSizeDelegate Tilemap_SetSize;
        internal static TilemapGetTileDelegate Tilemap_GetTile;
        internal static TilemapSetTileDelegate Tilemap_SetTile;
        internal static TilemapGetBoundsDelegate Tilemap_GetBounds;

        internal static Physics2DRaycastDelegate Physics2D_Raycast;

        internal static TimeGetDeltaTimeDelegate Time_GetDeltaTime;
        internal static SceneManagerLoadSceneDelegate SceneManager_LoadScene;
        internal static SceneManagerGetActiveScenePathDelegate SceneManager_GetActiveScenePath;
        internal static SceneInstantiatePrefabDelegate Scene_InstantiatePrefab;

        internal static EntityGetParentDelegate Entity_GetParent;
        internal static EntitySetParentDelegate Entity_SetParent;
        internal static EntityGetChildCountDelegate Entity_GetChildCount;
        internal static EntityGetChildAtDelegate Entity_GetChildAt;
        internal static TransformPosDelegate Transform_GetWorldTranslation;
        internal static TransformSetPosDelegate Transform_SetWorldTranslation;
        internal static TransformPosDelegate Transform_GetWorldRotation;

        internal static UITextGetTextDelegate UIText_GetText;
        internal static UITextSetTextDelegate UIText_SetText;
        internal static UITextGetColorDelegate UIText_GetColor;
        internal static UITextSetColorDelegate UIText_SetColor;
        internal static UITextGetFontSizeDelegate UIText_GetFontSize;
        internal static UITextSetFontSizeDelegate UIText_SetFontSize;

        internal static UIButtonGetBoolDelegate UIButton_GetInteractable;
        internal static UIButtonSetInteractableDelegate UIButton_SetInteractable;
        internal static UIButtonGetBoolDelegate UIButton_GetIsPointerInside;
        internal static UIButtonGetBoolDelegate UIButton_GetIsPressed;
        internal static UIButtonGetBoolDelegate UIButton_GetWasClickedThisFrame;

        internal static SoundPlayerVoidDelegate SoundPlayer_Play;
        internal static SoundPlayerVoidDelegate SoundPlayer_Stop;
        internal static SoundPlayerVoidDelegate SoundPlayer_Pause;
        internal static SoundPlayerVoidDelegate SoundPlayer_Resume;
        internal static SoundPlayerPlayOneShotDelegate SoundPlayer_PlayOneShot;
        internal static SoundPlayerGetFloatDelegate SoundPlayer_GetVolume;
        internal static SoundPlayerSetFloatDelegate SoundPlayer_SetVolume;
        internal static SoundPlayerGetBoolDelegate SoundPlayer_GetMute;
        internal static SoundPlayerSetBoolDelegate SoundPlayer_SetMute;
        internal static SoundPlayerGetBoolDelegate SoundPlayer_GetLoop;
        internal static SoundPlayerSetBoolDelegate SoundPlayer_SetLoop;
        internal static SoundPlayerGetBoolDelegate SoundPlayer_GetPlayOnStart;
        internal static SoundPlayerSetBoolDelegate SoundPlayer_SetPlayOnStart;
        internal static SoundPlayerGetHandleDelegate SoundPlayer_GetSoundHandle;
        internal static SoundPlayerSetHandleDelegate SoundPlayer_SetSoundHandle;

        internal static FontAssetGetDefaultHandleDelegate FontAsset_GetDefaultHandle;
        internal static FontAssetPreloadCharactersDelegate FontAsset_PreloadCharacters;
        internal static FontAssetPreloadTextAsyncDelegate FontAsset_PreloadTextAsync;
        internal static FontAssetWaitForPendingGenerationsDelegate FontAsset_WaitForPendingGenerations;

        internal static SpriteRendererGetColorDelegate SpriteRenderer_GetColor;
        internal static SpriteRendererSetColorDelegate SpriteRenderer_SetColor;
        internal static SpriteRendererGetHandleDelegate SpriteRenderer_GetSpriteHandle;
        internal static SpriteRendererSetHandleDelegate SpriteRenderer_SetSpriteHandle;
        internal static SpriteRendererGetHandleDelegate SpriteRenderer_GetTextureHandle;
        internal static SpriteRendererSetHandleDelegate SpriteRenderer_SetTextureHandle;
        internal static SpriteRendererGetFlipHorizontalDelegate SpriteRenderer_GetFlipHorizontal;
        internal static SpriteRendererSetFlipHorizontalDelegate SpriteRenderer_SetFlipHorizontal;
        internal static SpriteRendererGetSortingLayerDelegate SpriteRenderer_GetSortingLayer;
        internal static SpriteRendererSetSortingLayerDelegate SpriteRenderer_SetSortingLayer;
        internal static SpriteRendererGetSortingOrderDelegate SpriteRenderer_GetSortingOrder;
        internal static SpriteRendererSetSortingOrderDelegate SpriteRenderer_SetSortingOrder;

        internal static CameraGetOrthographicSizeDelegate Camera_GetOrthographicSize;
        internal static CameraSetOrthographicSizeDelegate Camera_SetOrthographicSize;
        internal static CameraGetBackgroundColorDelegate Camera_GetBackgroundColor;
        internal static CameraSetBackgroundColorDelegate Camera_SetBackgroundColor;
        internal static CameraGetPrimaryDelegate Camera_GetPrimary;
        internal static CameraSetPrimaryDelegate Camera_SetPrimary;

        internal static SpriteAnimationGetPlayingDelegate SpriteAnimation_GetPlaying;
        internal static SpriteAnimationSetPlayingDelegate SpriteAnimation_SetPlaying;
        internal static SpriteAnimationGetFrameRateDelegate SpriteAnimation_GetFrameRate;
        internal static SpriteAnimationSetFrameRateDelegate SpriteAnimation_SetFrameRate;
        internal static SpriteAnimationResetPlaybackDelegate SpriteAnimation_ResetPlayback;
        internal static SpriteAnimationPlayDelegate SpriteAnimation_Play;
        internal static SpriteAnimationGetCurrentAnimationNameDelegate SpriteAnimation_GetCurrentAnimationName;
        internal static SpriteAnimationSetCurrentAnimationNameDelegate SpriteAnimation_SetCurrentAnimationName;

        internal static InputGetKeyDelegate Input_IsKeyPressed;
        internal static InputGetKeyDelegate Input_IsKeyReleased;
        internal static InputGetAxisDelegate Input_GetAxisHorizontal;
        internal static InputGetAxisDelegate Input_GetAxisVertical;

        internal static Rigidbody2DGetBodyTypeDelegate Rigidbody2D_GetBodyType;
        internal static Rigidbody2DSetBodyTypeDelegate Rigidbody2D_SetBodyType;
        internal static Rigidbody2DGetFixedRotationDelegate Rigidbody2D_GetFixedRotation;
        internal static Rigidbody2DSetFixedRotationDelegate Rigidbody2D_SetFixedRotation;

        internal static Collider2DGetBoolDelegate BoxCollider2D_GetIsTrigger;
        internal static Collider2DSetBoolDelegate BoxCollider2D_SetIsTrigger;
        internal static Collider2DGetIntDelegate BoxCollider2D_GetLayer;
        internal static Collider2DSetIntDelegate BoxCollider2D_SetLayer;

        internal static Collider2DGetBoolDelegate CircleCollider2D_GetIsTrigger;
        internal static Collider2DSetBoolDelegate CircleCollider2D_SetIsTrigger;
        internal static Collider2DGetIntDelegate CircleCollider2D_GetLayer;
        internal static Collider2DSetIntDelegate CircleCollider2D_SetLayer;

        [UnmanagedCallersOnly]
        public static void Initialize(IntPtr functionTablePtr)
        {
            var funcs = Marshal.PtrToStructure<NativeFunctionsMap>(functionTablePtr);

            NativeLog = Marshal.GetDelegateForFunctionPointer<LogFuncDelegate>(funcs.LogFunc);

            Entity_HasComponent = Marshal.GetDelegateForFunctionPointer<EntityHasComponentDelegate>(funcs.Entity_HasComponent);
            Scene_CreateEntity = Marshal.GetDelegateForFunctionPointer<SceneCreateEntityDelegate>(funcs.Scene_CreateEntity);
            Scene_DestroyEntity = Marshal.GetDelegateForFunctionPointer<SceneDestroyEntityDelegate>(funcs.Scene_DestroyEntity);
            Scene_FindEntityByName = Marshal.GetDelegateForFunctionPointer<SceneFindEntityDelegate>(funcs.Scene_FindEntityByName);

            Transform_GetTranslation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetTranslation);
            Transform_SetTranslation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetTranslation);
            Transform_GetRotation = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetRotation);
            Transform_SetRotation = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetRotation);
            Transform_GetScale = Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetScale);
            Transform_SetScale = Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetScale);

            Input_IsKeyDown = Marshal.GetDelegateForFunctionPointer<InputGetKeyDelegate>(funcs.Input_IsKeyDown);
            Input_IsMouseButtonDown = Marshal.GetDelegateForFunctionPointer<InputGetMouseBtnDelegate>(funcs.Input_IsMouseButtonDown);
            Input_GetMousePosition = Marshal.GetDelegateForFunctionPointer<InputGetMousePosDelegate>(funcs.Input_GetMousePosition);

            Rigidbody2D_ApplyLinearImpulse = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseDelegate>(funcs.Rigidbody2D_ApplyLinearImpulse);
            Rigidbody2D_ApplyLinearImpulseToCenter = Marshal.GetDelegateForFunctionPointer<Rigidbody2DApplyImpulseCenterDelegate>(funcs.Rigidbody2D_ApplyLinearImpulseToCenter);
            Rigidbody2D_GetLinearVelocity = Marshal.GetDelegateForFunctionPointer<Rigidbody2DGetVelocityDelegate>(funcs.Rigidbody2D_GetLinearVelocity);
            Rigidbody2D_SetLinearVelocity = Marshal.GetDelegateForFunctionPointer<Rigidbody2DSetVelocityDelegate>(funcs.Rigidbody2D_SetLinearVelocity);

            BoxCollider2D_GetOffset = Marshal.GetDelegateForFunctionPointer<Collider2DGetOffsetDelegate>(funcs.BoxCollider2D_GetOffset);
            BoxCollider2D_SetOffset = Marshal.GetDelegateForFunctionPointer<Collider2DSetOffsetDelegate>(funcs.BoxCollider2D_SetOffset);
            BoxCollider2D_GetSize = Marshal.GetDelegateForFunctionPointer<BoxCollider2DGetSizeDelegate>(funcs.BoxCollider2D_GetSize);
            BoxCollider2D_SetSize = Marshal.GetDelegateForFunctionPointer<BoxCollider2DSetSizeDelegate>(funcs.BoxCollider2D_SetSize);
            BoxCollider2D_GetDensity = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.BoxCollider2D_GetDensity);
            BoxCollider2D_SetDensity = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.BoxCollider2D_SetDensity);
            BoxCollider2D_GetFriction = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.BoxCollider2D_GetFriction);
            BoxCollider2D_SetFriction = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.BoxCollider2D_SetFriction);
            BoxCollider2D_GetRestitution = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.BoxCollider2D_GetRestitution);
            BoxCollider2D_SetRestitution = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.BoxCollider2D_SetRestitution);

            CircleCollider2D_GetOffset = Marshal.GetDelegateForFunctionPointer<Collider2DGetOffsetDelegate>(funcs.CircleCollider2D_GetOffset);
            CircleCollider2D_SetOffset = Marshal.GetDelegateForFunctionPointer<Collider2DSetOffsetDelegate>(funcs.CircleCollider2D_SetOffset);
            CircleCollider2D_GetRadius = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.CircleCollider2D_GetRadius);
            CircleCollider2D_SetRadius = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.CircleCollider2D_SetRadius);
            CircleCollider2D_GetDensity = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.CircleCollider2D_GetDensity);
            CircleCollider2D_SetDensity = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.CircleCollider2D_SetDensity);
            CircleCollider2D_GetFriction = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.CircleCollider2D_GetFriction);
            CircleCollider2D_SetFriction = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.CircleCollider2D_SetFriction);
            CircleCollider2D_GetRestitution = Marshal.GetDelegateForFunctionPointer<Collider2DGetFloatDelegate>(funcs.CircleCollider2D_GetRestitution);
            CircleCollider2D_SetRestitution = Marshal.GetDelegateForFunctionPointer<Collider2DSetFloatDelegate>(funcs.CircleCollider2D_SetRestitution);

            Tilemap_GetSize = Marshal.GetDelegateForFunctionPointer<TilemapGetSizeDelegate>(funcs.Tilemap_GetSize);
            Tilemap_SetSize = Marshal.GetDelegateForFunctionPointer<TilemapSetSizeDelegate>(funcs.Tilemap_SetSize);
            Tilemap_GetTile = Marshal.GetDelegateForFunctionPointer<TilemapGetTileDelegate>(funcs.Tilemap_GetTile);
            Tilemap_SetTile = Marshal.GetDelegateForFunctionPointer<TilemapSetTileDelegate>(funcs.Tilemap_SetTile);
            Tilemap_GetBounds =
                Marshal.GetDelegateForFunctionPointer<TilemapGetBoundsDelegate>(funcs.Tilemap_GetBounds);

            Physics2D_Raycast = Marshal.GetDelegateForFunctionPointer<Physics2DRaycastDelegate>(funcs.Physics2D_Raycast);

            Time_GetDeltaTime = Marshal.GetDelegateForFunctionPointer<TimeGetDeltaTimeDelegate>(funcs.Time_GetDeltaTime);
            SceneManager_LoadScene =
                Marshal.GetDelegateForFunctionPointer<SceneManagerLoadSceneDelegate>(funcs.SceneManager_LoadScene);
            SceneManager_GetActiveScenePath =
                Marshal.GetDelegateForFunctionPointer<SceneManagerGetActiveScenePathDelegate>(
                    funcs.SceneManager_GetActiveScenePath);
            Scene_InstantiatePrefab =
                Marshal.GetDelegateForFunctionPointer<SceneInstantiatePrefabDelegate>(funcs.Scene_InstantiatePrefab);

            Entity_GetParent =
                Marshal.GetDelegateForFunctionPointer<EntityGetParentDelegate>(funcs.Entity_GetParent);
            Entity_SetParent =
                Marshal.GetDelegateForFunctionPointer<EntitySetParentDelegate>(funcs.Entity_SetParent);
            Entity_GetChildCount =
                Marshal.GetDelegateForFunctionPointer<EntityGetChildCountDelegate>(funcs.Entity_GetChildCount);
            Entity_GetChildAt =
                Marshal.GetDelegateForFunctionPointer<EntityGetChildAtDelegate>(funcs.Entity_GetChildAt);
            Transform_GetWorldTranslation =
                Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetWorldTranslation);
            Transform_SetWorldTranslation =
                Marshal.GetDelegateForFunctionPointer<TransformSetPosDelegate>(funcs.Transform_SetWorldTranslation);
            Transform_GetWorldRotation =
                Marshal.GetDelegateForFunctionPointer<TransformPosDelegate>(funcs.Transform_GetWorldRotation);

            UIText_GetText =
                Marshal.GetDelegateForFunctionPointer<UITextGetTextDelegate>(funcs.UIText_GetText);
            UIText_SetText =
                Marshal.GetDelegateForFunctionPointer<UITextSetTextDelegate>(funcs.UIText_SetText);
            UIText_GetColor =
                Marshal.GetDelegateForFunctionPointer<UITextGetColorDelegate>(funcs.UIText_GetColor);
            UIText_SetColor =
                Marshal.GetDelegateForFunctionPointer<UITextSetColorDelegate>(funcs.UIText_SetColor);
            UIText_GetFontSize =
                Marshal.GetDelegateForFunctionPointer<UITextGetFontSizeDelegate>(funcs.UIText_GetFontSize);
            UIText_SetFontSize =
                Marshal.GetDelegateForFunctionPointer<UITextSetFontSizeDelegate>(funcs.UIText_SetFontSize);

            UIButton_GetInteractable =
                Marshal.GetDelegateForFunctionPointer<UIButtonGetBoolDelegate>(funcs.UIButton_GetInteractable);
            UIButton_SetInteractable =
                Marshal.GetDelegateForFunctionPointer<UIButtonSetInteractableDelegate>(
                    funcs.UIButton_SetInteractable);
            UIButton_GetIsPointerInside =
                Marshal.GetDelegateForFunctionPointer<UIButtonGetBoolDelegate>(funcs.UIButton_GetIsPointerInside);
            UIButton_GetIsPressed =
                Marshal.GetDelegateForFunctionPointer<UIButtonGetBoolDelegate>(funcs.UIButton_GetIsPressed);
            UIButton_GetWasClickedThisFrame =
                Marshal.GetDelegateForFunctionPointer<UIButtonGetBoolDelegate>(
                    funcs.UIButton_GetWasClickedThisFrame);

            SoundPlayer_Play =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerVoidDelegate>(funcs.SoundPlayer_Play);
            SoundPlayer_Stop =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerVoidDelegate>(funcs.SoundPlayer_Stop);
            SoundPlayer_Pause =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerVoidDelegate>(funcs.SoundPlayer_Pause);
            SoundPlayer_Resume =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerVoidDelegate>(funcs.SoundPlayer_Resume);
            SoundPlayer_PlayOneShot =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerPlayOneShotDelegate>(
                    funcs.SoundPlayer_PlayOneShot);
            SoundPlayer_GetVolume =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerGetFloatDelegate>(funcs.SoundPlayer_GetVolume);
            SoundPlayer_SetVolume =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerSetFloatDelegate>(funcs.SoundPlayer_SetVolume);
            SoundPlayer_GetMute =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerGetBoolDelegate>(funcs.SoundPlayer_GetMute);
            SoundPlayer_SetMute =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerSetBoolDelegate>(funcs.SoundPlayer_SetMute);
            SoundPlayer_GetLoop =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerGetBoolDelegate>(funcs.SoundPlayer_GetLoop);
            SoundPlayer_SetLoop =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerSetBoolDelegate>(funcs.SoundPlayer_SetLoop);
            SoundPlayer_GetPlayOnStart =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerGetBoolDelegate>(
                    funcs.SoundPlayer_GetPlayOnStart);
            SoundPlayer_SetPlayOnStart =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerSetBoolDelegate>(
                    funcs.SoundPlayer_SetPlayOnStart);
            SoundPlayer_GetSoundHandle =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerGetHandleDelegate>(
                    funcs.SoundPlayer_GetSoundHandle);
            SoundPlayer_SetSoundHandle =
                Marshal.GetDelegateForFunctionPointer<SoundPlayerSetHandleDelegate>(
                    funcs.SoundPlayer_SetSoundHandle);

            FontAsset_GetDefaultHandle =
                Marshal.GetDelegateForFunctionPointer<FontAssetGetDefaultHandleDelegate>(
                    funcs.FontAsset_GetDefaultHandle);
            FontAsset_PreloadCharacters =
                Marshal.GetDelegateForFunctionPointer<FontAssetPreloadCharactersDelegate>(
                    funcs.FontAsset_PreloadCharacters);
            FontAsset_PreloadTextAsync =
                Marshal.GetDelegateForFunctionPointer<FontAssetPreloadTextAsyncDelegate>(
                    funcs.FontAsset_PreloadTextAsync);
            FontAsset_WaitForPendingGenerations =
                Marshal.GetDelegateForFunctionPointer<FontAssetWaitForPendingGenerationsDelegate>(
                    funcs.FontAsset_WaitForPendingGenerations);

            SpriteRenderer_GetColor =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetColorDelegate>(funcs.SpriteRenderer_GetColor);
            SpriteRenderer_SetColor =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetColorDelegate>(funcs.SpriteRenderer_SetColor);
            SpriteRenderer_GetSpriteHandle =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetHandleDelegate>(funcs.SpriteRenderer_GetSpriteHandle);
            SpriteRenderer_SetSpriteHandle =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetHandleDelegate>(funcs.SpriteRenderer_SetSpriteHandle);
            SpriteRenderer_GetTextureHandle =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetHandleDelegate>(funcs.SpriteRenderer_GetTextureHandle);
            SpriteRenderer_SetTextureHandle =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetHandleDelegate>(funcs.SpriteRenderer_SetTextureHandle);
            SpriteRenderer_GetFlipHorizontal =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetFlipHorizontalDelegate>(
                    funcs.SpriteRenderer_GetFlipHorizontal);
            SpriteRenderer_SetFlipHorizontal =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetFlipHorizontalDelegate>(
                    funcs.SpriteRenderer_SetFlipHorizontal);

            SpriteRenderer_GetSortingLayer =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetSortingLayerDelegate>(
                    funcs.SpriteRenderer_GetSortingLayer);
            SpriteRenderer_SetSortingLayer =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetSortingLayerDelegate>(
                    funcs.SpriteRenderer_SetSortingLayer);
            SpriteRenderer_GetSortingOrder =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererGetSortingOrderDelegate>(
                    funcs.SpriteRenderer_GetSortingOrder);
            SpriteRenderer_SetSortingOrder =
                Marshal.GetDelegateForFunctionPointer<SpriteRendererSetSortingOrderDelegate>(
                    funcs.SpriteRenderer_SetSortingOrder);

            Camera_GetOrthographicSize =
                Marshal.GetDelegateForFunctionPointer<CameraGetOrthographicSizeDelegate>(
                    funcs.Camera_GetOrthographicSize);
            Camera_SetOrthographicSize =
                Marshal.GetDelegateForFunctionPointer<CameraSetOrthographicSizeDelegate>(
                    funcs.Camera_SetOrthographicSize);
            Camera_GetBackgroundColor =
                Marshal.GetDelegateForFunctionPointer<CameraGetBackgroundColorDelegate>(
                    funcs.Camera_GetBackgroundColor);
            Camera_SetBackgroundColor =
                Marshal.GetDelegateForFunctionPointer<CameraSetBackgroundColorDelegate>(
                    funcs.Camera_SetBackgroundColor);
            Camera_GetPrimary =
                Marshal.GetDelegateForFunctionPointer<CameraGetPrimaryDelegate>(funcs.Camera_GetPrimary);
            Camera_SetPrimary =
                Marshal.GetDelegateForFunctionPointer<CameraSetPrimaryDelegate>(funcs.Camera_SetPrimary);

            SpriteAnimation_GetPlaying =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationGetPlayingDelegate>(funcs.SpriteAnimation_GetPlaying);
            SpriteAnimation_SetPlaying =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationSetPlayingDelegate>(funcs.SpriteAnimation_SetPlaying);
            SpriteAnimation_GetFrameRate =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationGetFrameRateDelegate>(funcs.SpriteAnimation_GetFrameRate);
            SpriteAnimation_SetFrameRate =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationSetFrameRateDelegate>(funcs.SpriteAnimation_SetFrameRate);
            SpriteAnimation_ResetPlayback =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationResetPlaybackDelegate>(funcs.SpriteAnimation_ResetPlayback);
            SpriteAnimation_Play =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationPlayDelegate>(funcs.SpriteAnimation_Play);
            SpriteAnimation_GetCurrentAnimationName =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationGetCurrentAnimationNameDelegate>(
                    funcs.SpriteAnimation_GetCurrentAnimationName);
            SpriteAnimation_SetCurrentAnimationName =
                Marshal.GetDelegateForFunctionPointer<SpriteAnimationSetCurrentAnimationNameDelegate>(
                    funcs.SpriteAnimation_SetCurrentAnimationName);

            Input_IsKeyPressed = Marshal.GetDelegateForFunctionPointer<InputGetKeyDelegate>(funcs.Input_IsKeyPressed);
            Input_IsKeyReleased = Marshal.GetDelegateForFunctionPointer<InputGetKeyDelegate>(funcs.Input_IsKeyReleased);
            Input_GetAxisHorizontal = Marshal.GetDelegateForFunctionPointer<InputGetAxisDelegate>(funcs.Input_GetAxisHorizontal);
            Input_GetAxisVertical = Marshal.GetDelegateForFunctionPointer<InputGetAxisDelegate>(funcs.Input_GetAxisVertical);

            Rigidbody2D_GetBodyType = Marshal.GetDelegateForFunctionPointer<Rigidbody2DGetBodyTypeDelegate>(funcs.Rigidbody2D_GetBodyType);
            Rigidbody2D_SetBodyType = Marshal.GetDelegateForFunctionPointer<Rigidbody2DSetBodyTypeDelegate>(funcs.Rigidbody2D_SetBodyType);
            Rigidbody2D_GetFixedRotation = Marshal.GetDelegateForFunctionPointer<Rigidbody2DGetFixedRotationDelegate>(funcs.Rigidbody2D_GetFixedRotation);
            Rigidbody2D_SetFixedRotation = Marshal.GetDelegateForFunctionPointer<Rigidbody2DSetFixedRotationDelegate>(funcs.Rigidbody2D_SetFixedRotation);

            BoxCollider2D_GetIsTrigger = Marshal.GetDelegateForFunctionPointer<Collider2DGetBoolDelegate>(funcs.BoxCollider2D_GetIsTrigger);
            BoxCollider2D_SetIsTrigger = Marshal.GetDelegateForFunctionPointer<Collider2DSetBoolDelegate>(funcs.BoxCollider2D_SetIsTrigger);
            BoxCollider2D_GetLayer = Marshal.GetDelegateForFunctionPointer<Collider2DGetIntDelegate>(funcs.BoxCollider2D_GetLayer);
            BoxCollider2D_SetLayer = Marshal.GetDelegateForFunctionPointer<Collider2DSetIntDelegate>(funcs.BoxCollider2D_SetLayer);

            CircleCollider2D_GetIsTrigger = Marshal.GetDelegateForFunctionPointer<Collider2DGetBoolDelegate>(funcs.CircleCollider2D_GetIsTrigger);
            CircleCollider2D_SetIsTrigger = Marshal.GetDelegateForFunctionPointer<Collider2DSetBoolDelegate>(funcs.CircleCollider2D_SetIsTrigger);
            CircleCollider2D_GetLayer = Marshal.GetDelegateForFunctionPointer<Collider2DGetIntDelegate>(funcs.CircleCollider2D_GetLayer);
            CircleCollider2D_SetLayer = Marshal.GetDelegateForFunctionPointer<Collider2DSetIntDelegate>(funcs.CircleCollider2D_SetLayer);

            Console.WriteLine("[C#] InternalCalls initialized.");
        }
    }
}