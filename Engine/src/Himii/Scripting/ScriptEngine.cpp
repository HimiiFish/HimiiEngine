#include "ScriptEngine.h"
#include "ScriptGlue.h"
#include "Himii/Project/Project.h"

#include <iostream>
#include <filesystem>
#include <vector>

// 尝试包含 nethost 和 hostfxr 头文件
// 如果编译报错找不到这些头文件，需要确保 vcpkg 安装了它们，或者手动复制到项目目录
#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>

#ifdef WIN32
#include <Windows.h>
#define STR(s) L##s
#define CH(c) L##c
#define DIR_SEPARATOR L'\\'
#else
#include <dlfcn.h>
#include <limits.h>
#define STR(s) s
#define CH(c) c
#define DIR_SEPARATOR '/'
#define MAX_PATH PATH_MAX
#endif

namespace Himii {

	// 全局变量保存 hostfxr 的库句柄和函数指针
	static hostfxr_initialize_for_runtime_config_fn init_fptr;
	static hostfxr_get_runtime_delegate_fn get_delegate_fptr;
	static hostfxr_close_fn close_fptr;
	static load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
	static hostfxr_handle cxt = nullptr;

	Scene* ScriptEngine::s_SceneContext = nullptr;

	typedef void(CORECLR_DELEGATE_CALLTYPE *OnCreateFn)(uint64_t entityID, const char *className);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnUpdateFn)(uint64_t entityID, float ts);
    typedef void(CORECLR_DELEGATE_CALLTYPE *LoadAssemblyFn)(const char *filepath);
    typedef bool(CORECLR_DELEGATE_CALLTYPE *ClassExistsFn)(const char *className);

    static LoadAssemblyFn s_LoadGameAssembly = nullptr;
    static ClassExistsFn s_EntityClassExists = nullptr;
    static OnCreateFn s_OnCreate = nullptr;
    static OnUpdateFn s_OnUpdate = nullptr;

	// 加载 hostfxr 库
	static bool LoadHostFxr()
	{
		// 1. 获取 hostfxr 路径
		char_t buffer[MAX_PATH];
		size_t buffer_size = sizeof(buffer) / sizeof(char_t);
		int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
		if (rc != 0)
			return false;

		// 2. 加载库
#ifdef WIN32
		HMODULE lib = LoadLibraryW(buffer);
		if (!lib) return false;
		init_fptr = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(lib, "hostfxr_initialize_for_runtime_config");
		get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GetProcAddress(lib, "hostfxr_get_runtime_delegate");
		close_fptr = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");
#else
		void* lib = dlopen(buffer, RTLD_LAZY);
		if (!lib) return false;
		init_fptr = (hostfxr_initialize_for_runtime_config_fn)dlsym(lib, "hostfxr_initialize_for_runtime_config");
		get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)dlsym(lib, "hostfxr_get_runtime_delegate");
		close_fptr = (hostfxr_close_fn)dlsym(lib, "hostfxr_close");
#endif

		return (init_fptr && get_delegate_fptr && close_fptr);
	}

	// 辅助转换 wide string 到 std::string (仅用于日志)
    static std::string ToString(const char_t* str) {
#ifdef WIN32
        if (!str) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        std::string res(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, str, -1, &res[0], size_needed, NULL, NULL);
        return res;
#else
        return std::string(str);
#endif
    }

	void ScriptEngine::Init()
	{
		// 1. 加载 HostFxr
		if (!LoadHostFxr())
		{
			std::cerr << "[ScriptEngine] Failed to load hostfxr!" << std::endl;
			return;
		}

		// 2. 确定配置文件的路径
		// 假设 ScriptCore.runtimeconfig.json 与可执行文件在同一位置
		// 实际项目中可能需要更复杂的路径处理
		std::filesystem::path runtimeConfigPath = std::filesystem::current_path() / "ScriptCore.runtimeconfig.json";
		
		if (!std::filesystem::exists(runtimeConfigPath))
		{
			std::cerr << "[ScriptEngine] Runtime config not found at: " << runtimeConfigPath << std::endl;
			return;
		}

		// 3. 初始化 .NET Runtime
		int rc = init_fptr(runtimeConfigPath.c_str(), nullptr, &cxt);
		if (rc != 0 || cxt == nullptr)
		{
			std::cerr << "[ScriptEngine] Init failed: " << std::hex << rc << std::endl;
			close_fptr(cxt);
			return;
		}

		// 4. 获取加载程序集的委托
		rc = get_delegate_fptr(
			cxt,
			hdt_load_assembly_and_get_function_pointer,
			(void**)&load_assembly_and_get_function_pointer);

		if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
		{
			std::cerr << "[ScriptEngine] Get delegate failed: " << std::hex << rc << std::endl;
			close_fptr(cxt);
			return;
		}

		// 5. 加载 ScriptCore 程序集并进行初始化
		std::filesystem::path assemblyPath = std::filesystem::current_path() / "ScriptCore.dll";
		LoadAssembly(assemblyPath);
	}

	void ScriptEngine::Shutdown()
	{
		if (cxt)
		{
			close_fptr(cxt);
			cxt = nullptr;
		}
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		if (!load_assembly_and_get_function_pointer) return;

		// 定义初始化函数的签名
		typedef void (CORECLR_DELEGATE_CALLTYPE *InteropInitFn)(void* functionTable);
		InteropInitFn interopInit = nullptr;

		// 6. 调用 C# 的 Interop.Initialize
		// 格式: Namespace.Class, AssemblyName
		const char_t* dotnet_type = STR("Himii.InternalCalls, ScriptCore");
		const char_t* dotnet_type_method = STR("Initialize");

		int rc = load_assembly_and_get_function_pointer(
			filepath.c_str(),
			dotnet_type,
			dotnet_type_method,
			UNMANAGEDCALLERSONLY_METHOD, // 指示这是一个 [UnmanagedCallersOnly] 方法
			nullptr,
			(void**)&interopInit);

		if (rc == 0 || interopInit)
		{
            auto nativeFunctions = ScriptGlue::GetNativeFunctions();
            interopInit((void *)&nativeFunctions);
            std::cout << "[ScriptEngine] Successfully loaded ScriptCore! Initializing Interop..." << std::endl;
		}

		// 2. 获取 C# 的生命周期函数 (OnCreate, OnUpdate)
        // 假设我们在 C# Himii.ScriptManager 类中定义这些静态方法
        load_assembly_and_get_function_pointer(filepath.c_str(), STR("Himii.ScriptManager, ScriptCore"),
                                               STR("LoadGameAssembly"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_LoadGameAssembly);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("Himii.ScriptManager, ScriptCore"),
                                               STR("EntityClassExists"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_EntityClassExists);

        load_assembly_and_get_function_pointer(
            filepath.c_str(),
            STR("Himii.ScriptManager, ScriptCore"),
            STR("OnCreateEntity"),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            (void**)&s_OnCreate);

        load_assembly_and_get_function_pointer(
            filepath.c_str(),
            STR("Himii.ScriptManager, ScriptCore"),
            STR("OnUpdateEntity"),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            (void**)&s_OnUpdate);


    }

    void ScriptEngine::CompileAndReloadAppAssembly(const std::filesystem::path &projectPath)
    {
        std::string command = "dotnet build \"" + projectPath.string() + "\" -c Debug";
        int result = std::system(command.c_str());

        if (result == 0)
        {
            std::filesystem::path dllPath = Project::GetProjectDirectory() / Project::GetConfig().ScriptModulePath;

            if (std::filesystem::exists(dllPath))
            {
                LoadAppAssembly(dllPath);
            }
            else
            {
                // 如果找不到，尝试构建完整路径（调试用）
                // 假设 exe 在 .../Debug/

                std::filesystem::path debugPath = Project::GetProjectDirectory() / Project::GetConfig().ScriptModulePath;
                if (std::filesystem::exists(debugPath))
                {
                    LoadAppAssembly(debugPath);
                }
                else
                {
                    std::cerr << "[ScriptEngine] DLL not found! Expected at: " << std::filesystem::absolute(dllPath)
                              << std::endl;
                    // 打印一下当前目录，帮你定位问题
                    std::cerr << "Current Dir: " << std::filesystem::current_path() << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "[ScriptEngine] Failed to compile C# project!" << std::endl;
        }
    }

    void ScriptEngine::LoadAppAssembly(const std::filesystem::path &filepath)
    {
        if (s_LoadGameAssembly)
        {
            // 传递 DLL 路径给 C#
            s_LoadGameAssembly(filepath.string().c_str());
        }
    }

    void ScriptEngine::OnCreateEntity(Entity entity)
    {
        const auto &sc = entity.GetComponent<ScriptComponent>();
        if (ScriptEngine::EntityClassExists(sc.ClassName))
        {
            UUID entityID = entity.GetUUID();
            // 调用 C# 的 OnCreateEntity，传入 ID 和类名
            if (s_OnCreate)
                s_OnCreate(entityID, sc.ClassName.c_str());
        }
    }

    void ScriptEngine::OnRuntimeStart(Scene *scene)
    {
        s_SceneContext = scene;
    }

    void ScriptEngine::OnRuntimeStop()
    {
    }

    void ScriptEngine::OnUpdateScript(Entity entity, Timestep ts)
    {
        if (s_OnUpdate && entity.HasComponent<ScriptComponent>())
        {
            // 调用 C# 的 Update
            s_OnUpdate(entity.GetUUID(), ts.GetSeconds());
        }
    }

    bool ScriptEngine::EntityClassExists(const std::string &fullClassName)
    {
        if (fullClassName.empty())
            return false;

        return true; 
    }

    Scene* ScriptEngine::GetSceneContext()
    {
        return s_SceneContext;
    }

}