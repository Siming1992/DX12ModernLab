cmake_minimum_required(VERSION 3.25)

project(DX12ModernLab)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 使用宽字符集
if(WIN32)
	add_compile_definitions(UNICODE _UNICODE)
endif()

add_subdirectory(Engine)

