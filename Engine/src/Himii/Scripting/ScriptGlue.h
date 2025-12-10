#pragma once
#include "glm/glm.hpp"

namespace Himii
{

    struct ScriptEngineData
    {
        void *LogFunc;
        void *Entity_HasComponent;
        
        void *Transform_GetTranslation;
        void *Transform_SetTranslation;
    };

    class ScriptGlue {
    public:
        static void RegisterFunctions();
        static ScriptEngineData GetNativeFunctions();
    };

}
