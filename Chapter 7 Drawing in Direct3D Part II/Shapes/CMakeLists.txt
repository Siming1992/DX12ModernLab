# 第7章 Shapes 子项目模板。

set(DX12_CH7_SHAPES_APP "${CMAKE_CURRENT_SOURCE_DIR}/ShapesApp.cpp")

set(DX12_CH7_SHAPES_TARGET Chapter7_Shapes)

if(DX12_ENGINE_USE_WINMAIN)
    dx12_add_demo(
        TARGET ${DX12_CH7_SHAPES_TARGET}
        WIN32
        FOLDER "Chapters/Chapter 7"
        SOURCES
            ShapesApp.cpp
            FrameResource.cpp
    )
else()
    dx12_add_demo(
        TARGET ${DX12_CH7_SHAPES_TARGET}
        FOLDER "Chapters/Chapter 7"
        SOURCES
            ShapesApp.cpp
            FrameResource.cpp
    )
endif()
