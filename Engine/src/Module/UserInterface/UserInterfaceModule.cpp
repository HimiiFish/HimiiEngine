#include "Hepch.h"
#include "Module/UserInterface/UserInterfaceModule.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void UserInterfaceModule::OnUpdate(Timestep timestep)
    {
        (void)timestep;
        if (m_Scene)
            m_Scene->ProcessUserInterfacePointer();
    }
}
