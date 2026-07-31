#include "Hepch.h"
#include "Module/Script/ScriptUpdateModule.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void ScriptUpdateModule::OnUpdate(Timestep timestep)
    {
        if (m_Scene)
            m_Scene->UpdateManagedAndNativeScripts(timestep);
    }
}
