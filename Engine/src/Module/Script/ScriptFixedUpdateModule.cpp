#include "Hepch.h"
#include "Module/Script/ScriptFixedUpdateModule.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void ScriptFixedUpdateModule::OnUpdate(Timestep timestep)
    {
        if (m_Scene)
            m_Scene->RunScriptFixedUpdate(timestep);
    }
}
