# 扩展指南

## 10.1 新增一个标准工具

以 `stdtool` 中已实现的 `StringProcessTool` / `WaitProcessTool` 为范例。

1. **创建文件**：在 `source/agile_vision/stdtool/` 下新增 `xxx_tool.h/.cpp`，遵循 MIT 文件头、`__xxx_tool_h__` guard、命名空间 `agv::stdtool`。
2. **继承基类**：`class AGV_STDTOOL_API XxxTool : public agv::core::Tool`。
3. **实现纯虚函数**：
   - `std::string getClsGuid() const override` — 返回唯一 GUID（用于 `ToolFactory::createTool` 的 clsid）。
   - `bool requestOutputData() override` — 实现算法，经 InputPin 读上游数据、写 OutputPin。
4. **构造函数中定义 Pin**：调用 `addPin(key, std::make_shared<...Pin>(...))`；若需子过程，调用 `autoCreateProcesManager()`。属性用 `PropPin`，输入用 `InputPin`，输出用 `OutputPin`。
5. **注册到 DLL**（插件形态）：在工具 DLL 中导出三个 C 符号供 `ToolFactory` 解析：
   - `GetToolClsID()` — 返回类 GUID 字符串
   - `CreateToolInstance(iid)` — `new XxxTool(iid)`
   - `DestroyToolInstance(tool*)` — `delete`
6. **接入构建**：源文件由 `stdtool/CMakeLists.txt` 的 GLOB 自动收集；工具 DLL 按 `.agvt` 命名放在 `test` 可执行文件目录供 `loadToolFile` 加载。

要点：Tool 基类构造时自动添加 `Status` OutputPin，`run()` 会先做 Pin 兼容校验再调 `requestOutputData()`，无需在派生类重复这些逻辑。

## 10.2 新增一个框架模块

1. 在 `source/agile_vision/` 下新建模块目录，添加 `CMakeLists.txt`。
2. 使用 `agv_add_module(agile_vision.<module>)`（如需额外源文件用 ARGN 传入），按依赖链接已有模块。
3. 定义 `<module>_export.h`，宏名 `AGV_<MODULE>_API`，开关 `AGV_<MODULE>_EXPORT`，Windows 下依据 `AGV_<MODULE>_EXPORT` 切换导出/导入。
4. 在根 `CMakeLists.txt` 的 `add_subdirectory` 序列中按依赖顺序加入（低层在前）。
5. 头文件 include 路径使用 `"agile_vision/<module>/..."`，与安装布局一致。
6. 命名空间按层归属：公共类型放 `agv` 根，业务类放 `agv::<module>`。

## 10.3 接入新的序列化后端

当前 `SerializePolicy` 的 `SERIALIZER_PB` 分支为空。接入 Protobuf 需：

1. 在 `package/CMakeLists.txt` 增加 `protobuf_generate()`，把 7 个 `.proto` 纳入目标并链接 protobuf 运行时；将 `AGV_REQUIRE_PROTOBUF` 设为 ON 或按需强制。
2. 实现领域对象 ↔ pb 消息的双向转换层（Solution/Procedure/Process/Tool/Pin/DataBuffer）。
3. 在 `serialize_policy.h` 的 `SERIALIZER_PB` 分支定义所需别名，并让各类的序列化函数体在新宏分支下提供实现。
4. 注意 Tool 实例由 `ToolFactory` 按 clsid 创建，反序列化时先建实例再回填属性；Pin 连接关系用 `pbProduceInfo`/`pbInputPin`/`pbOutputPin` 的字段恢复。

## 10.4 扩展数据流连接

若需调整 Pin 连接机制：

1. 连接入口是 `Tool::setPinConnection(consume_key, ProduceInfo)`，需保证 InputPin→OutputPin 引用与 OutputPin 侧 ConsumeInfo 双向一致。
2. 类型校验在 `Tool::checkPinDataCompatible()`，依赖 `DataSpec::compatibleWith()` 的三字段全等规则。
3. 若启用 `RelationshipNetwork` 图联动，需在连接建立/断开时同步 `makeRelationship/breakRelationship`，并用 `checkCycle()` 防止数据流成环。
4. 注意当前图网络与 Pin 连接尚未联动（旧代码被注释），这是待完成项。

## 10.5 代码规范与验证

- 编写前阅读 [编码规范](coding-standard.md) 与 [CODING_STANDARD.md](../../CODING_STANDARD.md)。
- 提交前跑 `python tools/style_check.py`，确认零违规（用法见 [质量工具链](quality-toolchain.md)）。
- 编译验证：configure → Debug/Release build → install，确认无 error/无警告。
- 提交信息一律使用中文（项目约定）。

## 10.6 常见陷阱

| 陷阱 | 说明 |
| --- | --- |
| 头文件写 `using namespace` | 规范禁止，会被检查器拦截；改用显式限定 |
| PinKey 误限定 | `PinKey` 在 `agv` 根，不在 `agv::core` |
| 导出宏与命名空间错配 | 类在 `agv::core` 但公共类型在 `agv` 根；特化需放在模板所在空间 |
| 直接调用 spdlog | 应使用 `LOG_*` 宏（级别配置例外） |
| 多配置用 `CMAKE_BUILD_TYPE` | 应使用 `$<CONFIG>` 生成器表达式 |
| vcpkg spdlog 与 `SPDLOG_USE_STD_FORMAT` 混用 | 会导致 LNK2001，vcpkg spdlog 用 fmt 后端，勿手定义该宏 |
