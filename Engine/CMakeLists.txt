option(DX12_ENGINE_USE_WINMAIN "以 Windows 子系统构建 Engine（入口为 WinMain）" ON)

if(DX12_ENGINE_USE_WINMAIN)
    add_executable(Engine WIN32
        InitDirect3DApp.cpp
    )
else()
    add_executable(Engine
        InitDirect3DApp.cpp
    )
endif()

message(STATUS "DX12_ENGINE_USE_WINMAIN = ${DX12_ENGINE_USE_WINMAIN}")

target_link_libraries(Engine
    PRIVATE
        Runtime
)

target_include_directories(Engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime
)

add_subdirectory(Runtime)


# 设置输出目录（推荐放在项目根目录的 bin 文件夹）
set_target_properties(Engine PROPERTIES
    # 可执行文件（.exe）
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    # 编译生成的库（如果有）
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
)