#pragma once

#include "Module/Script/ScriptIDE.h"
#include "Project/Physics2DLayerSettings.h"
#include "Project/SortingLayerSettings.h"

namespace Himii
{
    class ProjectSettingsPanel
    {
    public:
        void OnImGuiRender(bool* pOpen);
        void Reset() { m_Initialized = false; }

    private:
        ScriptIDEType m_TempIDEOverride = ScriptIDEType::Inherit;
        char m_TempCustomPath[512] = "";
        char m_TempCustomArgs[512] = "";
        Physics2DLayerSettings m_TempPhysics2DLayers;
        SortingLayerSettings m_TempSortingLayers;
        bool m_Initialized = false;

        void SyncFromProject();
        void ApplyToProject();
    };
}
