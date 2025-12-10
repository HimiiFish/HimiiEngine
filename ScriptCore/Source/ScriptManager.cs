using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;

namespace Himii
{
    public class ScriptManager
    {
        private static Dictionary<ulong, Entity> _instances = new Dictionary<ulong, Entity>();

        private static Dictionary<string, Type> _entityClasses = new Dictionary<string, Type>();

        [UnmanagedCallersOnly]
        public static void LoadGameAssembly(IntPtr assemblyPathPtr)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr);
            Console.WriteLine($"[C#] Loading Game Assembly: {assemblyPath}");

            try
            {
                byte[] assemblyData = File.ReadAllBytes(assemblyPath);
                Assembly gameAssembly = Assembly.LoadFrom(assemblyPath);

                _entityClasses.Clear();
                _instances.Clear();

                // 扫描 ScriptCore (自身) 和 GameAssembly 中的所有 Entity 子类
                var assemblies = new List<Assembly> { typeof(ScriptManager).Assembly, gameAssembly };
                foreach (var asm in assemblies)
                {
                    foreach (var type in asm.GetTypes())
                    {
                        if (type.IsSubclassOf(typeof(Entity)) && !type.IsAbstract)
                        {
                            _entityClasses[type.FullName] = type;
                            // 如果用户输入的类名不带命名空间，我们也存一份以便查找
                            if (type.FullName != type.Name)
                                _entityClasses[type.Name] = type;

                            Console.WriteLine($"[C#] Registered Class: {type.FullName}");
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error loading assembly: {e.Message}");
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