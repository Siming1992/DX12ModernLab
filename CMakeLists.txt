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
# 用法示例：
# dx12_add_demo(
#   TARGET Chapter4_InitDirect3D
#   WIN32
#   FOLDER "Chapters"
#   SOURCES InitDirect3DApp.cpp
# )
function(dx12_add_demo)
	# options: 仅有“出现/不出现”两种状态的开关参数。
	# 这里的 WIN32 出现时，生成 Windows 子系统程序（入口通常是 WinMain）。
	set(options WIN32)

	# oneValueArgs: 必须跟一个值的参数。
	# TARGET: 目标名；FOLDER: VS 里的分组名（可选）。
	set(oneValueArgs TARGET FOLDER)

	# multiValueArgs: 后面可跟多个值。
	# SOURCES: 源文件列表，可写一个或多个 .cpp。
	set(multiValueArgs SOURCES)

	# 解析传入参数：
	# - DX12_DEMO 是解析结果变量前缀。
	# - ${ARGN} 是调用此函数时传进来的“其余全部参数”。
	# 解析后会得到：DX12_DEMO_TARGET、DX12_DEMO_SOURCES、DX12_DEMO_WIN32 等变量。
	cmake_parse_arguments(DX12_DEMO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# 参数校验：TARGET 为空则直接在“配置阶段”报错并停止。
	if(NOT DX12_DEMO_TARGET)
		message(FATAL_ERROR "dx12_add_demo 需要提供 TARGET 参数")
	endif()

	# 参数校验：必须至少提供一个源文件。
	if(NOT DX12_DEMO_SOURCES)
		message(FATAL_ERROR "dx12_add_demo(${DX12_DEMO_TARGET}) 需要提供至少一个 SOURCES 文件")
	endif()

	# 根据 WIN32 开关创建可执行目标：
	# - ON: add_executable(... WIN32 ...)
	# - OFF: add_executable(...)
	if(DX12_DEMO_WIN32)
		add_executable(${DX12_DEMO_TARGET} WIN32 ${DX12_DEMO_SOURCES})
	else()
		add_executable(${DX12_DEMO_TARGET} ${DX12_DEMO_SOURCES})
	endif()

	# 链接通用库。PRIVATE 表示依赖只作用于当前目标，不向外传递。
	target_link_libraries(${DX12_DEMO_TARGET} PRIVATE DX12Common)

	# 添加 Common 头文件目录，保证章节代码可以直接包含通用头文件。
	target_include_directories(${DX12_DEMO_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/Common)

	# 若章节目录存在 Shaders，则自动收集并复制到运行目录。
	# 这样每个章节都能独立维护自己的 shader 资源。
	set(DX12_DEMO_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
	if(EXISTS "${DX12_DEMO_SHADER_DIR}")
		file(GLOB_RECURSE DX12_DEMO_SHADER_FILES CONFIGURE_DEPENDS
			"${DX12_DEMO_SHADER_DIR}/*.hlsl"
		)

		if(DX12_DEMO_SHADER_FILES)
			# 这些 shader 在运行时由 D3DCompileFromFile 编译，不参与 VS 的 FXC 构建流程。
			set_source_files_properties(${DX12_DEMO_SHADER_FILES}
				PROPERTIES
				HEADER_FILE_ONLY TRUE
				VS_TOOL_OVERRIDE "None"
			)
			target_sources(${DX12_DEMO_TARGET} PRIVATE ${DX12_DEMO_SHADER_FILES})
			source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DX12_DEMO_SHADER_FILES})
		endif()

		add_custom_command(TARGET ${DX12_DEMO_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${DX12_DEMO_TARGET}>/Shaders"
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${DX12_DEMO_SHADER_DIR}" "$<TARGET_FILE_DIR:${DX12_DEMO_TARGET}>/Shaders"
			COMMENT "Copying shader assets for ${DX12_DEMO_TARGET}"
		)
	endif()

	# 在 VS 调试时固定工作目录到可执行文件目录，避免相对路径受启动位置影响。
	if(MSVC)
		set_property(TARGET ${DX12_DEMO_TARGET} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${DX12_DEMO_TARGET}>")
	endif()

	# 可选：在 Visual Studio 解决方案中设置分组名称，便于管理多个章节目标。
	if(DX12_DEMO_FOLDER)
		set_target_properties(${DX12_DEMO_TARGET} PROPERTIES FOLDER "${DX12_DEMO_FOLDER}")
	endif()
endfunction()

add_subdirectory(Common)
add_subdirectory("Chapter 4 Direct3D Initialization")
add_subdirectory("Chapter 6 Drawing in Direct3D")
add_subdirectory("Chapter 7 Drawing in Direct3D Part II")

