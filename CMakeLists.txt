cmake_minimum_required(VERSION 3.25)

project(DX12ModernLab)

option(DX12_COMPAT_PROFILE "使用教程兼容编译配置" ON)
option(DX12_FORCE_UTF8_SOURCE "以 UTF-8 方式编译源码" ON)
option(DX12_ENGINE_USE_WINMAIN "以 Windows 子系统构建示例程序（入口为 WinMain）" ON)

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

# 统一输出目录，避免多目标项目产物分散在各子目录。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

message(STATUS "DX12_COMPAT_PROFILE = ${DX12_COMPAT_PROFILE}")
message(STATUS "CMAKE_CXX_STANDARD = ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_EXTENSIONS = ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "DX12_FORCE_UTF8_SOURCE = ${DX12_FORCE_UTF8_SOURCE}")
message(STATUS "DX12_ENGINE_USE_WINMAIN = ${DX12_ENGINE_USE_WINMAIN}")

# 添加示例程序的通用助手。
function(dx12_add_demo)
	set(options WIN32)
	set(oneValueArgs TARGET FOLDER)
	set(multiValueArgs SOURCES)
	cmake_parse_arguments(DX12_DEMO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(NOT DX12_DEMO_TARGET)
		message(FATAL_ERROR "dx12_add_demo 需要提供 TARGET 参数")
	endif()

	if(NOT DX12_DEMO_SOURCES)
		message(FATAL_ERROR "dx12_add_demo(${DX12_DEMO_TARGET}) 需要提供至少一个 SOURCES 文件")
	endif()

	if(DX12_DEMO_WIN32)
		add_executable(${DX12_DEMO_TARGET} WIN32 ${DX12_DEMO_SOURCES})
	else()
		add_executable(${DX12_DEMO_TARGET} ${DX12_DEMO_SOURCES})
	endif()

	target_link_libraries(${DX12_DEMO_TARGET} PRIVATE DX12Common)
	target_include_directories(${DX12_DEMO_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/Common)

	if(DX12_DEMO_FOLDER)
		set_target_properties(${DX12_DEMO_TARGET} PROPERTIES FOLDER "${DX12_DEMO_FOLDER}")
	endif()
endfunction()

add_subdirectory(Common)
add_subdirectory("Chapter 4 Direct3D Initialization")

