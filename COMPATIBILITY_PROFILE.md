# DX12 项目兼容配置说明

本文档用于说明：哪些项目配置会直接影响编译规则，以及为什么同一份代码在不同环境里会表现不一致。

## 当前开关

- `DX12_COMPAT_PROFILE`（顶层 CMake 选项）
  - ON：偏向教程兼容行为（更接近老项目默认设置）。
  - OFF：偏向现代严格行为。

- `DX12_FORCE_UTF8_SOURCE`（顶层 CMake 选项）
  - ON：为 MSVC 添加 `/utf-8`，以 UTF-8 方式解析源码与执行字符集。
  - OFF：编译器默认使用当前系统代码页。

- `DX12_ENGINE_USE_WINMAIN`（Engine 子项目 CMake 选项）
  - ON：`add_executable(Engine WIN32 ...)`，链接器期望入口为 `WinMain`/`wWinMain`。
  - OFF：`add_executable(Engine ...)`，链接器期望入口为 `main`/`wmain`。

## 规则影响映射

1. C++ 语言模式
- 由 `CMAKE_CXX_STANDARD` 控制。
- 影响：启用/禁用对应语言特性，并影响部分诊断行为。

2. 编译器扩展模式
- 由 `CMAKE_CXX_EXTENSIONS` 控制。
- 影响：ON 时允许编译器特有扩展。

3. MSVC 标准一致性严格度
- 由 `/permissive`（兼容模式）与默认严格行为共同体现。
- 影响：兼容模式下更容易接受部分旧式或非严格标准写法。

4. 预处理器兼容行为
- 由 `/Zc:preprocessor-` 控制。
- 影响：保留旧版宏展开行为，适合历史代码迁移。

5. 源码代码页处理
- 由 `/utf-8` 控制。
- 影响：减少对本地代码页的依赖，降低包含中文注释时触发 C4819 的概率。

6. 可执行子系统与入口函数
- 由 `add_executable` 是否使用 `WIN32` 关键字控制。
- 影响：
  - Windows 子系统 -> `WinMain`/`wWinMain`
  - Console 子系统 -> `main`/`wmain`

## 备注

- 某些硬性编译错误属于 C++ 语义层面问题，即使开启兼容模式也可能仍需修改源码。
- 兼容配置可以帮助项目更贴近教程环境，但不能保证接受所有不符合标准的历史写法。
