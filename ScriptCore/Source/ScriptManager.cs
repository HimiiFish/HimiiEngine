using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Runtime.Loader;

namespace HimiiEngine
{
    public enum ScriptInstanceFlags : int
    {
        None = 0,
        InvokeOnCreate = 1
    }

    public class ScriptManager
    {
        private static Dictionary<string, Type> _entityClasses = new Dictionary<string, Type>();

        private class GameAssemblyLoadContext : AssemblyLoadContext
        {
            public GameAssemblyLoadContext() : base(isCollectible: true) { }

            protected override Assembly? Load(AssemblyName assemblyName)
            {
                var existingAssembly = AppDomain.CurrentDomain.GetAssemblies()
                    .FirstOrDefault(a => a.GetName().Name == assemblyName.Name);

                if (existingAssembly != null)
                    return existingAssembly;

                return null;
            }
        }

        private static GameAssemblyLoadContext? _currentContext;

        [UnmanagedCallersOnly]
        public static void LoadGameAssembly(IntPtr assemblyPathPtr)
        {
            string assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr) ?? "";
            Console.WriteLine($"[C#] Loading Assembly: {assemblyPath}");

            try
            {
                if (_currentContext != null)
                {
                    _entityClasses.Clear();
                    _currentContext.Unload();
                    _currentContext = null;
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                }

                _currentContext = new GameAssemblyLoadContext();

                string pdbPath = Path.ChangeExtension(assemblyPath, ".pdb");
                bool pdbExists = File.Exists(pdbPath);

                using (var stream = new FileStream(assemblyPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                {
                    Assembly gameAssembly;
                    if (pdbExists)
                    {
                        using (var pdbStream = new FileStream(pdbPath, FileMode.Open, FileAccess.Read, FileShare.Read))
                        {
                            Console.WriteLine($"[C#] Loading PDB: {pdbPath}");
                            gameAssembly = _currentContext.LoadFromStream(stream, pdbStream);
                        }
                    }
                    else
                    {
                        gameAssembly = _currentContext.LoadFromStream(stream);
                    }

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

            foreach (var type in assembly.GetTypes())
            {
                if (type.IsAbstract || type.IsInterface)
                    continue;

                if (type.IsSubclassOf(typeof(Entity)))
                {
                    string fullName = type.FullName ?? type.Name;
                    _entityClasses[fullName] = type;
                    if (type.Name != fullName)
                        _entityClasses[type.Name] = type;
                    Console.WriteLine($"[C#] Registered Entity Class: {fullName}");
                }
            }
        }

        [UnmanagedCallersOnly]
        public static int EntityClassExists(IntPtr classNamePtr)
        {
            try
            {
                if (classNamePtr == IntPtr.Zero) return 0;
                string className = Marshal.PtrToStringUTF8(classNamePtr);
                if (string.IsNullOrEmpty(className)) return 0;

                return _entityClasses.ContainsKey(className) ? 1 : 0;
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] EntityClassExists failed: {e.Message}");
                return 0;
            }
        }

        private static Entity? CreateEntityInstance(ulong entityID, string className, ScriptInstanceFlags flags)
        {
            try
            {
                if (!_entityClasses.TryGetValue(className, out Type? type))
                    return null;

                var entity = (Entity)Activator.CreateInstance(type)!;
                entity.ID = entityID;

                if (flags.HasFlag(ScriptInstanceFlags.InvokeOnCreate))
                {
                    try
                    {
                        entity.OnCreate();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"[C# Error] {className}.OnCreate threw exception: {ex.Message}");
                    }
                }

                return entity;
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] CreateEntityInstance failed: {e.Message}");
                return null;
            }
        }

        [UnmanagedCallersOnly]
        public static IntPtr InstantiateClass(ulong entityID, IntPtr classNamePtr, int flags)
        {
            try
            {
                string className = Marshal.PtrToStringUTF8(classNamePtr) ?? "";
                if (!_entityClasses.ContainsKey(className))
                {
                    Console.WriteLine($"[C# Error] Cannot instantiate class '{className}': Class not found.");
                    return IntPtr.Zero;
                }

                var scriptFlags = (ScriptInstanceFlags)flags;
                var entity = CreateEntityInstance(entityID, className, scriptFlags);
                if (entity != null)
                {
                    GCHandle handle = GCHandle.Alloc(entity);
                    return GCHandle.ToIntPtr(handle);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Critical Error] InstantiateClass failed: {e.Message}");
                Console.WriteLine(e.StackTrace);
            }

            return IntPtr.Zero;
        }

        [UnmanagedCallersOnly]
        public static void ReleaseInstance(IntPtr handlePtr)
        {
            if (handlePtr == IntPtr.Zero) return;
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            if (handle.IsAllocated)
                handle.Free();
        }

        [UnmanagedCallersOnly]
        public static void OnCreateInstance(IntPtr handlePtr)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnCreate();
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnCreateInstance failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnDestroyInstance(IntPtr handlePtr)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnDestroy();
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnDestroyInstance failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnUpdateInstance(IntPtr handlePtr, float timestep)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnUpdate(timestep);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnUpdateInstance failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnFixedUpdateInstance(IntPtr handlePtr, float timestep)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnFixedUpdate(timestep);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnFixedUpdateInstance failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnCollisionEnter2DInstance(IntPtr handlePtr, Collision2DInfo collision)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnCollisionEnter2D(collision);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C# Error] OnCollisionEnter2DInstance failed: {e.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnCollisionExit2DInstance(IntPtr handlePtr, ulong otherEntityID)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnCollisionExit2D(new Collision2DInfo { OtherEntityID = otherEntityID });
            }
            catch (Exception exception)
            {
                Console.WriteLine($"[C# Error] OnCollisionExit2DInstance failed: {exception.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnTriggerEnter2DInstance(IntPtr handlePtr, Collision2DInfo collision)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnTriggerEnter2D(collision);
            }
            catch (Exception exception)
            {
                Console.WriteLine($"[C# Error] OnTriggerEnter2DInstance failed: {exception.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnTriggerExit2DInstance(IntPtr handlePtr, ulong otherEntityID)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (handle.IsAllocated && handle.Target is Entity entity)
                    entity.OnTriggerExit2D(new Collision2DInfo { OtherEntityID = otherEntityID });
            }
            catch (Exception exception)
            {
                Console.WriteLine($"[C# Error] OnTriggerExit2DInstance failed: {exception.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static void OnPointerEventInstance(IntPtr handlePtr, int eventType)
        {
            if (handlePtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                if (!handle.IsAllocated || handle.Target is not Entity entity)
                    return;

                switch (eventType)
                {
                    case 0: entity.OnPointerEnter(); break;
                    case 1: entity.OnPointerExit(); break;
                    case 2: entity.OnPointerDown(); break;
                    case 3: entity.OnPointerUp(); break;
                    case 4: entity.OnPointerClick(); break;
                }
            }
            catch (Exception exception)
            {
                Console.WriteLine($"[C# Error] OnPointerEventInstance failed: {exception.Message}");
            }
        }
    }
}
