
add_library(DX12Common STATIC
	D3DApp.cpp
	d3dUtil.cpp
	DDSTextureLoader.cpp
	GameTimer.cpp
)

target_include_directories(DX12Common
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
	# 减少 windows.h 引入的宏污染，避免与 std::min/std::max 冲突。
	target_compile_definitions(DX12Common PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

target_link_libraries(DX12Common
	PUBLIC
		d3d12
		d3d11
		dxgi
		d3dcompiler
		dxguid
)