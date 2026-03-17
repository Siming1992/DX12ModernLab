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

# 将 Runtime 库链接到 Engine 可执行文件
# private 表示 Engine 依赖 Runtime，但 Runtime 不会暴露给 Engine 的用户（如果 Engine 是一个库的话）
# private和public的区别在于，public会将依赖关系传递给使用Engine的其他目标，而private则不会。
# 举例来说，如果 Engine 是一个库，并且它依赖 Runtime，那么如果使用 public，任何链接到 Engine 的目标也会自动链接到 Runtime；而如果使用 private，只有 Engine 自己会链接到 Runtime，其他链接到 Engine 的目标不会自动链接到 Runtime。
# 什么是链接到Engine的目标？链接到Engine的目标是指那些在CMake中通过target_link_libraries命令链接到Engine的其他库或可执行文件。例如，如果你有另一个可执行文件或库需要使用Engine提供的功能，那么你可能会在那个目标的CMakeLists.txt中使用target_link_libraries来链接到Engine。这样，那个目标就被认为是链接到Engine的目标。
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