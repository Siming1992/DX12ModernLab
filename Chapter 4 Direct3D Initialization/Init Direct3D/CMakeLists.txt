set(DX12_CH4_INIT_TARGET Chapter4_InitDirect3D)

if(DX12_ENGINE_USE_WINMAIN)
    dx12_add_demo(
        TARGET ${DX12_CH4_INIT_TARGET}
        WIN32
        FOLDER "Chapters/Chapter 4"
        SOURCES
            InitDirect3DApp.cpp
    )
else()
    dx12_add_demo(
        TARGET ${DX12_CH4_INIT_TARGET}
        FOLDER "Chapters/Chapter 4"
        SOURCES
            InitDirect3DApp.cpp
    )
endif()
