//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
// 译者注：这是Frank Luna编写的Direct3D初始化示例代码，用于演示DX12的基础初始化流程
//
// 演示了如何通过示例框架初始化Direct3D、清空屏幕并显示帧统计信息
//***************************************************************************************

#include "D3DApp.h"
// 引入DirectX颜色定义头文件（提供预定义的颜色常量），如Colors::LightSteelBlue
#include <DirectXColors.h>

using namespace DirectX;

// 定义InitDirect3DApp类，继承自D3DApp基类（C++类继承语法）
// D3DApp是封装了DX12应用程序通用逻辑的基类（窗口创建、设备初始化等）
class InitDirect3DApp : public D3DApp
{
public:
    // 构造函数：接收应用程序实例句柄，传递给基类构造函数
    // HINSTANCE：Windows API类型，表示应用程序实例句柄
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp();

    // 重写基类的Initialize方法（C++虚函数重写，override关键字显式声明）
    virtual bool Initialize() override;

private:
    // 重写基类的窗口大小改变处理函数
    // 目的：窗口尺寸变化时重新设置渲染视图、缓冲区等相关资源
    virtual void OnResize() override;

    // 重写基类的更新函数
    // 目的：处理每帧的逻辑更新（如输入、动画、物理等）
    // GameTimer& gt：引用传递的游戏计时器对象（避免拷贝，C++引用语法）
    virtual void Update(const GameTimer &gt) override;

    // 重写基类的绘制函数
    // 目的：实现每帧的渲染逻辑（核心渲染代码）
    virtual void Draw(const GameTimer &gt) override;
};
// Windows程序入口函数（替代标准C++的main函数）
// 参数说明：
// hInstance：当前应用实例句柄
// prevInstance：已废弃，始终为NULL
// cmdLine：命令行参数
// showCmd：窗口显示方式（最大化、最小化等）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
    // 调试模式下启用运行时内存检查（C++调试宏）
    // _CRTDBG_ALLOC_MEM_DF：启用内存分配调试
    // _CRTDBG_LEAK_CHECK_DF：程序退出时检查内存泄漏
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // try-catch异常处理（C++异常机制）
    // 目的：捕获DX12相关异常并友好提示
    try
    {
        // 创建应用程序对象（调用构造函数）
        InitDirect3DApp theApp(hInstance);

        // 初始化应用程序，失败则返回0
        if (!theApp.Initialize())
            return 0;

        // 运行应用程序主循环（基类D3DApp的Run方法）
        // 主循环：处理消息→更新逻辑→绘制画面→重复
        return theApp.Run();
    }
    // 捕获DX12异常（DxException是自定义的异常类）
    catch (DxException &e)
    {
        // 弹出消息框显示异常信息（Windows API）
        // e.ToString()：将异常信息转换为字符串
        // L"HR Failed"：宽字符串（Windows API要求）
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

// 构造函数实现
// 初始化列表：调用基类D3DApp的构造函数（C++初始化列表语法）
// 目的：先初始化基类成员，再初始化派生类成员
InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

// 析构函数实现
// 此处为空，因为基类D3DApp的析构函数会自动清理DX12资源
// C++析构函数调用规则：先执行派生类析构函数，再执行基类析构函数
InitDirect3DApp::~InitDirect3DApp()
{
}

// 初始化函数实现
bool InitDirect3DApp::Initialize()
{
    // 先调用基类的Initialize方法（初始化窗口、DX12设备等）
    // 失败则返回false
    if (!D3DApp::Initialize())
        return false;

    // 自定义初始化逻辑（此处暂无）
    return true;
}

// 窗口大小改变处理函数实现
void InitDirect3DApp::OnResize()
{
    // 调用基类的OnResize方法（重新设置交换链、渲染目标等）
    D3DApp::OnResize();
}

// 更新函数实现
void InitDirect3DApp::Update(const GameTimer &gt)
{
    // 此处暂无更新逻辑（仅演示基础框架）
    // 实际项目中：处理输入、更新游戏对象、计算动画等
}

// 绘制函数实现（DX12核心渲染逻辑）
void InitDirect3DApp::Draw(const GameTimer &gt)
{
    // 重置命令分配器（DX12核心概念）
    // 目的：复用命令分配器的内存，避免重复分配
    // 注意：只有当关联的命令列表在GPU上执行完成后才能重置
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // 重置命令列表（DX12核心概念）
    // 命令列表：记录要提交给GPU执行的渲染命令
    // 参数1：命令分配器（存储命令列表的内存）
    // 参数2：管线状态对象（PSO，此处为nullptr表示使用默认状态）
    // 目的：重新使用命令列表，记录新一帧的渲染命令
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // 资源状态转换（DX12核心概念）
    // 目的：将后台缓冲区从"显示状态"（PRESENT）转换为"渲染目标状态"（RENDER_TARGET）
    // DX12要求资源在使用前必须处于正确的状态
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 设置视口和裁剪矩形（DX12渲染设置）
    // 视口：定义渲染的区域（通常是整个窗口）
    // 裁剪矩形：定义哪些像素会被渲染（超出的会被裁剪）
    // 注意：命令列表重置后需要重新设置这些状态
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 清空后台缓冲区和深度模板缓冲区
    // ClearRenderTargetView：清空渲染目标（设置背景色）
    // 参数1：后台缓冲区视图
    // 参数2：清空颜色（LightSteelBlue：淡钢蓝色）
    // 参数3：要清空的矩形数量（0表示整个缓冲区）
    // 参数4：清空的矩形数组（nullptr表示整个缓冲区）
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);

    // ClearDepthStencilView：清空深度模板缓冲区
    // 参数1：深度模板缓冲区视图
    // 参数2：清空标志（同时清空深度和模板）
    // 参数3：深度值（1.0f表示最远）
    // 参数4：模板值（0）
    // 参数5-6：清空区域（整个缓冲区）
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 设置输出合并阶段的渲染目标（DX12渲染流水线）
    // 目的：告诉GPU接下来要渲染到哪个后台缓冲区和深度模板缓冲区
    // 参数1：渲染目标数量
    // 参数2：渲染目标视图数组
    // 参数3：是否使用深度模板缓冲区
    // 参数4：深度模板缓冲区视图
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    // 资源状态转换
    // 目的：将后台缓冲区从"渲染目标状态"转换回"显示状态"，准备显示到屏幕
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 关闭命令列表（DX12要求）
    // 目的：表示命令记录完成，不能再添加新命令，可以提交给GPU执行
    ThrowIfFailed(mCommandList->Close());

    // 将命令列表提交到命令队列（DX12核心流程）
    // 命令队列：负责将命令列表分发给GPU执行
    // _countof(cmdsLists)：C++宏，计算数组元素个数
    ID3D12CommandList *cmdsLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 交换后台缓冲区和前台缓冲区（DX12交换链）
    // 目的：将渲染好的后台缓冲区显示到屏幕上
    // 参数1：同步间隔（0表示立即显示）
    // 参数2：交换标志（0表示无特殊标志）
    ThrowIfFailed(mSwapChain->Present(0, 0));

    // 更新当前后台缓冲区索引（循环使用多个后台缓冲区）
    // SwapChainBufferCount：交换链的缓冲区数量（通常为2或3）
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // 等待命令队列执行完成（简化版，实际项目中应避免每帧等待）
    // 目的：确保GPU完成当前帧的渲染，避免资源访问冲突
    FlushCommandQueue();
}
