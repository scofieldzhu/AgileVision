# system 模块

跨平台动态库加载。命名空间 `agv`，**没有独立构建目标**——实现 `dynamic_library_loader.cpp` 由 basic 目标编译，头文件随 basic 安装。

## 文件与职责

| 文件 | 内容 |
| --- | --- |
| `system_typedef.h` | `DLL_HANDLE` 平台别名（Windows=`HMODULE`，Linux=`void*`，其他平台 `static_assert` 阻断） |
| `system_export.h` | `RATEL_SYSTEM_API` 导出宏 |
| `dynamic_library_loader.h` | `DynamicLibraryLoader` 类声明与模板 `getFunction` |
| `dynamic_library_loader.cpp` | 平台相关的 `load/unload` 实现 |

## DynamicLibraryLoader

接口：

| 接口 | 说明 |
| --- | --- |
| `bool load(const string& filename, string* err_detail = nullptr)` | 加载 DLL；先 unload 现有句柄，Windows `LoadLibrary` / Linux `dlopen(RTLD_LAZY)` |
| `template<typename ExportFunc> ExportFunc getFunction(const string& name) const` | 头文件内联：Windows `GetProcAddress` / Linux `dlsym`，失败返回 nullptr |
| `bool isLoaded() const` | 是否已加载 |
| `bool unload(string* err_detail = nullptr)` | 卸载并置句柄为 nullptr；未加载直接返回 true |
| 构造 / 析构 | 析构自动 `unload()` |

## 平台隔离

- 所有平台差异封装在 `.cpp` 的 `#ifdef PLATFORM_WIN` / `#ifdef PLATFORM_LINUX` 中，对外接口统一。
- 匿名命名空间 `getLastErrorMsg()`：Windows 用 `GetLastError + FormatMessageA`（`LocalFree` 释放），Linux 用 `dlerror()`。
- 不支持平台用 `THROW_EXCEPTION_OF_METHOD_NOT_IMPL()` 抛 `std::domain_error`。

## 典型使用：ToolFactory

`core/tool_factory.cpp` 使用本加载器解析工具 DLL 的三个导出符号：

- `GetToolClsID` — 取工具类 ID
- `CreateToolInstance` — 创建工具实例
- `DestroyToolInstance` — 销毁工具实例（作为 shared_ptr 删除器）

详见 [领域模型](../02-architecture/domain-model.md) 的工厂与插件生命周期章节。

> 已修复：`dynamic_library_loader.h` 的 Linux 分支 `dlsym(handle_, func_name.c_str())` 曾缺少右括号，现编译通过。
