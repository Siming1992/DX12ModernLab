//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
// 这是DirectX 12应用程序的基础框架类实现文件
// 核心功能：封装DX12应用的窗口创建、设备初始化、渲染循环、消息处理等基础功能
//***************************************************************************************

#include "D3DApp.h"
#include <windowsx.h> // Windows消息处理辅助函数，如GET_X_LPARAM/GET_Y_LPARAM

// Microsoft::WRL::ComPtr：Windows Runtime C++ Template Library提供的智能指针
// 专门用于管理COM对象（DX12核心对象都是COM对象），自动处理引用计数，避免内存泄漏
using Microsoft::WRL::ComPtr;
using namespace std;     // 标准命名空间，简化std::前缀
using namespace DirectX; // DirectX数学库命名空间，包含XMMATRIX/XMVECTOR等数学类型

// 窗口过程函数（回调函数）：处理所有发送到窗口的Windows消息
// HWND：窗口句柄，UINT：消息类型，WPARAM/LPARAM：消息附加参数
LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 转发消息到D3DApp的MsgProc方法
    // 原因：WM_CREATE等消息可能在CreateWindow返回前触发，此时mhMainWnd还未初始化
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

// 静态成员变量初始化：指向唯一的D3DApp实例
// C++语法：类的静态成员必须在类外初始化，默认值为nullptr
D3DApp *D3DApp::mApp = nullptr;

// 获取全局唯一的D3DApp实例
// 静态成员函数：属于类本身，可通过类名直接调用，没有this指针
D3DApp *D3DApp::GetApp()
{
    return mApp;
}

// D3DApp构造函数：初始化应用程序实例
// HINSTANCE：应用程序实例句柄，Windows系统用于标识当前应用
D3DApp::D3DApp(HINSTANCE hInstance)
    : mhAppInst(hInstance) // 成员初始化列表：比构造函数体内赋值更高效
{
    // 断言：确保只创建一个D3DApp实例（单例模式）
    // assert：调试模式下生效，条件为false时触发断点，用于捕获逻辑错误
    assert(mApp == nullptr);
    mApp = this; // 将当前实例赋值给静态成员，实现全局访问
}

// D3DApp析构函数：释放资源
// 虚析构函数（基类析构函数通常声明为virtual）：确保子类析构时能正确调用基类析构
D3DApp::~D3DApp()
{
    // 如果D3D设备已创建，先刷新命令队列，确保所有GPU命令执行完毕
    // 为什么需要刷新命令队列？因为DX12是异步执行的，GPU可能还在处理之前提交的命令，如果直接释放资源可能会导致GPU访问已释放的资源，造成崩溃或未定义行为。
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

// 获取应用程序实例句柄（只读接口）
// const成员函数：承诺不修改类的成员变量，提高代码安全性
HINSTANCE D3DApp::AppInst() const
{
    return mhAppInst;
}

// 获取主窗口句柄（只读接口）
HWND D3DApp::MainWnd() const
{
    return mhMainWnd;
}

// 计算窗口的宽高比
float D3DApp::AspectRatio() const
{
    // static_cast<float>：显式类型转换，将整型转为浮点型，避免整数除法
    return static_cast<float>(mClientWidth) / mClientHeight;
}

// 获取4倍多重采样抗锯齿(MSAA)的启用状态
bool D3DApp::Get4xMsaaState() const
{
    return m4xMsaaState;
}

// 设置4倍MSAA状态
void D3DApp::Set4xMsaaState(bool value)
{
    // 只有状态变化时才执行后续操作，避免不必要的资源重建
    if (m4xMsaaState != value)
    {
        m4xMsaaState = value;

        // 重新创建交换链和缓冲区，应用新的多重采样设置
        // 重新创建交换链：因为多重采样设置是交换链缓冲区的一部分，必须重建交换链才能生效
        // 何时需要重建交换链：当多重采样状态改变时，交换链的缓冲区格式和采样描述需要更新，因此必须重建交换链。
        CreateSwapChain();
        OnResize();
    }
}

// 应用程序主循环：处理消息和渲染
int D3DApp::Run()
{
    // 初始化MSG结构体：存储Windows消息
    MSG msg = {0}; // 聚合初始化：将结构体所有成员置0

    // 重置计时器：初始化帧率统计相关变量
    mTimer.Reset();

    // 消息循环：直到收到WM_QUIT消息才退出
    while (msg.message != WM_QUIT)
    {
        // PeekMessage：非阻塞方式检查消息队列
        // PM_REMOVE：从消息队列中移除已处理的消息
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            // 翻译虚拟键消息（如键盘输入）
            TranslateMessage(&msg);
            // 将消息分发到窗口过程函数
            DispatchMessage(&msg);
        }
        // 无消息时执行游戏/渲染逻辑
        else
        {
            // 更新计时器：计算帧时间
            mTimer.Tick();

            // 应用未暂停时执行更新和渲染
            if (!mAppPaused)
            {
                // 计算并显示帧率统计
                CalculateFrameStats();
                // 更新游戏逻辑（由子类实现）
                Update(mTimer);
                // 渲染帧（由子类实现）
                Draw(mTimer);
            }
            else
            {
                // 应用暂停时休眠100ms，减少CPU占用
                Sleep(100);
            }
        }
    }

    // 返回退出码：WM_QUIT消息的wParam参数
    return (int)msg.wParam;
}

// 初始化函数：创建窗口和Direct3D设备
bool D3DApp::Initialize()
{
    // 初始化主窗口，失败则返回false
    if (!InitMainWindow())
    {
        return false;
    }
    // 初始化Direct3D设备，失败则返回false
    if (!InitDirect3D())
    {
        return false;
    }
    // 执行初始的窗口大小逻辑调整
    OnResize();

    return true;
}

// 创建渲染目标试图（RTV）和深度模版试图（DSV）描述符堆
// DX2核心概念：描述符堆是GPU资源视图（如RTV/DSV/SRV/UAV）的内存容器，存储资源的描述符信息，
// 相当于资源的“句柄”或“指针”，用于GPU访问资源。每种类型的描述符堆有不同的用途和访问权限。
// 何时需要创建RTV和DSV描述符堆？在初始化Direct3D设备时需要创建这些描述符堆，因为它们是存储渲染目标视图和深度模板视图描述符的必要组件。
// 没有这些描述符堆，就无法创建和管理RTV和DSV资源，导致无法进行渲染操作。
// 因此，在InitDirect3D函数中调用CreateRtvAndDsvDescriptorHeaps是必要的步骤，以确保后续的渲染流程能够正常进行。
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    // 初始化RTV描述符堆描述信息
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;   // RTV描述符数量，通常等于交换链缓冲区数量
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   // 描述符类型：渲染目标视图
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 描述符堆标志：无特殊标志
    rtvHeapDesc.NodeMask = 0;                            // 多GPU节点掩码，单GPU时为0

    // 创建RTV描述符堆，失败则抛出异常
    // IID_PPV_ARGS：安全的COM接口转换宏，将智能指针地址转换为正确的类型
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

    // 初始化DSV描述符堆描述信息
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;                      // DSV描述符数量，通常为1
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;   // 描述符类型：深度模板视图
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 描述符堆标志：无特殊标志
    dsvHeapDesc.NodeMask = 0;                            // 多GPU节点掩码，单GPU时为0

    // 创建DSV描述符堆，失败则抛出异常
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

// 窗口大小调整时的处理逻辑：重建交换链、深度缓冲区等资源
void D3DApp::OnResize()
{
    // 断言：确保设备、交换链、命令列表分配器已初始化
    assert(md3dDevice);
    assert(mSwapChain);
    assert(mDirectCmdListAlloc);

    FlushCommandQueue(); // 刷新命令队列，等待GPU完成所有命令，确保资源不被占用

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)); // 重置命令列表，准备记录新的命令

    for (int i = 0; i < SwapChainBufferCount; i++)
        mSwapChainBuffer[i].Reset(); // 释放交换链缓冲区资源，准备重新创建
    mDepthStencilBuffer.Reset();     // 释放深度模版缓冲区资源，准备重新创建

    // 调整交换链缓冲区大小
    ThrowIfFailed(mSwapChain->ResizeBuffers(
        SwapChainBufferCount,                     // 缓冲区数量
        mClientWidth, mClientHeight,              // 新的宽高
        mBackBufferFormat,                        // 缓冲区格式
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)); // 允许全屏/窗口模式切换

    mCurrBackBuffer = 0; // 重置当前后备缓冲区索引

    // 创建渲染目标试图（RTV）：将交换链缓冲区资源绑定到RTV描述符堆
    // CD3DX12_CPU_DESCRIPTOR_HANDLE: D3DX12库提供的CPU描述符句柄封装类，简化了描述符句柄的操作和计算
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < SwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));           // 获取交换链缓冲区资源
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle); // 创建渲染目标视图，关联到RTV描述符堆
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);                                           // 偏移到下一个RTV描述符位置
    }

    // 创建深度模版缓冲区资源：根据窗口大小和格式创建深度模版缓冲区
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 资源维度：2D纹理
    depthStencilDesc.Alignment = 0;                                  // 资源对齐，0表示默认
    depthStencilDesc.Width = mClientWidth;                           // 资源宽度
    depthStencilDesc.Height = mClientHeight;                         // 资源高度
    depthStencilDesc.DepthOrArraySize = 1;                           // 资源深度或数组大小，2D纹理为1
    depthStencilDesc.MipLevels = 1;                                  // MIP级别数量，深度模板通常为1

    // 修正 2016 年 12 月 11 日：SSAO 章节需要一个 SRV 来读取深度缓冲区。因此，由于我们需要为同一个资源创建两个视图：
    // 1. SRV 格式：DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    // 2. DSV 格式：DXGI_FORMAT_D24_UNORM_S8_UINT
    // 我们需要以无类型格式创建深度缓冲区资源。
    // 修正说明（2016/11/12）：SSAO章节需要读取深度缓冲区，因此需要创建无类型格式的资源
    // 原因：可以为同一资源创建不同格式的视图（SRV用R24_UNORM_X8_TYPELESS，DSV用D24_UNORM_S8_UINT）
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    // 多重采样设置：根据MSAA状态设置采样数和质量级别
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;           // 资源布局，由GPU自动管理，未知表示由驱动决定
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 资源标志：允许作为深度模板缓冲区使用

    // 深度/模板缓冲区清除值：默认深度1.0（最远），模板0
    // 如何优化：当资源被清除时，GPU可以使用优化路径，如果清除值与资源格式兼容且合理，可以显著提升清除性能
    D3D12_CLEAR_VALUE optClear;            // 优化清除值：为深度模板缓冲区提供优化的清除值，提升性能
    optClear.Format = mDepthStencilFormat; // 清除值格式，必须与资源格式兼容
    optClear.DepthStencil.Depth = 1.0f;    // 深度清除值，1.0表示最远
    optClear.DepthStencil.Stencil = 0;     // 模板清除值，通常为0

    // 创建深度模版缓冲区资源，使用优化清除值
    // CreateCommittedResource：创建一个提交的资源，直接分配在默认堆（GPU本地内存）上，适合频繁GPU访问的资源
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // CD3DX12_HEAP_PROPERTIES：辅助类，堆属性：默认堆，GPU本地内存
        D3D12_HEAP_FLAG_NONE,                              // 堆标志：无特殊标志
        &depthStencilDesc,                                 // 资源描述信息
        D3D12_RESOURCE_STATE_COMMON,                       // 初始资源状态：通用状态，后续会转换为DSV使用状态
        &optClear,                                         // 优化清除值，提升深度模板缓冲区的清除性能
        IID_PPV_ARGS(&mDepthStencilBuffer)));              // 输出资源指针

    // 创建深度模板视图（DSV），关联到DSV描述符堆
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc; // 深度模板视图描述信息
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;   // 视图标志：无特殊标志
    // 视图维度：根据MSAA状态选择多重采样或普通2D纹理
    dsvDesc.ViewDimension = m4xMsaaState ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat; // 视图格式，必须与资源格式兼容
    dsvDesc.Texture2D.MipSlice = 0;       // MIP级

    // 创建DSV视图，绑定到DSV描述符堆的起始位置
    // 为什么创建rtv的时候使用了GetCPUDescriptorHandleForHeapStart，而DSV时不用GetCPUDescriptorHandleForHeapStart()
    // 因为RTV描述符堆中有多个描述符（等于交换链缓冲区数量），需要通过GetCPUDescriptorHandleForHeapStart获取起始句柄，并通过Offset计算每个RTV的句柄位置。
    // 而DSV描述符堆中只有一个描述符，直接使用DepthStencilView()函数返回的句柄即可，无需计算偏移。
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    // 资源屏障：将深度缓冲区从COMMON状态转为DEPTH_WRITE状态
    // DX12核心概念：资源状态转换必须通过资源屏障显式声明，确保GPU正确同步资源访问，避免数据冲突和未定义行为。
    // 转换资源状态：将深度模板缓冲区从通用状态转换为深度写入状态，准备使用
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
                                                                           D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // 关闭命令列表：标记命令记录完成，准备执行
    ThrowIfFailed(mCommandList->Close());
    // 执行命令列表：将命令提交到命令队列，GPU开始执行
    ID3D12CommandList *cmdLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // 刷新命令队列：等待GPU完成所有命令，确保资源
    FlushCommandQueue();

    // 更新视口：覆盖整个客户端区域
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(mClientWidth);
    mScreenViewport.Height = static_cast<float>(mClientHeight);
    mScreenViewport.MinDepth = 0.0f; // 最小深度值
    mScreenViewport.MaxDepth = 1.0f; // 最大深度值

    // 更新裁剪矩形：与窗口大小一致
    mScissorRect = {0, 0, mClientWidth, mClientHeight};
}

// 消息处理函数：处理所有Windows消息
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // WM_ACTIVATE：窗口激活/失活消息
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            // 窗口失活：暂停应用，停止计时器
            mAppPaused = true;
            mTimer.Stop();
        }
        else
        {
            // 窗口激活：恢复应用，启动计时器
            mAppPaused = false;
            mTimer.Start();
        }
        return 0;

    // WM_SIZE：窗口大小改变消息
    case WM_SIZE:
        // 保存新的客户端区域尺寸
        mClientWidth = LOWORD(lParam);  // 低16位：宽度
        mClientHeight = HIWORD(lParam); // 高16位：高度
        if (md3dDevice)                 // DX12设备已初始化
        {
            if (wParam == SIZE_MINIMIZED)
            {
                // 窗口最小化：暂停应用
                mAppPaused = true;
                mMinimized = true;
                mMaximized = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                // 窗口最大化：恢复应用，调整资源
                mAppPaused = false;
                mMinimized = false;
                mMaximized = true;
                OnResize();
            }
            else if (wParam == SIZE_RESTORED)
            {
                // 窗口恢复（从最小化/最大化恢复）
                if (mMinimized)
                {
                    // 从最小化恢复
                    mAppPaused = false;
                    mMinimized = false;
                    OnResize();
                }
                else if (mMaximized)
                {
                    // 从最大化恢复
                    mAppPaused = false;
                    mMaximized = false;
                    OnResize();
                }
                else if (mResizing)
                {
                    // 用户正在拖动调整大小：暂不处理（避免频繁重建资源）
                }
                else
                {
                    // API调用导致的大小改变（如SetWindowPos）：调整资源
                    OnResize();
                }
            }
        }
        return 0;

    // WM_ENTERSIZEMOVE：用户开始拖动调整窗口大小
    case WM_ENTERSIZEMOVE:
        mAppPaused = true;
        mResizing = true;
        mTimer.Stop();
        return 0;

    // WM_EXITSIZEMOVE：用户停止拖动调整窗口大小
    case WM_EXITSIZEMOVE:
        mAppPaused = false;
        mResizing = false;
        mTimer.Start();
        OnResize();
        return 0;

    // WM_DESTROY：窗口销毁消息
    case WM_DESTROY:
        // 发送退出消息，结束消息循环
        PostQuitMessage(0);
        return 0;

    // WM_MENUCHAR：菜单激活时按下非快捷键
    case WM_MENUCHAR:
        // 处理Alt+Enter全屏切换时的蜂鸣音问题
        return MAKELRESULT(0, MNC_CLOSE);

    // WM_GETMINMAXINFO：获取窗口最小/最大尺寸
    case WM_GETMINMAXINFO:
        // 设置窗口最小尺寸：200x200
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = 200;
        return 0;

    // 鼠标按键按下消息
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    // 鼠标按键释放消息
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    // 鼠标移动消息
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    // 键盘按键释放消息
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            // 按ESC键退出应用
            PostQuitMessage(0);
        }
        else if ((int)wParam == VK_F2)
            // 按F2键切换4倍MSAA状态
            Set4xMsaaState(!m4xMsaaState);

        return 0;
    }

    // 未处理的消息：交给默认窗口过程处理
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 初始化主窗口
bool D3DApp::InitMainWindow()
{
    // 窗口类结构体：描述窗口的属性
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;                    // 窗口样式：水平/垂直重绘
    wc.lpfnWndProc = MainWndProc;                          // 窗口过程函数
    wc.cbClsExtra = 0;                                     // 类额外内存
    wc.cbWndExtra = 0;                                     // 窗口额外内存
    wc.hInstance = mhAppInst;                              // 应用实例句柄
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);               // 默认图标
    wc.hCursor = LoadCursor(0, IDC_ARROW);                 // 默认光标
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // 无背景刷
    wc.lpszMenuName = 0;                                   // 无菜单
    wc.lpszClassName = L"MainWnd";                         // 窗口类名

    // 注册窗口类
    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    // 计算窗口矩形尺寸（基于客户端区域大小）
    RECT R = {0, 0, mClientWidth, mClientHeight};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false); // 调整窗口大小以包含边框
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    // 创建窗口
    mhMainWnd = CreateWindow(
        L"MainWnd",                   // 窗口类名
        mMainWndCaption.c_str(),      // 窗口标题
        WS_OVERLAPPEDWINDOW,          // 窗口样式：重叠窗口
        CW_USEDEFAULT, CW_USEDEFAULT, // 窗口位置：默认
        width, height,                // 窗口大小
        0, 0,                         // 父窗口/菜单句柄
        mhAppInst,                    // 应用实例句柄
        0);                           // 附加参数
    if (!mhMainWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }

    // 显示窗口
    ShowWindow(mhMainWnd, SW_SHOW);
    // 更新窗口
    UpdateWindow(mhMainWnd);

    return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
    // 调试模式：启用DX12调试层（捕获API使用错误）
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    // 创建DXGI工厂（用于创建交换链等DXGI对象）
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

    // HRESULT：Windows API函数的返回类型，表示函数执行结果，S_OK表示成功，其他值表示错误
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,                    // 默认适配器（第一个可用GPU）
        D3D_FEATURE_LEVEL_11_0,     // 最低功能级别：支持DX11.0及以上的GPU
        IID_PPV_ARGS(&md3dDevice)); // 输出设备指针

    // 硬件设备创建失败：回退到WARP设备（软件渲染）
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&md3dDevice)));
    }

    // 创建围栏（用于同步GPU和CPU）
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    // 获取描述符句柄增量大小（每个描述符占用的字节数）
    // GetDescriptorHandleIncrementSize：根据描述符类型返回描述符堆中每个描述符占用的字节数，用于计算描述符句柄的偏移
    mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 检查4倍MSAA质量级别支持
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = mBackBufferFormat; // 后台缓冲区格式
    msQualityLevels.SampleCount = 4;            // 采样数：4
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
    // 调试模式：输出适配器信息
    LogAdapters();
#endif

    // 创建命令队列、命令分配器和命令列表
    CreateCommandObjects();
    // 创建交换链
    CreateSwapChain();
    // 创建RTV和DSV描述符堆
    CreateRtvAndDsvDescriptorHeaps();

    return true;
}

// 何时需要创建命令队列、命令分配器和命令列表？在初始化Direct3D设备时需要创建这些对象，因为它们是执行GPU命令的核心组件。
// 命令队列用于提交命令列表，命令分配器用于分配命令列表的内存，命令列表用于记录GPU命令。
// 没有这些对象，就无法向GPU发送渲染指令或进行资源管理操作。
// 因此，在InitDirect3D函数中创建这些对象是必要的步骤，以确保后续的渲染和资源管理能够正常进行。
void D3DApp::CreateCommandObjects()
{
    // 命令队列描述结构体：描述命令队列的属性
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 直接命令列表：支持所有类型的GPU命令
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // 命令队列标志：无特殊标志

    // 创建命令队列
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    // 创建命令分配器：每帧一个，用于分配命令列表的内存
    // 为什么命令分配器是每帧一个？因为每个命令分配器只能重置一次，重置后之前记录的命令列表必须执行完毕才能重置下一次。
    // 使用多个命令分配器可以实现多帧同时记录命令，提高CPU/GPU并行效率。
    // 这里为什么只创建一个命令分配器？因为在OnResize中会重置命令列表，确保所有命令执行完毕后再重置命令分配器，所以只需要一个即可。
    // 如果需要更复杂的多帧资源管理，可以创建多个命令分配器，每帧使用不同的命令分配器，避免等待GPU完成命令执行。
    // 什么时需要多个命令分配器？当应用程序需要同时记录多帧命令时（如双缓冲或三缓冲），每帧使用一个独立的命令分配器，可以避免等待GPU完成命令执行，提高效率。
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));

    // 创建命令列表：用于记录GPU命令，初始状态为未记录
    ThrowIfFailed(md3dDevice->CreateCommandList(
        0,                              // 0表示默认GPU节点
        D3D12_COMMAND_LIST_TYPE_DIRECT, // 直接命令列表：支持所有类型的GPU命令
        mDirectCmdListAlloc.Get(),      // 关联命令分配器
        nullptr,                        // 初始管线状态对象（PSO），这里为nullptr，后续会设置
        IID_PPV_ARGS(&mCommandList)));

    // 创建命令列表后默认处于记录状态，需要关闭，准备执行，第一次使用时需要Reset，后续每帧使用时需要Reset重置命令列表
    ThrowIfFailed(mCommandList->Close());
}

// 创建交换链（用于管理后台缓冲区和屏幕显示）
void D3DApp::CreateSwapChain()
{
    // 释放旧的交换链
    mSwapChain.Reset();

    // 描述交换链的属性
    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;   // 缓冲区宽度
    sd.BufferDesc.Height = mClientHeight; // 缓冲区高度
    // 刷新率设置：通常设置为60Hz，分子为60，分母为1，表示60帧每秒,
    // 为什么DXGI_SWAP_CHAIN_DESC中的刷新率是一个分数形式？
    // 因为某些显示器可能支持非整数的刷新率（如59.94Hz），使用分数形式可以更精确地表示这些刷新率，避免浮点数精度问题。
    sd.BufferDesc.RefreshRate.Numerator = 60;  // 刷新率分子
    sd.BufferDesc.RefreshRate.Denominator = 1; // 刷新率分母
    sd.BufferDesc.Format = mBackBufferFormat;  // 缓冲区格式
    // 扫描线是显示器刷新屏幕时逐行扫描的方式，扫描线顺序可以是从上到下（TOP_LEFT）或从下到上（BOTTOM_LEFT），未指定（UNSPECIFIED）表示由系统决定，通常为TOP_LEFT。
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // 扫描线顺序
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;                 // 缩放方式
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;                            // 采样数：根据MSAA状态设置
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;       // 质量级别：根据MSAA状态设置
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;                      // 缓冲区用途：渲染目标输出
    sd.BufferCount = SwapChainBufferCount;                                 // 缓冲区数量
    sd.OutputWindow = mhMainWnd;                                           // 输出窗口句柄
    sd.Windowed = true;                                                    // 窗口模式
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;                         // 交换效果：翻转丢弃，适合现代GPU
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;                     // 允许全屏/窗口模式切换

    // 创建交换链
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
        mCommandQueue.Get(), // 交换链需要关联命令队列，确保GPU命令执行与交换链同步
        &sd,                 // 交换链描述信息
        &mSwapChain));       // 输出交换链指针
}

// 刷新命令队列：等待GPU完成所有命令，确保资源安全
void D3DApp::FlushCommandQueue()
{
    // 增加当前围栏值，表示新的命令批次
    // 为什么增加当前围栏值？因为每次提交命令列表后都需要一个新的围栏值来标记这批命令，GPU执行完这批命令后会更新这个围栏值，CPU可以通过检查这个值来判断GPU是否完成了这批命令。
    // 围栏值是一个递增的计数器，每次提交命令列表时增加，GPU执行完命令后会更新这个值，CPU通过检查这个值来判断GPU是否完成了这批命令。
    // 不会超过UINT64的最大值，因为围栏值是一个递增的计数器，理论上可以持续增加，但实际应用中不会达到UINT64的上限。
    // 为什么实际应用中不会达到UINT64的上限？因为UINT64的最大值非常大，达到这个值需要提交极大量的命令，实际应用中几乎不可能达到。
    // 每帧提交的命令数量有限，通常在几十到几百条命令之间，即使每秒提交1000帧，每帧1000条命令，也需要数百万年才能达到UINT64的上限。
    mCurrentFence++;

    // 向命令队列添加围栏信号：GPU执行到此处时更新围栏值
    // 如果一帧之内提交多次命令列表，每次提交后都增加围栏值并添加信号，可以更精确地跟踪GPU执行状态，避免等待过多的命令完成，提高效率。
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
    // 等待GPU完成命令：如果GPU尚未执行到当前围栏值，等待事件
    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        // 创建事件句柄，用于等待GPU完成命令
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        // 当GPU执行到当前围栏值时触发事件
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        // 等待事件，直到GPU完成命令
        WaitForSingleObject(eventHandle, INFINITE);
        // 关闭事件句柄
        CloseHandle(eventHandle);
    }
}

// 获取当前后台缓冲区
// 后台缓冲区和前台缓冲区的作用是：前台缓冲区是当前显示在屏幕上的缓冲区，后台缓冲区是正在被渲染的缓冲区。交换链通常有多个后台缓冲区（如双缓冲或三缓冲），渲染器将图像渲染到当前后台缓冲区，然后通过交换链将其切换到前台显示。通过CurrentBackBuffer()函数可以获取当前正在渲染的后台缓冲区资源指针，方便后续设置渲染目标和执行渲染命令。
// 我们操作的永远都是后台缓冲区，因为前台缓冲区正在被显示器读取，直接操作会导致显示问题和性能下降。后台缓冲区则可以安全地进行渲染操作，完成后通过交换链切换到前台显示。
// 为什么需要CurrentBackBuffer()函数？因为交换链有多个后台缓冲区，当前使用哪个缓冲区由mCurrBackBuffer索引决定，
// 通过这个函数可以获取当前缓冲区的资源指针，方便后续操作（如设置渲染目标）。
ID3D12Resource *D3DApp::CurrentBackBuffer() const
{
    return mSwapChainBuffer[mCurrBackBuffer].Get();
}

// 获取当前后台缓冲区的RTV视图
// 后台缓冲期和后台缓冲区的RTV视图的关系是：每个后台缓冲区都有一个对应的RTV视图，RTV视图是用于渲染到该缓冲区的接口。
// 通过CurrentBackBuffer()函数获取当前后台缓冲区的资源指针，然后通过CurrentBackBufferView()函数获取对应的RTV视图句柄，
// 可以将渲染目标设置为当前后台缓冲区，实现渲染输出到屏幕。
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer,
        mRtvDescriptorSize);
}

// 获取深度模板视图(DSV)
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView() const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

// 计算并显示帧率统计信息
void D3DApp::CalculateFrameStats()
{
    // 静态变量：仅初始化一次，用于累计帧数和时间
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    frameCnt++;

    // 每1秒更新一次统计
    if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCnt; // 帧率 = 帧数 / 时间（1秒）
        float mspf = 1000.0f / fps;  // 每帧毫秒数

        // 转换为宽字符串（Windows API使用宽字符）
        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        // 拼接窗口标题：包含帧率和每帧耗时
        wstring windowText = mMainWndCaption +
                             L"    fps: " + fpsStr +
                             L"   mspf: " + mspfStr;

        // 设置窗口标题
        SetWindowText(mhMainWnd, windowText.c_str());

        // 重置统计值
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

// 输出适配器（显卡）信息（调试用）
void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter *adapter = nullptr;
    std::vector<IDXGIAdapter *> adapterList;
    // 枚举所有适配器
    while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        // 输出适配器名称
        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);

        ++i;
    }

    // 输出每个适配器的输出设备信息
    for (size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]); // 释放COM对象
    }
}

// 输出适配器的输出设备（显示器）信息
void D3DApp::LogAdapterOutputs(IDXGIAdapter *adapter)
{
    UINT i = 0;
    IDXGIOutput *output = nullptr;
    // 枚举所有输出设备
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        // 输出输出设备名称
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        // 输出显示模式
        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

// 输出输出设备的显示模式列表
void D3DApp::LogOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // 第一次调用：获取显示模式数量
    output->GetDisplayModeList(format, flags, &count, nullptr);

    // 分配内存存储显示模式列表
    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    // 遍历并输出所有显示模式
    for (auto &x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}
