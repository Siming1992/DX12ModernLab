set(DX12_CH6_BOX_TARGET Chapter6_BoxApp)

if(DX12_ENGINE_USE_WINMAIN)
    dx12_add_demo(
        TARGET ${DX12_CH6_BOX_TARGET}
        WIN32
        FOLDER "Chapters/Chapter 6"
        SOURCES
            BoxApp.cpp
    )
else()
    dx12_add_demo(
        TARGET ${DX12_CH6_BOX_TARGET}
        FOLDER "Chapters/Chapter 6"
        SOURCES
            BoxApp.cpp
    )
endif()
