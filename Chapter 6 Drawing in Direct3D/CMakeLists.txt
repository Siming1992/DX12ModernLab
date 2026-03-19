set(DX12_CH6_TARGET Chapter6_BoxApp)

# 第6章示例：根据全局开关选择 WinMain 或 main 子系统。
if(DX12_ENGINE_USE_WINMAIN)
	dx12_add_demo(
		TARGET ${DX12_CH6_TARGET}
		WIN32
		FOLDER "Chapters"
		SOURCES
			BoxApp.cpp
	)
else()
	dx12_add_demo(
		TARGET ${DX12_CH6_TARGET}
		FOLDER "Chapters"
		SOURCES
			BoxApp.cpp
	)
endif()
