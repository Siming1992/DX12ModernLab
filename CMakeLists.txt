cmake_minimum_required(VERSION 3.25)

project(DX12ModernLab)

option(DX12_COMPAT_PROFILE "使用教程兼容编译配置" ON)
option(DX12_FORCE_UTF8_SOURCE "以 UTF-8 方式编译源码" ON)

if(DX12_COMPAT_PROFILE)
	# 兼容优先配置：尽量贴近老教程项目的默认行为。
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS ON)

	if(MSVC)
		# /permissive：放宽标准一致性检查。
		# /Zc:preprocessor-：保留旧版预处理器行为。
		add_compile_options(/permissive /Zc:preprocessor-)
	endif()
else()
	# 现代严格配置。
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(MSVC AND DX12_FORCE_UTF8_SOURCE)
	# 避免依赖系统代码页解析源码（可减少 C4819 警告）。
	add_compile_options(/utf-8)
endif()

# 使用宽字符集
if(WIN32)
	add_compile_definitions(UNICODE _UNICODE)
endif()

message(STATUS "DX12_COMPAT_PROFILE = ${DX12_COMPAT_PROFILE}")
message(STATUS "CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_EXTENSIONS = ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "DX12_FORCE_UTF8_SOURCE = ${DX12_FORCE_UTF8_SOURCE}")

add_subdirectory(Engine)

