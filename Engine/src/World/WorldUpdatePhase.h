#pragma once

#include <cstdint>

namespace Himii
{
    enum class WorldUpdatePhase : uint32_t
    {
        UserInterface = 0,
        ScriptUpdate,
        Animation,
        Physics,
        ScriptFixedUpdate,
        Presentation,
        Render,

        Count
    };

    inline const char *GetWorldUpdatePhaseName(WorldUpdatePhase phase)
    {
        switch (phase)
        {
            case WorldUpdatePhase::UserInterface: return "UserInterface";
            case WorldUpdatePhase::ScriptUpdate: return "ScriptUpdate";
            case WorldUpdatePhase::Animation: return "Animation";
            case WorldUpdatePhase::Physics: return "Physics";
            case WorldUpdatePhase::ScriptFixedUpdate: return "ScriptFixedUpdate";
            case WorldUpdatePhase::Presentation: return "Presentation";
            case WorldUpdatePhase::Render: return "Render";
            default: return "Unknown";
        }
    }
}
