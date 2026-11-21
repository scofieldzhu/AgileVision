# geometry 模块

自定义二进制序列化框架 + 基础几何类型。全部为头文件（`.h/.hpp`），无独立构建目标，随 basic 模块安装。

## 文件与职责

| 文件 | 核心类型 | 职责 |
| --- | --- | --- |
| `is_serializable.hpp` | concept `IsSerializable`、`VecMemberSerializable` | 序列化能力的编译期契约 |
| `default_creator.hpp` | `DefaultCreator<T>` / `DefaultCreator<T*>` | 反序列化时创建元素，指针特化返回 `new T()` |
| `element_proxy.hpp` | `ElementProxy<T, Ref>` | 包装单个标量（算术类型且 ≥4 字节） |
| `vec_proxy.hpp` | `VecProxy<E, Ref>` | 拥有/引用 `std::vector<E>` 的序列化 |
| `dict_proxy.hpp` | `DictProxy<K,V,Ref>` | 拥有/引用 `std::map<K,V>` 的序列化 |
| `proxy_combine.hpp` | `ProxyCombine<A,B,Ref>` | 两个可序列化代理的顺序拼接 |
| `array_x.hpp` | `ArrayX<T,S>` | 定长原生数组，直接 memcpy |
| `geometry_typedef.h` | `Point2/Point2f/Point3/Point3f` | `ArrayX<int/float,2/3>` 的别名 |
| `point.h` | — | 聚合头 |
| `line.hpp` | `Line<PointType>` | 线段，两端点 |
| `rect.hpp` | `Rect<T>` | 矩形：左上角、尺寸、旋转角 |
| `circle.hpp` | `Circle<T>` | 圆：圆心、半径 |
| `geometry.h` | — | 总聚合头，include 全部代理与实体 |
| `geometry_export.h` | `RATEL_GEOMETRY_API` | 导出宏 |

## Proxy 机制

核心思想：存储与序列化职责分离。`bool Ref` 参数切换两种特化：

- `Ref=false`：拥有数据，实现完整 `serializeToBytes/loadBytes`。
- `Ref=true`：持有成员引用，静态 `SerializeToBytes/LoadBytes` 委托给 `Ref=false` 实现，用于实体类把成员直接绑定进协议。

### VecProxy<E, Ref>

- 算术元素：`[uint32 数量][连续原始字节]`。
- `std::string` 元素：逐个 `StringProxy` 编码。
- 可序列化对象（含指针 `E*`）：逐个 `serializeToBytes` 拼接；加载用 `DefaultCreator<E>::Create()` 创建后 `loadBytes` 回填。
- 提供成对的 `mutableData()/data()`。

### DictProxy<K,V,Ref>

- `K` 限算术或 `string`，`V` 需满足 `VecProxyMember`。
- 序列化：拆成 keys/values 两个 `VecProxy` 字节流，头部 `[uint32 数量][uint32 keys 字节数]`。
- 反序列化：先恢复两个向量，再按下标 `insert` 重组（依赖 `std::map` 有序）。

### ProxyCombine<A,B,Ref>

- `Ref=false`：拥有 A、B 两个代理。
- `Ref=true`：持有 `A&`、`B&`，是 `Line`、`Circle` 直接绑定成员的关键。
- `SerializeToBytes` 拼接两流；`LoadBytes` 依次加载并返回总消费字节数。

## 几何类型

- `ArrayX<T,S>`：`kByteSize = sizeof(T)*S`；`operator[]` 越界抛 `std::out_of_range`；`toArray()` 返回 `std::array`。
- `Line<PointType>`：`p1_/p2_` 两端点，序列化用 `ProxyCombine<PointType,PointType>` 拼接。
- `Rect<T>`（T 限 int/float）：`lt_` 与 `size_` 为 `ArrayX<T,2>`，`angle_` 为 float；序列化顺序 lt → size → angle，手工 memcpy。
- `Circle<T>`：`center_` + `radius_`，用引用模式 `ProxyCombine` 绑定序列化。

## 使用方式

实体类通常组合这些 Proxy 来实现自己的持久化，例如 core 的 `Solution` 用 `VecProxy<Procedure*, true>` 引用流程指针数组，`DataSpec` 用 `ArrayX<unsigned int,3>` 打包三个字段。详见 [序列化体系](../02-architecture/serialization.md)。

> 注意：`Line::loadBytes` / `Circle::loadBytes` 使用值模式（Ref=false）的临时 ProxyCombine 反序列化，结果未回填到成员；`Circle::serializeToBytes` 构造引用模式 ProxyCombine 时传入的 radius 类型也存疑。详见 [已知问题](../06-reference/known-issues.md)。
