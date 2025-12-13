#pragma once
#include "glm/glm.hpp"

namespace Himii
{

    struct ScriptEngineData {
        void *LogFunc;
        void *Entity_HasComponent;

        // Transform
        void *Transform_GetTranslation;
        void *Transform_SetTranslation;

        // Input
        void *Input_IsKeyDown;

        // Rigidbody2D
        void *Rigidbody2D_ApplyLinearImpulse;
        void *Rigidbody2D_ApplyLinearImpulseToCenter;
    };

    class ScriptGlue {
    public:
        static void RegisterFunctions();
        static ScriptEngineData GetNativeFunctions();
    };

}
