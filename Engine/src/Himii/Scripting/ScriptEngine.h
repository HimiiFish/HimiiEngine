#pragma once

#include <filesystem>
#include <string>
#include <map>

#include "Himii/Scene/Entity.h"
#include "Himii/Scene/Scene.h"
#include "Himii/Core/Timestep.h"

namespace Himii {

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& filepath);
		static void CompileAndReloadAppAssembly(const std::filesystem::path& projectPath);
		static void LoadAppAssembly(const std::filesystem::path& filepath);

		// 运行时生命周期
        static void OnRuntimeStart(Scene* scene);
        static void OnRuntimeStop();

		static void OnUpdateScript(Entity entity, Timestep ts);

		static bool EntityClassExists(const std::string &fullClassName);

		static Scene* GetSceneContext();

    private:
        static Scene* s_SceneContext;
	};


}
