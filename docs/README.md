# AgileVision 项目知识库

本知识库基于当前源码逐文件分析整理，面向新成员入门、日常开发和架构演进参考。所有结论以源码为准，未实现或存在疑问之处均显式标注。

## 文档地图

| 分区 | 文档 | 内容 |
| --- | --- | --- |
| 总览 | [项目总览](01-overview/project-overview.md) | 项目定位、技术栈、目录结构、快速上手 |
| 架构 | [系统架构](02-architecture/system-architecture.md) | 分层、模块依赖、构建目标、命名空间与导出宏 |
| 架构 | [领域模型](02-architecture/domain-model.md) | Solution → Procedure → Process → Tool → Pin 领域对象体系 |
| 架构 | [执行引擎与调度](02-architecture/execution-and-scheduling.md) | 运行入口、线程池、同步/异步、取消与等待 |
| 架构 | [数据流与引脚连接](02-architecture/dataflow-and-pins.md) | DataSpec/DataBuffer、三类 Pin、连接协议、类型兼容 |
| 架构 | [序列化体系](02-architecture/serialization.md) | 自定义二进制协议、Proxy 框架、持久化边界、Protobuf 规划 |
| 模块 | [basic](03-modules/basic.md) | 字节/字符串类型、日志、时间、并发、图结构 |
| 模块 | [geometry](03-modules/geometry.md) | ArrayX、Proxy 系列、几何类型 |
| 模块 | [system](03-modules/system.md) | 跨平台动态库加载 |
| 模块 | [core](03-modules/core.md) | 领域对象与引擎的实现细节 |
| 模块 | [stdtool](03-modules/stdtool.md) | 标准工具集（已实现与占位清单） |
| 模块 | [package](03-modules/package.md) | 打包模块与 7 个 Protobuf 契约 |
| 模块 | [test](03-modules/test.md) | 演示程序与运行流程 |
| 构建 | [构建与安装](04-build/build-and-install.md) | 预设、命令、产物布局 |
| 构建 | [依赖与环境](04-build/dependencies-and-environment.md) | vcpkg、spdlog、Qt、Protobuf 与本机路径 |
| 开发 | [编码规范](05-development/coding-standard.md) | `CODING_STANDARD.md` 摘要与落地方式 |
| 开发 | [质量工具链](05-development/quality-toolchain.md) | `tools/` 三个脚本的检查项与用法 |
| 开发 | [扩展指南](05-development/extension-guide.md) | 新增工具、模块、序列化后端的操作路径 |
| 参考 | [API 速查](06-reference/api-quick-reference.md) | 核心类型与接口的扁平索引 |
| 参考 | [术语表](06-reference/glossary.md) | 缩写、概念、命名约定 |
| 参考 | [已知问题与技术债](06-reference/known-issues.md) | 占位实现、疑似缺陷、演进方向 |

## 阅读路径建议

- **新人入门**：项目总览 → 系统架构 → 领域模型 → 执行引擎与调度 → 构建与安装。
- **开发插件工具**：领域模型 → 数据流与引脚连接 → stdtool → 扩展指南 → 编码规范。
- **维护序列化**：序列化体系 → geometry → package → 已知问题与技术债。
- **代码审查**：编码规范 → 质量工具链 → 已知问题与技术债。

## 维护约定

1. 本知识库随源码演进更新；新增模块或重大重构后，应同步修订对应文档与文档地图。
2. 描述以当前源码为事实来源；凡“未实现”“疑似缺陷”“待核实”均需在文末“已知问题与技术债”登记。
3. 文档使用中文；代码标识符、文件名、宏名保留原文以便检索。
4. 编码规范权威来源为仓库根目录 [`CODING_STANDARD.md`](../CODING_STANDARD.md)，本知识库只做索引与摘要，不复制全文。
