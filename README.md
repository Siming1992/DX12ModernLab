# DX12ModernLab

基于 DirectX 12 的学习项目，当前采用 CMake 统一管理：

- 根 CMake 负责全局编译选项与输出目录。
- Common 目录构建 DX12Common 静态库。
- 各章节示例通过 dx12_add_demo 注册为可执行目标。

## 当前项目结构

- Common: 通用框架代码，产物为 DX12Common.lib
- Chapter 4 Direct3D Initialization: 第 4 章示例，产物为 Chapter4_InitDirect3D.exe

## 环境要求

- Windows 10/11
- Visual Studio 2022（包含 C++ 桌面开发工具集）
- CMake 3.25+

## 快速开始（PowerShell）

在仓库根目录执行：

1. 配置项目

	cmake -S . -B build -G "Visual Studio 17 2022" -A x64

2. 构建全部目标（Debug）

	cmake --build build --config Debug

3. 仅构建第 4 章示例（可选）

	cmake --build build --config Debug --target Chapter4_InitDirect3D

4. 运行程序

	.\build\bin\Debug\Chapter4_InitDirect3D.exe

## 关键 CMake 选项

可在配置阶段通过 -D 传入：

- DX12_COMPAT_PROFILE=ON|OFF
  - ON: 教程兼容优先（C++17，MSVC 兼容行为）
  - OFF: 更现代严格（C++20，关闭编译器扩展）

- DX12_FORCE_UTF8_SOURCE=ON|OFF
  - ON: MSVC 添加 /utf-8，减少代码页问题

- DX12_ENGINE_USE_WINMAIN=ON|OFF
  - ON: 章节示例以 WIN32 子系统构建，入口为 WinMain
  - OFF: 以控制台子系统构建，入口应为 main/wmain

示例：

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDX12_COMPAT_PROFILE=ON -DDX12_FORCE_UTF8_SOURCE=ON -DDX12_ENGINE_USE_WINMAIN=ON

## 输出目录说明

由根 CMake 统一设置：

- 可执行文件: build/bin/<Config>/
- 静态库与动态库: build/lib/<Config>/

例如 Debug 下：

- build/bin/Debug/Chapter4_InitDirect3D.exe
- build/lib/Debug/DX12Common.lib

## 新增章节的推荐方式

1. 新建章节目录与其 CMakeLists.txt
2. 在根 CMake 中 add_subdirectory 该章节目录
3. 在章节 CMakeLists 中调用 dx12_add_demo，传入 TARGET 和 SOURCES

这样可以保证：

- 自动链接 DX12Common
- 自动继承统一输出目录
- 目标命名和组织保持一致