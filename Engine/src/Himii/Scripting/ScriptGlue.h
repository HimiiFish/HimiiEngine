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
    };

    class ScriptGlue {
    public:
        //static void RegisterFunctions();
        static ScriptEngineData GetNativeFunctions();
    };

}
