# API 速查

按使用频率整理的扁平索引。类名前缀均为 `agv::core::`（另行标注者除外）。

## 顶层容器

| 类型 | 头文件 | 关键接口 |
| --- | --- | --- |
| `Solution` | `core/solution.h` | `Solution(max_threads)`、`createProcedure(iid)`、`findProcedure`、`removeProcedure`、`run()`、`serializeToBytes/loadBytes` |
| `Procedure` | `core/procedure.h` | `mutableRoot()/root()`、`run()`、`setActiveRun`、`serializeToBytes/loadBytes` |
| `Process` | `core/process.h` | `appendTool/insertTool/removeTool/findTool`、`run()`、`setInterrupt`、`findChildProcess`、`getRootProcess` |
| `ProcessManager` | `core/process_manager.h` | `appendProcess/insertProcess`、`findProcess(iid, recursive)`、`removeProcess`、`getActiveProcess` |

## 执行引擎

| 类型 | 头文件 | 关键接口 |
| --- | --- | --- |
| `Engine` | `core/engine.h` | `commit(p, cb)`、`syncRunProcess(p)`、`get(p)`、`exist(w)`、`cancel(w, wait)`、`wait(w, timeout)` |
| `RunContext` | `core/run_context.h` | 字段：`engine`、`async_run`、`work_id`、`is_running`、`run_result` 等 |
| `ThreadPool` | `basic/thread_pool.h` | `ThreadPool(n)`、`enqueue(task)` |

## 工具与插件

| 类型 | 头文件 | 关键接口 |
| --- | --- | --- |
| `Tool` | `core/tool.h` | `run()`、`setPinConnection`、`getInputPin/getOutputPin/getPropPin`、`getClsGuid()=0`、`requestOutputData()=0`、`kStatusPinKey` |
| `ToolFactory` | `core/tool_factory.h` | `loadToolFile(path)`、`createTool(clsid, iid)`、`unloadToolFile`、`unloadAll` |
| `StringProcessTool` | `stdtool/string_process_tool.h` | 继承 Tool，字符串触发器 |
| `WaitProcessTool` | `stdtool/wait_process_tool.h` | 继承 Tool，等待异步过程 |

## Pin 与数据

| 类型 | 头文件 | 关键接口 |
| --- | --- | --- |
| `ToolPin` | `core/tool_pin.h` | `getPinType()=0`、`dataSpec/alias/deprecated` |
| `InputPin` | `core/input_pin.h` | `bindProduceInfo/unbind`、`produceInfo`、`optional` |
| `OutputPin` | `core/output_pin.h` | `mutableDataBuffer/dataBuffer`、`addConsumeInfo`、`consumeInfoList` |
| `PropPin` | `core/prop_pin.h` | 同时具备 DataBuffer 与 ProduceInfo；实现序列化 |
| `DataSpec` | `core/data_spec.h` | `Single*/StaticArray/DynamicArray`、`compatibleWith`、`isStaticArray/isDynamicArray` |
| `DataBuffer` | `core/data_buffer.h` | `setIntValue/getIntValue/...`、`resize`、`valueSize`、`lastModifyTime` |
| `ProduceInfo` | `core/produce_info.h` | `pin`、`data_location`、`isNull`、`operator bool` |
| `ConsumeInfo` | `core/consume_info.h` | `pin`、`operator bool/==` |
| `ToolLinkage` | `core/tool_linkage.h` | `producer`、`produce_pin_key`、`data_location`、`consumer`、`consume_pin_key`、`isValid` |

## 公共类型（`namespace agv`）

| 类型 | 头文件 | 说明 |
| --- | --- | --- |
| `DataType` | `core/core_typedef.h` | `kUnk/kInt/kFloat/kString/kBytes/kImage` |
| `PinType` | 同上 | `kInput/kOutput/kProp` |
| `PinKey` | 同上 | `std::string_view` |
| `wkid_t` / `kNullId` | 同上 | 工作 ID 类型与空值 |
| `AgvBytes` / `AgvString` | `basic/basic_typedef.h` | 字节向量 / 字符串 |
| `ImageData` | `basic/image_data.h` | 图像描述（packed） |
| `StringProxy` | `basic/string_proxy.h` | 字符串封装、序列化、UID、转换 |
| `Timestamp` / `TimeTracker` | `basic/time_util.h` | 原子计数 / 计时器 |
| `IdSeed<T>` | `basic/id_seed.hpp` | ID 生成器（非线程安全） |
| `DirectedGraph<V,A,K>` | `core/directed_graph.hpp` | 有向图，支持判环 |
| `AGV_LOG_*` 宏 / `GetLogger()` | `basic/log_service.h` | 统一日志门面 |
| `DynamicLibraryLoader` | `system/dynamic_library_loader.h` | 跨平台动态库加载 |

## 序列化代理（`namespace agv`，geometry）

| 类型 | 头文件 | 用途 |
| --- | --- | --- |
| `StringProxy` | `basic/string_proxy.h` | `[len][utf8]` |
| `ArrayX<T,N>` | `geometry/array_x.hpp` | 定长数组 memcpy |
| `VecProxy<E,Ref>` | `geometry/vec_proxy.hpp` | `[count][元素流]` |
| `DictProxy<K,V,Ref>` | `geometry/dict_proxy.hpp` | map 序列化 |
| `ProxyCombine<A,B,Ref>` | `geometry/proxy_combine.hpp` | 顺序拼接 |
| `ElementProxy<T,Ref>` | `geometry/element_proxy.hpp` | 标量包装 |
| `DefaultCreator<T>` | `geometry/default_creator.hpp` | 反序列化元素创建 |

## 常用模式片段

```cpp
// 创建方案与流程
agv::core::Solution solution(3);
auto* proc = solution.createProcedure(uid);

// 创建工具并挂到过程（工厂）
auto tool = factory.createTool(clsId, uid);
process->appendTool(tool);

// 设置属性 Pin 数据
auto* prop = tool->getPropPin("ImagePath");
prop->mutableDataBuffer().setStringValue(0, "ImageCalib.bmp");

// 连接 Pin：下游 InputPin 绑定上游 OutputPin
agv::core::ProduceInfo pi{upstreamOutputPin, 0};   // data_location = 0
downstreamTool->setPinConnection("Source", pi);

// 异步提交与等待
auto wid = engine.commit(process, [](agv::wkid_t id){ /* done */ });
auto result = engine.wait(wid, std::chrono::milliseconds(2000));
```
