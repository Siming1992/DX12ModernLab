set(DX12_CH4_TARGET Chapter4_InitDirect3D)

if(DX12_ENGINE_USE_WINMAIN)
    dx12_add_demo(
        TARGET ${DX12_CH4_TARGET}
        WIN32
        FOLDER "Chapters"
        SOURCES
            InitDirect3DApp.cpp
    )
else()
    dx12_add_demo(
        TARGET ${DX12_CH4_TARGET}
        FOLDER "Chapters"
        SOURCES
            InitDirect3DApp.cpp
    )
endif()
