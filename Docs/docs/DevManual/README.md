# 开发者手册

本手册面向 HimiiEngine 的源码贡献者与深入研究者。文档站点上与用户手册分区展示；本分区可写内部路径与模块边界。

## 目录

1. **[源码构建 (Building From Source)](BuildingFromSource.md)**  
   环境要求、CMake Preset、输出目录

2. **[第三方依赖](ThirdParty.md)**  
   vcpkg / `vender` 分桶、冻结方式、版本与许可证简表

3. **[架构概览 (Architecture)](Architecture.md)**  
   分层、目录地图、Application / Module / World / Resource / Project

4. **核心系统 (Core Systems)**
   - **[编辑器运行时](CoreSystems/EditorRuntime.md)** — Play / Simulate、World 帧调度
   - **[脚本系统（C++）](CoreSystems/Scripting.md)** — 宿主加载链与阶段
   - **[日志数据流](CoreSystems/Logging.md)** — spdlog / ConsoleLog
   - **[图形笔记](CoreSystems/GraphicsNotes.md)** — OpenGL / GLM / ImGui 速查
   - **[场景序列化](CoreSystems/Serialization.md)** — `.himii` 与资产序列化分工
