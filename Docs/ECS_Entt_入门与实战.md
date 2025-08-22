# EnTT（ECS）深入解析与实战：在 HimiiEngine 中落地

> 目标：基于数据导向（DOD）的视角，系统性对比 Archetype 型 ECS（以 Bevy 为例）与 Sparse-Set 型 ECS（EnTT），给出工程化落地范式、性能权衡与踩坑规避策略，并结合本仓库（HimiiEngine）的需求提供可直接套用的代码组织方案。

---

## 0. TL;DR（给忙碌的你）

- 小而快、C++ 生态：选 EnTT。它用 Sparse Set 存储，增删改组件/实体成本低、遍历也快；结合 `group` 能获取良好局部性。适合引擎/编辑器型 C++ 项目。
- 大量稳定“装配形态”的游戏：Archetype（如 Bevy）在热路径上有极佳连续内存（整列式）局部性，但改动原型（增删组件）时需要搬迁，突发性成本更高。
- 系统与调度：Bevy 自动冲突检测与并行调度；EnTT 给出高效数据结构，你需要自己定义调度顺序/并行策略（可配任务系统）。
- 变更时机：Bevy 常用 Commands 做延迟生灭；EnTT 支持即时增删，但要避开当前迭代器失效场景，必要时自建延迟队列。
- HimiiEngine 采用 EnTT：围绕 `registry` 搭建 Scene/Entity 薄封装，热路径使用 owning group，编辑器用 observer/dispatcher。

---

## 1. ECS 思想与 EnTT 的选择理由

- ECS 拆解：
  - Entity：身份（ID + 代际），不携带行为；
  - Component：纯数据（POD/结构体），可独立组合；
  - System：按“需要的数据组合”查询/遍历实体，并执行业务逻辑。
- 好处：高内聚的数据布局、低耦合的逻辑拼装、增量扩展容易（新行为=新组件+新系统）。
- 为什么 EnTT：
  - Header-only、现代 C++、泛型元编程成熟；
  - Sparse Set + group/view 组合，增删操作稳定、遍历高效；
  - 丰富周边：observer、dispatcher、resource_cache、meta，方便编辑器化与工具化。

---

## 2. 存储模型的对比：Archetype vs Sparse Set（参考 Bevy Cheat Book）

- Archetype（原型表）
  - 概念：实体的“组件集合类型”决定其 Archetype；同 Archetype 的组件按列式连续存储，遍历缓存局部性极佳。
  - 代价：实体增删组件会导致跨 Archetype 搬迁（数据移动）；频繁结构变化可能带来抖动。
  - 调度：框架（如 Bevy）可静态分析系统读写集，自动并行。
- Sparse Set（稀疏集合，EnTT）
  - 概念：每种组件独立存储（稀疏索引 + 紧凑密集数组），实体是否拥有该组件在稀疏索引中查得；
  - 特点：增删组件接近 O(1)，无整体搬迁；遍历性能高且灵活，通过 `view`/`group` 进一步优化；
  - Owning group：将若干组件“绑定”为连续块，热路径（如渲染收集）可享受接近 archetype 的局部性；
  - 组装弹性：组件独立，结构变化开销稳定，编辑器场景友好。

工程建议：
- 若你的游戏对象形态变化频繁、编辑器改动活跃，EnTT 的 Sparse Set 更稳健；
- 若对象形态较固定且查询模式可预测，Archetype 的整列式连续内存可能在极致性能下更优；
- HimiiEngine 面向引擎/工具与多样玩法实验，EnTT 更契合。

---

## 3. EnTT 核心概念与 API 速览

- `entt::registry`：实体/组件的主容器；创建/销毁实体，`emplace/remove` 组件，生成 `view`/`group`。
- `entt::entity`：句柄（含代际），`entt::null` 表示空。
- `view<T...>`：按需组件集合的遍历视图（可 `exclude<U...>`）。
- `group<T...>(entt::get<U...>)`：可选 owning/non-owning，owning 下 T 连续、U 以 `get` 拉取；常用于热路径。
- `observer`：监听组件增删/替换事件；
- `dispatcher`：事件总线，系统间解耦通信；
- `resource_cache`：资源句柄缓存（纹理/网格/着色器等），避免重复加载；
- `meta`：反射，用于通用 Inspector/序列化等高级玩法。

示例（简化）：
```cpp
struct Transform { glm::vec3 pos{0}; };
struct SpriteRenderer { glm::vec4 color{1}; };

entt::registry reg;
auto e = reg.create();
reg.emplace<Transform>(e);
reg.emplace<SpriteRenderer>(e, glm::vec4{1,1,1,1});

// 只读/可写遍历
for (auto [entity, tr, sr] : reg.view<Transform, SpriteRenderer>().each()) {
    // ...
}

// 热路径：owning group（Transform 连续）
auto g = reg.group<Transform>(entt::get<SpriteRenderer>());
for (auto entity : g) {
    auto &tr = g.get<Transform>(entity);
    auto &sr = g.get<SpriteRenderer>(entity);
    // 提交渲染
}
```

---

## 4. 系统、查询与并行

- 查询（Query）= `view`/`group` 定义所需组件集合；
- 系统（System）= 普通函数/可调用对象，入参为 `registry` 或预构建的视图；
- 顺序与并行：
  - EnTT 不内置调度器；你需要自己安排系统执行顺序；
  - 并行策略：
    1) 读写分离：同时运行“只读组件”的系统；
    2) 显式分阶段：输入→脚本→物理→渲染收集→渲染提交；
    3) 引入任务系统：如 cpp-taskflow / folly / TBB，将无数据冲突的系统作为任务并行；
  - 约束：`registry` 的同一组件存储非线程安全，跨线程访问需外部同步或数据分片。

---

## 5. 变更与时机：即时修改 vs 延迟命令

- Bevy 的 Commands 会在安全窗口统一应用；
- EnTT 支持即时 `create/destroy/emplace/remove`，但需要避免：
  - 在遍历同一个 `view`/`group` 时修改该视图涉及的组件，导致迭代器失效或顺序变化；
  - 建议：
    - 收集到 `std::vector<entt::entity>` 或操作闭包，遍历结束后统一应用；
    - 或分两次 `view`（读+写分离）；
    - 或用 `observer` 监听增删，在适当阶段处理。

示例：延迟销毁
```cpp
std::vector<entt::entity> to_kill;
auto v = reg.view<Health>();
for (auto e : v) if (v.get<Health>(e).hp <= 0) to_kill.push_back(e);
for (auto e : to_kill) reg.destroy(e);
```

---

## 6. 在 HimiiEngine 中的落地架构（推荐）

目录组织：
```
Engine/src/Himii/Scene/
  Components.h/.cpp      // Transform, Sprite, Camera, Script, Physics ...
  Entity.h/.cpp          // 对 entt::entity 的薄包装，提供 Add/Get/Has/Remove
  Scene.h/.cpp           // 持有 registry；调度 Update；封装渲染提交流程
  Systems/               // Movement, Physics, Script, Rendering, Animation ...
```

Entity 包装（摘要）：
```cpp
class Scene;
class Entity {
public:
  Entity() = default;
  Entity(entt::entity h, Scene* s): handle(h), scene(s) {}
  template<class T, class...Args> T& Add(Args&&...args);
  template<class T> bool Has() const;
  template<class T> T& Get();
  template<class T> void Remove();
  entt::entity Raw() const { return handle; }
private:
  entt::entity handle{entt::null};
  Scene* scene{};
};
```

Scene 驱动（摘要）：
```cpp
class Scene {
public:
  Entity Create(const char* name=nullptr);
  void Destroy(Entity e);
  void OnUpdate(Timestep ts) {
    UpdateScripts(reg, ts);
    UpdateMovement(reg, (float)ts);
    Renderer2D::BeginScene(camera);
    SubmitSprites(reg);
    Renderer2D::EndScene();
  }
  entt::registry& Registry() { return reg; }
private:
  entt::registry reg;
};
```

编辑器集成：
- Hierarchy：`registry.each(...)` 枚举实体；
- Inspector：`try_get` + `emplace/remove`；
- 监听：`observer` 在组件变更时刷新面板；
- 事件：`dispatcher` 用于“选择变化/资源热更”等广播。

---

## 7. 进阶特性与适用场景

- `group`：
  - Owning：热路径（渲染/物理收集）拿到连续的主组件，加速遍历；
  - Non-owning：快速组合，无所有权改变，构建成本低；
- `observer`：
  - 监听 `on_construct<T> / on_destroy<T> / on_update<T>`，驱动编辑器 UI、脏标记；
- `dispatcher`：
  - 事件总线，系统间解耦；适合碰撞、拾取、关卡事件；
- 资源：
  - `resource_cache` 用于共享资源句柄；项目中建议再包一层 ResourceManager（哈希 key + 生命周期策略）；
- 反射 `meta`：
  - 可做通用 Inspector/序列化（字段枚举、类型注册）。

---

## 8. 性能与工程实践清单

- 存储选型：热路径用 owning group；编辑器/工具期采用 view/非拥有 group 平衡灵活性；
- 组件设计：POD/轻量化，资源改用 ID/句柄；避免在组件内持有大对象与复杂所有权；
- 迭代安全：同一遍历期间避免结构性修改；必要时延迟；
- 批处理：批量创建/销毁时尽量预留空间，减少分配；
- 并行：引入任务系统，按读写集划分阶段并行；
- 诊断：统计遍历实体数、系统耗时、组件热度，指导分组与数据布局；
- 序列化：按组件种类序列化，生成版本号与迁移脚本，控制兼容性。

---

## 9. 常见陷阱与规避

- 视图迭代期间修改相同视图涉及的组件 → 可能打乱迭代或失效；改为延迟或拆相；
- Owning group 会移动组件存储：确保组件可移动/拷贝语义正确；
- 悬挂指针：组件间引用用 `entt::entity` 或全局索引，避免裸指针；
- 线程安全：`registry` 非并发容器，跨线程需外部同步或分区数据；
- ID 复用：销毁实体后 ID 会复用（带代际），不要在外部长期缓存旧 ID 未校验代际。

---

## 10. 快速上手（完整最小示例）

组件与系统：
```cpp
struct Transform { glm::vec3 position{0}; };
struct Velocity  { glm::vec3 value{0}; };
struct SpriteRenderer { glm::vec4 color{1}; };

void Movement(entt::registry &reg, float dt) {
  auto v = reg.view<Transform, Velocity>();
  for (auto e : v) v.get<Transform>(e).position += v.get<Velocity>(e).value * dt;
}

void SubmitSprites(entt::registry &reg) {
  auto g = reg.group<Transform>(entt::get<SpriteRenderer>());
  for (auto e : g) {
    auto &tr = g.get<Transform>(e);
    auto &sr = g.get<SpriteRenderer>(e);
    Himii::Renderer2D::DrawQuad({tr.position.x, tr.position.y}, {1,1}, sr.color);
  }
}
```

延迟销毁：
```cpp
std::vector<entt::entity> to_kill;
auto v = reg.view<SpriteRenderer>();
for (auto e : v) if (/* 条件 */) to_kill.push_back(e);
for (auto e : to_kill) reg.destroy(e);
```

---

## 11. 在项目中引入 EnTT（vcpkg + CMake）

EnTT 为 header-only，经 vcpkg 安装后即可 `#include <entt/entt.hpp>`。

- `vcpkg.json` 建议：
```json
{
  "dependencies": ["glad", "glfw3", "spdlog", "glm", "stb", "entt"]
}
```
- CMake：无需 `find_package`；确保 vcpkg manifest 模式启用即可。

---

## 12. 参考与延伸阅读（精选）

- Bevy Cheat Book：Intro: Your Data（ECS/Archetype/查询/并行 等概览）
  - https://bevy-cheatbook.github.io/programming/intro-data.html
- EnTT 仓库与文档
  - https://github.com/skypjack/entt
- EnTT 生态讨论与 Wiki（高级用法/设计理念）
  - https://github.com/skypjack/entt/wiki
- Data-Oriented Design（DOD）与 ECS 讨论集合
  - https://www.dataorienteddesign.com/dodmain/
- 其他 ECS 参考实现（对比阅读）
  - Flecs（C/C++）：https://github.com/SanderMertens/flecs

---

## 13. 总结（落地处方）

- 选型：HimiiEngine 采用 EnTT 的 Sparse Set + owning group，兼顾编辑器灵活与运行时热路径性能。
- 结构：以 `Scene/Entity/Components/Systems` 组织代码；Scene 驱动更新与渲染提交。
- 调度：按阶段划分系统；引入任务系统并行“只读集”。
- 变更：迭代期间避免结构修改，统一延迟；借助 observer/dispatcher 实现 UI/事件联动。
- 工具化：结合 meta 做 Inspector/序列化；用 resource_cache/自定义 ResourceManager 管理资源。

需要的话，我可以按本章处方直接为 HimiiEngine 生成最小可运行的 Scene/Entity 框架与示例 Systems，以便快速验证与迭代。

---

## 14. 借鉴 UE Mass：调度、读写集、热路径与表示层 LOD（落地到 EnTT）

目标：借鉴 Mass 的“分阶段 + 读写集 + Chunk 思维 + 表示层 LOD”，在不引入 UE 的前提下，用 EnTT 和少量基础设施获得相似的工程收益。

### 14.1 分阶段调度（Phases）

按帧内职责划分阶段，降低系统间隐式耦合：

```cpp
enum class Phase {
  PrePhysics,
  Simulation,    // 脚本/AI/移动/动画采样
  PostPhysics,
  RenderGather,  // 收集渲染数据
  RenderSubmit   // 提交到渲染后端
};

struct SystemFn { void (*fn)(entt::registry&, float); };
using Schedule = std::unordered_map<Phase, std::vector<SystemFn>>;

void RunSchedule(Schedule& sch, entt::registry& reg, float dt) {
  auto run = [&](Phase p){ for (auto s : sch[p]) s.fn(reg, dt); };
  run(Phase::PrePhysics);
  run(Phase::Simulation);
  run(Phase::PostPhysics);
  run(Phase::RenderGather);
  run(Phase::RenderSubmit);
}
```

Scene::OnUpdate 中调用 RunSchedule，系统按照阶段有序运行；并行化见下一节。

### 14.2 读/写集声明与无锁并行

Mass 的 Processor 会声明读取/写入的 Fragment 集合，据此自动并行。我们可做一版轻量声明，基于“读写集冲突检测”在同一 Phase 内并行执行：

```cpp
struct RWSet {
  std::unordered_set<std::type_index> reads;
  std::unordered_set<std::type_index> writes;
};

template<class...Ts> RWSet Read()  { return { {typeid(Ts)...}, {} }; }
template<class...Ts> RWSet Write() { return { {}, {typeid(Ts)...} }; }

inline bool Conflict(const RWSet& a, const RWSet& b) {
  auto inter = [&](auto &x, auto &y){ for (auto &t: x) if (y.count(t)) return true; return false; };
  return inter(a.writes, b.writes) || inter(a.writes, b.reads) || inter(a.reads, b.writes);
}

struct SystemDesc {
  SystemFn entry; RWSet rw; const char* name; Phase phase;
};

// 将同一 phase 的系统做“贪心着色”，把互不冲突的系统放到同一批次并行执行
void RunPhaseParallel(std::vector<SystemDesc>& systems, Phase phase,
                      entt::registry& reg, float dt) {
  std::vector<SystemDesc*> pool;
  for (auto &s: systems) if (s.phase == phase) pool.push_back(&s);
  while (!pool.empty()) {
    std::vector<SystemDesc*> batch; // 本批互不冲突
    for (auto it = pool.begin(); it != pool.end();) {
      bool ok = true; for (auto *b: batch) if (Conflict((*it)->rw, b->rw)) { ok = false; break; }
      if (ok) { batch.push_back(*it); it = pool.erase(it); } else { ++it; }
    }
    // 并行执行 batch（示意：可换为你的线程池）
    std::vector<std::future<void>> fs; fs.reserve(batch.size());
    for (auto *b: batch) fs.emplace_back(std::async(std::launch::async, [&]{ b->entry.fn(reg, dt); }));
    for (auto &f: fs) f.get();
  }
}
```

系统注册示例：

```cpp
void Movement(entt::registry&, float);
void SubmitSprites(entt::registry&, float);

std::vector<SystemDesc> systems = {
  { {Movement},      /*rw=*/Read<Transform, Velocity>(),          "Movement",      Phase::Simulation },
  { {SubmitSprites}, /*rw=*/Read<Transform, SpriteRenderer>(),     "SubmitSprites", Phase::RenderGather },
  // 需要写的系统用 Write<T...>() 或组合 Read+Write
};
```

注意：这是“数据冲突上的无锁并行”示例，不处理外部资源锁。真实项目应接入线程池、分区数据或 Job 系统。

### 14.3 热路径“Chunk”化：owning group 清单

Mass 的 Chunk 连续内存思路可用 EnTT 的 owning group 近似：预先规划热路径组合，提高顺序访问效率。

```cpp
// 渲染收集：Transform 连续，拉取 SpriteRenderer/其他只读数据
using RenderGroup = entt::basic_group<entt::entity, entt::get_t<SpriteRenderer /*...*/>, Transform>;

RenderGroup MakeRenderGroup(entt::registry& reg) {
  return reg.group<Transform>(entt::get<SpriteRenderer /*, More...*/>());
}

void SubmitSprites(entt::registry& reg, float /*dt*/) {
  auto g = MakeRenderGroup(reg);
  for (auto e : g) {
    auto &tr = g.get<Transform>(e);
    auto &sr = g.get<SpriteRenderer>(e);
    Himii::Renderer2D::DrawQuad({tr.position.x, tr.position.y}, {1,1}, sr.color);
  }
}
```

建议整理一份“热路径 owning group 清单”，避免在热区频繁变更这些组件组合。

### 14.4 Shared Fragment 思想的映射

Mass 的 Shared Fragment 可理解为“同批实体共享的只读参数”。在 EnTT 中可用：

- `registry.ctx<T>()` 存放全局/阶段共享参数；
- 或建立资源表 + 句柄，在组件内仅存 id（轻量、可复用）；
- 或给一批实体打同一 Tag，通过 Tag 在资源管理器查公共参数。

示例（ctx + 资源句柄）：

```cpp
struct CrowdParams { float avoidRadius = 1.2f; float maxSpeed = 3.0f; };
struct MaterialHandle { uint32_t id{}; };

// 初始化
reg.ctx().emplace<CrowdParams>(CrowdParams{1.1f, 2.5f});

// 组件仅存句柄
struct SpriteRenderer { MaterialHandle mat; glm::vec4 color{1}; };

// 系统读取共享参数
void CrowdAvoid(entt::registry& reg, float dt) {
  auto &p = reg.ctx<CrowdParams>();
  // 使用 p.avoidRadius / p.maxSpeed 进行行为计算
}
```

### 14.5 表示层 LOD 与稀疏表现

仅为“近处或需要可见”的小子集生成重型表示（渲染体/物理体），其余维持数据代理：

```cpp
struct LOD0{}; struct LOD1{}; struct Hidden{}; // Tag
struct Renderable{}; // 有该组件才会被渲染收集系统看到

void UpdateLOD(entt::registry& reg, float /*dt*/, const Camera& cam) {
  auto v = reg.view<Transform>(entt::exclude<Hidden>);
  for (auto e : v) {
    float d = distance_to_camera(v.get<Transform>(e), cam);
    bool needHeavy = d < 50.0f; // 近处
    if (needHeavy && !reg.any_of<Renderable>(e)) reg.emplace<Renderable>(e);
    if (!needHeavy && reg.any_of<Renderable>(e)) reg.remove<Renderable>(e);
  }
}
```

远处批量绘制可走“数据批处理路径”（实例化/间接绘制），而非逐实体的重系统。

### 14.6 大规模实体的实践要点（Mass 风格）

- 数据批：尽量按 SoA 顺序访问，减少随机跳转；
- 去虚表：避免逐实体虚函数调用，系统基于查询批处理；
- 桶式创建/销毁：集中在阶段边界应用，降低抖动；
- 观测与统计：追踪各系统处理实体数与耗时，指导 LOD/分组；
- 表达与数据分离：表示层可按需生灭，核心模拟保持稀疏轻量。

### 14.7 将并行接入 Scene（最小任务图示例）

```cpp
void Scene::OnUpdate(Timestep ts) {
  // Phase: Simulation（示例）
  RunPhaseParallel(systems_, Phase::Simulation, reg, (float)ts);
  RunPhaseParallel(systems_, Phase::PostPhysics, reg, (float)ts);
  RunPhaseParallel(systems_, Phase::RenderGather, reg, (float)ts);
  RunPhaseParallel(systems_, Phase::RenderSubmit, reg, (float)ts);
}
```

配合第 5 章的“延迟命令”策略，避免在同一遍历中结构性修改引发迭代失效。

---

小结：不迁移到 UE，也能借鉴 Mass 的方法论——用“分阶段 + 读写集 + owning group + LOD/共享参数”搭建稳定的并行与热路径体系。把这些实践接入本文第 6 章的 Scene/Entity 架构，即可在 HimiiEngine 中快速落地。
