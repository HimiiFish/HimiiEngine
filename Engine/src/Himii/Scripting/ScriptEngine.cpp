#include "Hepch.h"
#include "ScriptEngine.h"
#include "ScriptGlue.h"
#include "ScriptCompiler.h"
#include "Himii/Project/Project.h"
#include "Himii/Core/Application.h"
#include "Himii/Scene/Components.h"
#include "Himii/Scene/Entity.h"

#include <iostream>
#include <filesystem>
#include <vector>

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

namespace Himii
{
    static hostfxr_initialize_for_runtime_config_fn init_fptr;
    static hostfxr_get_runtime_delegate_fn get_delegate_fptr;
    static hostfxr_close_fn close_fptr;
    static load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
    static hostfxr_handle cxt = nullptr;

    Scene *ScriptEngine::s_SceneContext = nullptr;
    std::string ScriptEngine::s_ActiveSceneRelativePath;
    static float s_ScriptDeltaTime = 0.0f;
    std::unordered_map<UUID, void *> ScriptEngine::s_EntityInstanceMap;

    typedef void(CORECLR_DELEGATE_CALLTYPE *LoadAssemblyFn)(const char *filepath);
    typedef int(CORECLR_DELEGATE_CALLTYPE *ClassExistsFn)(const char *className);

    typedef void *(CORECLR_DELEGATE_CALLTYPE *InstantiateClassFn)(uint64_t entityID, const char *className, int flags);
    typedef void(CORECLR_DELEGATE_CALLTYPE *ReleaseInstanceFn)(void *handle);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnCreateInstanceFn)(void *handle);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnDestroyInstanceFn)(void *handle);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnUpdateInstanceFn)(void *handle, float ts);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnFixedUpdateInstanceFn)(void *handle, float ts);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnCollisionEnter2DInstanceFn)(void *handle, Collision2DInterop collision);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnCollisionExit2DInstanceFn)(void *handle, uint64_t otherEntityID);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnTriggerEnter2DInstanceFn)(void *handle, Collision2DInterop collision);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnTriggerExit2DInstanceFn)(void *handle, uint64_t otherEntityID);
    typedef void(CORECLR_DELEGATE_CALLTYPE *OnPointerEventInstanceFn)(void *handle, int eventType);

    typedef const char *(CORECLR_DELEGATE_CALLTYPE *GetFieldsFn)(void *handle);
    typedef int(CORECLR_DELEGATE_CALLTYPE *GetFloatFn)(void *handle, const char *name, float *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetFloatFn)(void *handle, const char *name, float val);

    typedef int(CORECLR_DELEGATE_CALLTYPE *GetIntFn)(void *handle, const char *name, int *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetIntFn)(void *handle, const char *name, int val);

    typedef int(CORECLR_DELEGATE_CALLTYPE *GetBoolFn)(void *handle, const char *name, int *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetBoolFn)(void *handle, const char *name, int val);

    typedef int(CORECLR_DELEGATE_CALLTYPE *GetVec2Fn)(void *handle, const char *name, glm::vec2 *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetVec2Fn)(void *handle, const char *name, glm::vec2 *val);
    typedef int(CORECLR_DELEGATE_CALLTYPE *GetVec3Fn)(void *handle, const char *name, glm::vec3 *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetVec3Fn)(void *handle, const char *name, glm::vec3 *val);
    typedef int(CORECLR_DELEGATE_CALLTYPE *GetVec4Fn)(void *handle, const char *name, glm::vec4 *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetVec4Fn)(void *handle, const char *name, glm::vec4 *val);

    // String 通过 UTF-8 C 字符串进行互操作
    typedef const char *(CORECLR_DELEGATE_CALLTYPE *GetStringFn)(void *handle, const char *name);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetStringFn)(void *handle, const char *name, const char *value);

    static LoadAssemblyFn s_LoadGameAssembly = nullptr;
    static ClassExistsFn s_EntityClassExists = nullptr;

    static InstantiateClassFn s_InstantiateClass = nullptr;
    static ReleaseInstanceFn s_ReleaseInstance = nullptr;
    static OnCreateInstanceFn s_OnCreateInstance = nullptr;
    static OnDestroyInstanceFn s_OnDestroyInstance = nullptr;
    static OnUpdateInstanceFn s_OnUpdateInstance = nullptr;
    static OnFixedUpdateInstanceFn s_OnFixedUpdateInstance = nullptr;
    static OnCollisionEnter2DInstanceFn s_OnCollisionEnter2D = nullptr;
    static OnCollisionExit2DInstanceFn s_OnCollisionExit2D = nullptr;
    static OnTriggerEnter2DInstanceFn s_OnTriggerEnter2D = nullptr;
    static OnTriggerExit2DInstanceFn s_OnTriggerExit2D = nullptr;
    static OnPointerEventInstanceFn s_OnPointerEvent = nullptr;

    // Reflection
    static GetFieldsFn s_GetFields = nullptr;
    static GetFloatFn s_GetFloat = nullptr;
    static SetFloatFn s_SetFloat = nullptr;
    static GetIntFn s_GetInt = nullptr;
    static SetIntFn s_SetInt = nullptr;
    static GetBoolFn s_GetBool = nullptr;
    static SetBoolFn s_SetBool = nullptr;
    static GetVec2Fn s_GetVec2 = nullptr;
    static SetVec2Fn s_SetVec2 = nullptr;
    static GetVec3Fn s_GetVec3 = nullptr;
    static SetVec3Fn s_SetVec3 = nullptr;
    static GetVec4Fn s_GetVec4 = nullptr;
    static SetVec4Fn s_SetVec4 = nullptr;

    static GetStringFn s_GetString = nullptr;
    static SetStringFn s_SetString = nullptr;

    typedef int(CORECLR_DELEGATE_CALLTYPE *GetEntityIDFn)(void *handle, const char *name, uint64_t *out);
    typedef void(CORECLR_DELEGATE_CALLTYPE *SetEntityIDFn)(void *handle, const char *name, uint64_t val);
    static GetEntityIDFn s_GetEntityID = nullptr;
    static SetEntityIDFn s_SetEntityID = nullptr;

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
        if (!lib)
            return false;
        init_fptr =
                (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(lib, "hostfxr_initialize_for_runtime_config");
        get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)GetProcAddress(lib, "hostfxr_get_runtime_delegate");
        close_fptr = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");
#else
        void *lib = dlopen(buffer, RTLD_LAZY);
        if (!lib)
            return false;
        init_fptr = (hostfxr_initialize_for_runtime_config_fn)dlsym(lib, "hostfxr_initialize_for_runtime_config");
        get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)dlsym(lib, "hostfxr_get_runtime_delegate");
        close_fptr = (hostfxr_close_fn)dlsym(lib, "hostfxr_close");
#endif

        return (init_fptr && get_delegate_fptr && close_fptr);
    }

    // 辅助转换 wide string 到 std::string (仅用于日志)
    static std::string ToString(const char_t *str)
    {
#ifdef WIN32
        if (!str)
            return "";
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

        // 2. 确定配置文件的路径（与 HimiiEngine 子目录中的 ScriptCore 对齐）
        const std::filesystem::path engineDirectory = Application::GetEngineDir();
        std::filesystem::path runtimeConfigPath = engineDirectory / "ScriptCore.runtimeconfig.json";

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
        rc = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer,
                               (void **)&load_assembly_and_get_function_pointer);

        if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
        {
            std::cerr << "[ScriptEngine] Get delegate failed: " << std::hex << rc << std::endl;
            close_fptr(cxt);
            return;
        }

        // 5. 加载 ScriptCore 程序集并进行初始化
        std::filesystem::path assemblyPath = engineDirectory / "ScriptCore.dll";
        LoadAssembly(assemblyPath);
    }

    void ScriptEngine::Shutdown()
    {
        ReleaseAllInstances();
        s_SceneContext = nullptr;

        if (cxt)
        {
            close_fptr(cxt);
            cxt = nullptr;
        }
    }

    void ScriptEngine::LoadAssembly(const std::filesystem::path &filepath)
    {
        if (!load_assembly_and_get_function_pointer)
            return;

        // 定义初始化函数的签名
        typedef void(CORECLR_DELEGATE_CALLTYPE * InteropInitFn)(void *functionTable);
        InteropInitFn interopInit = nullptr;

        // 6. 调用 C# 的 Interop.Initialize
        // 格式: Namespace.Class, AssemblyName
        const char_t *dotnet_type = STR("HimiiEngine.InternalCalls, ScriptCore");
        const char_t *dotnet_type_method = STR("Initialize");

        int rc = load_assembly_and_get_function_pointer(
                filepath.c_str(), dotnet_type, dotnet_type_method,
                UNMANAGEDCALLERSONLY_METHOD, // 指示这是一个 [UnmanagedCallersOnly] 方法
                nullptr, (void **)&interopInit);

        if (rc == 0 || interopInit)
        {
            auto nativeFunctions = ScriptGlue::GetNativeFunctions();
            interopInit((void *)&nativeFunctions);
            std::cout << "[ScriptEngine] Successfully loaded ScriptCore! Initializing Interop..." << std::endl;
        }

        // 2. 获取 C# 的生命周期函数 (OnCreate, OnUpdate)
        // 假设我们在 C# HimiiEngine.ScriptManager 类中定义这些静态方法
        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("LoadGameAssembly"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_LoadGameAssembly);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("EntityClassExists"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_EntityClassExists);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("InstantiateClass"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_InstantiateClass);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("ReleaseInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_ReleaseInstance);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnCreateInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnCreateInstance);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnDestroyInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnDestroyInstance);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnUpdateInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnUpdateInstance);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnFixedUpdateInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnFixedUpdateInstance);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnCollisionEnter2DInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnCollisionEnter2D);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnCollisionExit2DInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnCollisionExit2D);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnTriggerEnter2DInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnTriggerEnter2D);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnTriggerExit2DInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnTriggerExit2D);

        load_assembly_and_get_function_pointer(filepath.c_str(), STR("HimiiEngine.ScriptManager, ScriptCore"),
                                               STR("OnPointerEventInstance"), UNMANAGEDCALLERSONLY_METHOD, nullptr,
                                               (void **)&s_OnPointerEvent);

        // [NEW] Load Reflection Bridge functions
        const char_t *bridge_type = STR("HimiiEngine.ReflectionBridge, ScriptCore");
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetFields"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetFields);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetFloat"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetFloat);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetFloat"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetFloat);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetInt"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetInt);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetInt"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetInt);

        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetBool"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetBool);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetBool"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetBool);

        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetVector2"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetVec2);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetVector2"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetVec2);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetVector3"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetVec3);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetVector3"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetVec3);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetVector4"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetVec4);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetVector4"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetVec4);

        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetString"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetString);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetString"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetString);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("GetEntityID"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_GetEntityID);
        load_assembly_and_get_function_pointer(filepath.c_str(), bridge_type, STR("SetEntityID"),
                                               UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&s_SetEntityID);
    }

    std::filesystem::path ScriptEngine::GetGameAssemblyDllPath()
    {
        if (!Project::GetActive())
            return {};

        return std::filesystem::absolute(Project::GetProjectDirectory() / Project::GetConfig().ScriptModulePath);
    }

    bool ScriptEngine::RequestCompileAndReload(const std::filesystem::path &projectPath)
    {
        if (ScriptCompiler::IsCompiling())
            return false;

        return ScriptCompiler::RequestBuild(projectPath, [projectPath](bool success)
        {
            if (!success)
            {
                std::cerr << "[ScriptEngine] Script compile failed.\n";
                return;
            }

            auto dllPath = GetGameAssemblyDllPath();
            if (std::filesystem::exists(dllPath))
                LoadAppAssembly(dllPath);
            else
                std::cerr << "[ScriptEngine] DLL not found at: " << dllPath << std::endl;
        });
    }

    bool ScriptEngine::LoadAppAssembly(const std::filesystem::path &filepath)
    {
        if (s_SceneContext != nullptr)
        {
            std::cerr << "[ScriptEngine] Cannot reload assembly while runtime is active. Stop Play first.\n";
            return false;
        }

        ReleaseAllInstances();

        if (!s_LoadGameAssembly)
            return false;

        auto absolutePath = std::filesystem::absolute(filepath);
        s_LoadGameAssembly(absolutePath.string().c_str());
        return true;
    }

    bool ScriptEngine::IsRuntimeActive()
    {
        return s_SceneContext != nullptr;
    }

    void ScriptEngine::ReleaseAllInstances()
    {
        for (auto &[uuid, handle] : s_EntityInstanceMap)
            DestroyInstance(handle);
        s_EntityInstanceMap.clear();
    }

    void ScriptEngine::DestroyInstance(void *handle)
    {
        if (!handle)
            return;

        if (s_OnDestroyInstance)
            s_OnDestroyInstance(handle);

        if (s_ReleaseInstance)
            s_ReleaseInstance(handle);
    }

    void ScriptEngine::OnCreateEntity(Entity entity)
    {
        const auto &sc = entity.GetComponent<ScriptComponent>();
        if (!ScriptEngine::EntityClassExists(sc.ClassName))
        {
            HIMII_CORE_ERROR("Script class '{0}' not found in GameAssembly. "
                             "Check Script Component ClassName and that scripts compiled.",
                             sc.ClassName);
            return;
        }

        UUID entityID = entity.GetUUID();

        // 先实例化、写入序列化字段，再调用 OnCreate，避免脚本在默认字段值上初始化。
        void *instance = InstantiateClass(entityID, sc.ClassName, ScriptInstanceFlags::None);
        if (!instance)
        {
            HIMII_CORE_ERROR("Failed to instantiate script class '{0}'", sc.ClassName);
            return;
        }

        for (const auto &[name, field] : sc.Fields)
        {
            if (field.Type == ScriptFieldType::Float)
            {
                float value = field.GetValue<float>();
                SetFloat(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Int)
            {
                int value = field.GetValue<int>();
                SetInt(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Bool)
            {
                bool value = field.GetValue<bool>();
                SetBool(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Vector2)
            {
                glm::vec2 value = field.GetValue<glm::vec2>();
                SetVector2(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Vector3)
            {
                glm::vec3 value = field.GetValue<glm::vec3>();
                SetVector3(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Vector4)
            {
                glm::vec4 value = field.GetValue<glm::vec4>();
                SetVector4(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::String)
            {
                std::string value = field.GetValue<std::string>();
                SetString(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::KeyCode)
            {
                int value = field.GetValue<int>();
                SetInt(instance, name, value);
            }
            else if (field.Type == ScriptFieldType::Entity)
            {
                UUID value = field.GetValue<UUID>();
                SetEntityField(instance, name, value);
            }
        }

        if (s_OnCreateInstance)
            s_OnCreateInstance(instance);
    }

    void ScriptEngine::OnRuntimeStart(Scene *scene)
    {
        s_SceneContext = scene;
    }

    void ScriptEngine::OnRuntimeStop()
    {
        ReleaseAllInstances();
        s_SceneContext = nullptr;
    }

    void ScriptEngine::OnUpdateScript(Entity entity, Timestep ts)
    {
        s_ScriptDeltaTime = ts.GetSeconds();

        if (!entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnUpdateInstance)
            s_OnUpdateInstance(handle, ts.GetSeconds());
    }

    void ScriptEngine::OnFixedUpdateScript(Entity entity, Timestep ts)
    {
        s_ScriptDeltaTime = ts.GetSeconds();

        if (!entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnFixedUpdateInstance)
            s_OnFixedUpdateInstance(handle, ts.GetSeconds());
    }

    float ScriptEngine::GetScriptDeltaTime()
    {
        return s_ScriptDeltaTime;
    }

    bool ScriptEngine::EntityClassExists(const std::string &fullClassName)
    {
        if (fullClassName.empty())
            return false;

        if (s_EntityClassExists)
        {
            int result = s_EntityClassExists(fullClassName.c_str());
            return result != 0;
        }
        return false;
    }

    Scene *ScriptEngine::GetSceneContext()
    {
        return s_SceneContext;
    }

    void ScriptEngine::SetActiveSceneRelativePath(const std::string& relativePath)
    {
        s_ActiveSceneRelativePath = relativePath;
    }

    const std::string& ScriptEngine::GetActiveSceneRelativePath()
    {
        return s_ActiveSceneRelativePath;
    }

    // [NEW] Maps C# fields to C++ ScriptFieldMap
    ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
    {
        auto& sc = entity.GetComponent<ScriptComponent>();
        
        UUID entityID = entity.GetUUID();
        void* instance = GetEntityScriptInstance(entityID);
        
        // If we instantiate here, we must be careful about Lifecycle.
        bool createdTemp = false;
        if (!instance)
        {
             instance = InstantiateClass(entityID, sc.ClassName, ScriptInstanceFlags::None);
             createdTemp = true;
        }

        if (instance)
        {
            std::string fieldsStr = GetFields(instance);
            // Format: "Name:Type;Name:Type;"
            
            std::unordered_map<std::string, ScriptFieldInstance> newFields;
            std::vector<std::string> currentFieldNames;

            std::stringstream ss(fieldsStr);
            std::string segment;
            while (std::getline(ss, segment, ';'))
            {
                size_t colonPos = segment.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string name = segment.substr(0, colonPos);
                    std::string typeStr = segment.substr(colonPos + 1);
                    
                    currentFieldNames.push_back(name);

                    // Determine Type
                    ScriptFieldType type = ScriptFieldType::None;
                    if (typeStr == "Single") type = ScriptFieldType::Float;
                    else if (typeStr == "Int32") type = ScriptFieldType::Int;
                    else if (typeStr == "Boolean") type = ScriptFieldType::Bool;
                    else if (typeStr == "Vector2") type = ScriptFieldType::Vector2;
                    else if (typeStr == "Vector3") type = ScriptFieldType::Vector3;
                    else if (typeStr == "Vector4") type = ScriptFieldType::Vector4;
                    else if (typeStr == "Entity") type = ScriptFieldType::Entity;
                    else if (typeStr == "String") type = ScriptFieldType::String;
                    else if (typeStr == "KeyCode") type = ScriptFieldType::KeyCode;

                    if (type != ScriptFieldType::None)
                    {
                        // If field exists, keep its value (unless type mismatched completely)
                        if (sc.Fields.find(name) != sc.Fields.end())
                        {
                            ScriptFieldInstance& existingField = sc.Fields[name];
                            if (existingField.Type == type)
                            {
                                newFields[name] = existingField;
                            }
                            else
                            {
                                // Type mismatch, use new default
                                ScriptFieldInstance fieldInst;
                                fieldInst.Type = type;
                                // Get default... (same as below)
                                // We can extract a helper for getting value but for now duplicate
                                // Actually, let's just fall through to "new field" logic for simplicity or copy-paste
                                // But if we want to overwrite, we need to read from C# again.
                                // Let's just create a function to read value.
                            }
                        }
                        
                        // If not in newFields yet (either new or type mismatch handling needed)
                        if (newFields.find(name) == newFields.end())
                        {
                             ScriptFieldInstance fieldInst;
                             fieldInst.Type = type;
                             
                             // Get default value from C# instance
                             if (type == ScriptFieldType::Float)
                             {
                                 float val; GetFloat(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Int)
                             {
                                 int val; GetInt(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Bool)
                             {
                                 bool val; GetBool(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Vector2)
                             {
                                 glm::vec2 val; GetVector2(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Vector3)
                             {
                                 glm::vec3 val; GetVector3(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Vector4)
                             {
                                 glm::vec4 val; GetVector4(instance, name, val);
                                 fieldInst.SetValue(val);
                             }
                             else if (type == ScriptFieldType::Entity)
                             {
                                 UUID entityID{};
                                 if (GetEntityField(instance, name, entityID))
                                     fieldInst.SetValue(entityID);
                             }
                             else if (type == ScriptFieldType::String)
                             {
                                 std::string value;
                                 GetString(instance, name, value);
                                 fieldInst.SetValue(std::move(value));
                             }
                             else if (type == ScriptFieldType::KeyCode)
                             {
                                 int code = 0;
                                 GetInt(instance, name, code);
                                 fieldInst.SetValue(code);
                             }

                             newFields[name] = fieldInst;
                        }
                    }
                }
            }
            
            // Replace fields with the new set (pruning old ones)
            sc.Fields = newFields;
            
            if (createdTemp)
            {
                DestroyInstance(instance);
                s_EntityInstanceMap.erase(entityID);
            }
        }
        
        return sc.Fields;
    }

    void ScriptEngine::InitInterop()
    {
        // Placeholder implementation
    }

    void *ScriptEngine::InstantiateClass(UUID entityID, const std::string &className, ScriptInstanceFlags flags)
    {
        if (s_InstantiateClass)
        {
            void *handle = s_InstantiateClass(entityID, className.c_str(), static_cast<int>(flags));
            if (handle)
                s_EntityInstanceMap[entityID] = handle;
            return handle;
        }
        return nullptr;
    }

    std::string ScriptEngine::GetFields(void *instanceHandle)
    {
        if (s_GetFields && instanceHandle)
        {
            const char *str = s_GetFields(instanceHandle);
            std::string result = str ? str : "";
#ifdef WIN32
            if (str)
                CoTaskMemFree((LPVOID)str);
#endif
            return result;
        }
        return "";
    }

    bool ScriptEngine::GetFloat(void *instanceHandle, const std::string &fieldName, float &outValue)
    {
        if (s_GetFloat && instanceHandle)
            return s_GetFloat(instanceHandle, fieldName.c_str(), &outValue);
        return false;
    }

    void ScriptEngine::SetFloat(void *instanceHandle, const std::string &fieldName, float value)
    {
        if (s_SetFloat && instanceHandle)
            s_SetFloat(instanceHandle, fieldName.c_str(), value);
    }

    bool ScriptEngine::GetInt(void *instanceHandle, const std::string &fieldName, int &outValue)
    {
        if (s_GetInt && instanceHandle)
            return s_GetInt(instanceHandle, fieldName.c_str(), &outValue);
        return false;
    }
    void ScriptEngine::SetInt(void *instanceHandle, const std::string &fieldName, int value)
    {
        if (s_SetInt && instanceHandle)
            s_SetInt(instanceHandle, fieldName.c_str(), value);
    }

    bool ScriptEngine::GetBool(void *instanceHandle, const std::string &fieldName, bool &outValue)
    {
        if (s_GetBool && instanceHandle)
        {
            int val;
            bool res = s_GetBool(instanceHandle, fieldName.c_str(), &val);
            outValue = (val != 0);
            return res;
        }
        return false;
    }
    void ScriptEngine::SetBool(void *instanceHandle, const std::string &fieldName, bool value)
    {
        if (s_SetBool && instanceHandle)
            s_SetBool(instanceHandle, fieldName.c_str(), value ? 1 : 0);
    }

    bool ScriptEngine::GetVector2(void *instanceHandle, const std::string &fieldName, glm::vec2 &outValue)
    {
        if (s_GetVec2 && instanceHandle)
            return s_GetVec2(instanceHandle, fieldName.c_str(), &outValue);
        return false;
    }
    void ScriptEngine::SetVector2(void *instanceHandle, const std::string &fieldName, const glm::vec2 &value)
    {
        glm::vec2 temp = value;
        if (s_SetVec2 && instanceHandle)
            s_SetVec2(instanceHandle, fieldName.c_str(), &temp);
    }

    bool ScriptEngine::GetVector3(void *instanceHandle, const std::string &fieldName, glm::vec3 &outValue)
    {
        if (s_GetVec3 && instanceHandle)
            return s_GetVec3(instanceHandle, fieldName.c_str(), &outValue);
        return false;
    }
    void ScriptEngine::SetVector3(void *instanceHandle, const std::string &fieldName, const glm::vec3 &value)
    {
        glm::vec3 temp = value;
        if (s_SetVec3 && instanceHandle)
            s_SetVec3(instanceHandle, fieldName.c_str(), &temp);
    }

    bool ScriptEngine::GetVector4(void *instanceHandle, const std::string &fieldName, glm::vec4 &outValue)
    {
        if (s_GetVec4 && instanceHandle)
            return s_GetVec4(instanceHandle, fieldName.c_str(), &outValue);
        return false;
    }
    void ScriptEngine::SetVector4(void *instanceHandle, const std::string &fieldName, const glm::vec4 &value)
    {
        glm::vec4 temp = value;
        if (s_SetVec4 && instanceHandle)
            s_SetVec4(instanceHandle, fieldName.c_str(), &temp);
    }

    bool ScriptEngine::GetString(void *instanceHandle, const std::string &fieldName, std::string &outValue)
    {
        if (s_GetString && instanceHandle)
        {
            const char *str = s_GetString(instanceHandle, fieldName.c_str());
            if (str)
            {
                outValue = str;
#ifdef WIN32
                CoTaskMemFree((LPVOID)str);
#endif
                return true;
            }
        }
        return false;
    }

    void ScriptEngine::SetString(void *instanceHandle, const std::string &fieldName, const std::string &value)
    {
        if (s_SetString && instanceHandle)
        {
            s_SetString(instanceHandle, fieldName.c_str(), value.c_str());
        }
    }

    bool ScriptEngine::GetEntityField(void *instanceHandle, const std::string &fieldName, UUID &outEntityID)
    {
        if (s_GetEntityID && instanceHandle)
        {
            uint64_t id = 0;
            if (s_GetEntityID(instanceHandle, fieldName.c_str(), &id))
            {
                outEntityID = id;
                return true;
            }
        }
        return false;
    }

    void ScriptEngine::SetEntityField(void *instanceHandle, const std::string &fieldName, UUID entityID)
    {
        if (s_SetEntityID && instanceHandle)
            s_SetEntityID(instanceHandle, fieldName.c_str(), entityID);
    }

    void ScriptEngine::OnCollisionEnter2D(Entity entity, Entity other, const Collision2DInterop &collision)
    {
        if (!entity || !other || !entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnCollisionEnter2D)
            s_OnCollisionEnter2D(handle, collision);
    }

    void ScriptEngine::OnCollisionExit2D(Entity entity, Entity other)
    {
        if (!entity || !other || !entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnCollisionExit2D)
            s_OnCollisionExit2D(handle, other.GetUUID());
    }

    void ScriptEngine::OnTriggerEnter2D(Entity entity, Entity other, const Collision2DInterop &collision)
    {
        if (!entity || !other || !entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnTriggerEnter2D)
            s_OnTriggerEnter2D(handle, collision);
    }

    void ScriptEngine::OnTriggerExit2D(Entity entity, Entity other)
    {
        if (!entity || !other || !entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnTriggerExit2D)
            s_OnTriggerExit2D(handle, other.GetUUID());
    }

    void ScriptEngine::OnPointerEvent(Entity entity, ScriptPointerEventType eventType)
    {
        if (!entity || !entity.HasComponent<ScriptComponent>())
            return;

        void *handle = GetEntityScriptInstance(entity.GetUUID());
        if (handle && s_OnPointerEvent)
            s_OnPointerEvent(handle, static_cast<int>(eventType));
    }

    void *ScriptEngine::GetEntityScriptInstance(UUID entityID)
    {
        auto it = s_EntityInstanceMap.find(entityID);
        if (it != s_EntityInstanceMap.end())
            return it->second;
        return nullptr;
    }

    /*std::string ScriptEngine::Serialize(void *instanceHandle)
    {
        if (s_Serialize && instanceHandle)
        {
            const char *str = s_Serialize(instanceHandle);
            std::string result = str ? str : "";
#ifdef WIN32
            if (str)
                CoTaskMemFree((LPVOID)str);
#endif
            return result;
        }
        return "";
    }

    void ScriptEngine::Deserialize(void *instanceHandle, const std::string &json)
    {
        if (s_Deserialize && instanceHandle)
        {
            s_Deserialize(instanceHandle, json.c_str());
        }
    }*/
} // namespace Himii
