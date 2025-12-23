# okn-ecs (OmniKillerNexus ECS Module)

`okn-ecs` 提供以 Archetype/Chunk SoA 为主、可混合 Sparse Set 的实体组件系统。支持读写集推导与并行调度、层次 Transform、事件总线、快照/回滚、序列化、反射/脚本桥接、生命周期钩子、性能统计与调试视图，并对接 okn-memory（分配器）、okn-core 序列化与日志/剖析钩子。

## 主要特性
- 存储：Archetype/Chunk SoA + Sparse Set 混合；可配置 chunk 分配与对齐。
- 查询：Include/Exclude/Optional，Tag/Layer，Parent-Child 关系查询与视图。
- 调度：系统读写集推导，拓扑排序，多线程/Job System 并行切分。
- 层次：内置 Transform/Hierarchy，脏标记 + 拓扑更新。
- 事件：事件总线（瞬时/滞留）。
- 快照/回滚：帧同步/回合制的快照、回滚/重放接口。
- 序列化：对接 okn-core 序列化，选择性持久化组件。
- 反射/脚本：组件/系统注册表，动态注册，预留脚本（Lua/Python/JS）桥接。
- 生命周期：OnAdd/OnRemove/OnMove，构造/析构钩子。
- 可观测性：系统耗时、匹配数、实体/组件计数，调试视图接口。
- ID 安全：实体 ID 含世代号，防悬挂引用。
- 多世界：多 World 并存，克隆/迁移。
- CLI：生成代码/组件反射/快照检查占位。
- 内存：对接 okn-memory（池/arena/页）；可配置 chunk 对齐。
- 并发安全：读写锁/版本号防冲突，或依赖调度避免锁。

## 目录概要
```
include/okn/ecs/
  api.hpp config.hpp ecs_types.hpp entity.hpp entity_id.hpp component.hpp
  world.hpp world_builder.hpp
  archetype/ archetype.hpp chunk.hpp chunk_allocator.hpp chunk_view.hpp
  sparse/    sparse_set.hpp sparse_storage.hpp
  storage/   storage.hpp storage_registry.hpp
  query/     query.hpp filter.hpp relation.hpp view.hpp
  scheduler/ system.hpp system_graph.hpp scheduler.hpp job_adapter.hpp
  hierarchy/ transform.hpp hierarchy.hpp
  events/    event_bus.hpp event_queue.hpp
  snapshot/  snapshot.hpp rollback.hpp
  serialization/ serialize.hpp deserialize.hpp
  reflection/ registry.hpp component_info.hpp system_info.hpp
  lifecycle/ lifecycle.hpp
  metrics/   metrics.hpp debug_view.hpp
  scripting/ scripting_bridge.hpp
  memory/    alloc_hooks.hpp chunk_pools.hpp
  log/       log_hooks.hpp
  profile/   profile_hooks.hpp
  cli/       cli_commands.hpp
  ecs_export.hpp

src/  # 与头文件对应实现
samples/  # sample_basic.cpp, sample_hierarchy.cpp, sample_snapshot.cpp, sample_scheduler.cpp, sample_cli.cpp
tests/    # test_entity.cpp ... test_cli.cpp
CMakeLists.txt
```

## CMake 选项
- `BUILD_SAMPLES` (ON/OFF) 构建示例
- `BUILD_TESTS` (ON/OFF) 构建测试
- `OKN_ECS_ENABLE_SPARSE` (ON/OFF) 启用 Sparse Set 支持
- `OKN_ECS_ENABLE_SNAPSHOT` (ON/OFF) 启用快照/回滚
- `OKN_ECS_ENABLE_REFLECTION` (ON/OFF) 启用反射/注册表
- `OKN_ECS_ENABLE_SCRIPTING` (ON/OFF) 启用脚本桥接占位
- `OKN_ECS_ENABLE_CLI` (ON/OFF) 启用 CLI 功能
- `OKN_ECS_ENABLE_PROFILE_HOOKS` (ON/OFF) 启用剖析钩子
- `OKN_ECS_ENABLE_LOG_HOOKS` (ON/OFF) 启用日志钩子
- `OKN_ECS_ENABLE_MEMORY_HOOKS` (ON/OFF) 启用分配器钩子

## 构建
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j
```

## 集成要点
- 调度：系统声明读/写组件，调度器拓扑排序并切分任务，适配 okn-platform Job System。
- 存储：高频组件用 Archetype/Chunk，稀疏/低频组件可用 Sparse Set；chunk 对齐通过 config/memory。
- 序列化/快照：serialization + snapshot/rollback，结合 okn-core 序列化用于保存/重放/帧同步。
- 反射/脚本：reflection 注册表用于动态注册组件/系统，脚本桥接可在 scripting_bridge 中扩展。
- 可观测性：metrics/debug_view 暴露统计；profile/log hooks 接 okn-core 观测体系。