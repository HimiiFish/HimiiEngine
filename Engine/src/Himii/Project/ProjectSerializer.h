#pragma once

#include "Project.h"

namespace Himii
{

    class ProjectSerializer {
    public:
        ProjectSerializer(Ref<Project> project);

        /// @param omitScriptIDEFields 发布模式：不写入 ScriptIDE / CustomScriptIDE*（开发存盘保持默认 false）。
        bool Serialize(const std::filesystem::path &filepath, bool omitScriptIDEFields = false);
        bool Deserialize(const std::filesystem::path &filepath);

    private:
        Ref<Project> m_Project;
    };

}
