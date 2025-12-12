using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Runtime.Loader;

namespace Himii
{
    public class ScriptManager
    {
        private static Dictionary<ulong, Entity> _instances = new Dictionary<ulong, Entity>();

        private static Dictionary<string, Type> _entityClasses = new Dictionary<string, Type>();

        private class GameAssemblyLoadContext : AssemblyLoadContext
        {
            public GameAssemblyLoadContext() : base(isCollectible: true) { }

            protected override Assembly? Load(AssemblyName assemblyName)
            {
                // 核心逻辑：
                // 当 GameAssembly 试图加载 ScriptCore 时，
                // 我们拦截请求，并返回当前 AppDomain 中已经存在的 ScriptCore。
                // 这样就能保证 Entity 类型是同一个。

                var existingAssembly = AppDomain.CurrentDomain.GetAssemblies()
                    .FirstOrDefault(a => a.GetName().Name == assemblyName.Name);

                if (existingAssembly != null)
                    return existingAssembly;

                return null; // 其他依赖尝试默认加载
            }
        }

        private static GameAssemblyLoadContext? _currentContext;

        [UnmanagedCallersOnly]
        public static void LoadGameAssembly(IntPtr assemblyPathPtr)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr);
            Console.WriteLine($"[C#] Loading Game Assembly: {assemblyPath}");

            try
            {
                if (_currentContext != null)
                {
                    _currentContext.Unload();
                    _currentContext = null;
                    GC.Collect(); // 尝试触发垃圾回收
                    GC.WaitForPendingFinalizers();
                }

                _currentContext = new GameAssemblyLoadContext();

                // 3. 读取 DLL 字节流
                // 使用 FileShare.Read 防止文件被锁，允许外部再次编译
                byte[] assemblyData;
                using (var stream = new FileStream(assemblyPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    assemblyData = new byte[stream.Length];
                    stream.Read(assemblyData, 0, assemblyData.Length);
                }

                // 4. 通过 Context 加载程序集
                // 这里的 LoadFromStream 会触发上面的 Load(AssemblyName) 重写方法
                using (var stream = new MemoryStream(assemblyData))
                {
                    Assembly gameAssembly = _currentContext.LoadFromStream(stream);
                    LoadAssemblyClasses(gameAssembly);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error loading assembly: {e.Message}");
                Console.WriteLine(e.StackTrace);
            }
        }

        private static void LoadAssemblyClasses(Assembly assembly)
        {
            _entityClasses.Clear();
            _instances.Clear();

            foreach (var type in assembly.GetTypes())
            {
                if (type.IsAbstract || type.IsInterface)
                    continue;

                // 这里的 Entity 就是 Host 里的 Entity，因为我们重定向了 ScriptCore
                if (type.IsSubclassOf(typeof(Entity)))
                {
                    string fullName = type.FullName;
                    _entityClasses[fullName] = type;
                    if (type.Name != fullName)
                        _entityClasses[type.Name] = type;

                    Console.WriteLine($"[C#] Registered Entity Class: {fullName}");
                }
            }
        }

        [UnmanagedCallersOnly]
        public static bool EntityClassExists(IntPtr classNamePtr)
        {
            string className = Marshal.PtrToStringUTF8(classNamePtr);
            return _entityClasses.ContainsKey(className);
        }

        [UnmanagedCallersOnly]
        public static void OnCreateEntity(ulong entityID, IntPtr classNamePtr)
        {
            string className = Marshal.PtrToStringUTF8(classNamePtr);

            if (_entityClasses.TryGetValue(className, out Type type))
            {
                // 反射创建实例
                var entity = (Entity)Activator.CreateInstance(type);
                entity.ID = entityID;
                _instances[entityID] = entity;
                entity.OnCreate();
            }
            else
            {
                Console.WriteLine($"[C#] Error: Class '{className}' not found!");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnUpdateEntity(ulong entityID, float ts)
        {
            if (_instances.TryGetValue(entityID, out var entity))
            {
                entity.OnUpdate(ts);
            }
        }
    }
}