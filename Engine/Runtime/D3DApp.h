
#include "Core/d3dUtil.h"
#include "Core/GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp &rhs) = delete;
    D3DApp &operator=(const D3DApp &rhs) = delete;
    virtual ~D3DApp();

private:
public:
};
