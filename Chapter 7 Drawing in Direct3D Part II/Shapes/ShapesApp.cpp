
#include "ShapesApp.h"

// Windows程序主函数：替代控制台的main()，所有Windows桌面程序入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
    // 调试模式下开启内存泄漏检测：VC++调试工具，程序退出时报告泄漏
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        // 创建应用对象
        ShapesApp theApp(hInstance);
        // 初始化失败则退出
        if (!theApp.Initialize())
            return 0;
        // 运行消息循环+渲染循环
        return theApp.Run();
    }
    catch (DxException &e)
    {
        // 捕获DX异常，弹出错误提示框
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

// 析构函数：清空命令队列，确保GPU执行完所有命令再释放资源
ShapesApp::~ShapesApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

// 初始化应用：重写基类虚函数
bool ShapesApp::Initialize()
{
    // 先调用基类初始化：创建窗口、D3D设备、交换链、深度缓冲区等
    if (!D3DApp::Initialize())
        return false;

    // 重置命令列表：准备执行初始化命令（上传资源、创建管线等）
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // 按顺序构建所有DX12资源
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildRenderItems();
    BuildFrameResources();
    BuildDescriptorHeaps();
    BuildConstantBufferViews();
    BuildPSOs();

    // 关闭命令列表：结束录制
    ThrowIfFailed(mCommandList->Close());
    // 执行命令列表：将初始化命令提交给GPU
    ID3D12CommandList *cmdsLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 等待GPU完成初始化操作
    FlushCommandQueue();

    return true;
}

// 窗口大小改变时调用
void ShapesApp::OnResize()
{
    // 调用基类OnResize：重新创建交换链、深度缓冲区
    D3DApp::OnResize();

    // 重新计算透视投影矩阵
    // 参数：视场角FOV、宽高比、近裁剪面、远裁剪面
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    // 将XMMATRIX（SIMD寄存器类型）存储到XMFLOAT4X4（内存类型）
    XMStoreFloat4x4(&mProj, P);
}

// 每帧更新函数（CPU逻辑更新）
void ShapesApp::Update(const GameTimer &gt)
{
    // 处理键盘输入
    OnKeyboardInput(gt);
    // 更新相机矩阵
    UpdateCamera(gt);

    // 循环切换帧资源：0→1→2→0，实现3帧并行
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // 同步CPU与GPU：如果GPU还没处理完当前帧资源，CPU等待
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        // 创建事件对象
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        // 等待GPU到达指定围栏点
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        // 关闭事件句柄，释放系统资源
        CloseHandle(eventHandle);
    }

    // 更新所有物体的常量缓冲区
    UpdateObjectCBs(gt);
    // 更新全局Pass常量缓冲区
    UpdateMainPassCB(gt);
}

// 每帧绘制函数（GPU渲染执行）
void ShapesApp::Draw(const GameTimer &gt)
{
    // 获取当前帧的命令分配器
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // 重置命令分配器：复用内存，必须等GPU执行完之前的命令
    ThrowIfFailed(cmdListAlloc->Reset());

    // 重置命令列表：根据是否开启线框模式选择不同的PSO
    if (mIsWireframe)
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    }

    // 设置视口：渲染区域
    mCommandList->RSSetViewports(1, &mScreenViewport);
    // 设置裁剪矩形
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 资源屏障：将后台缓冲区从"呈现状态"转换为"渲染目标状态"
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 清空后台缓冲区（填充浅蓝色）
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    // 清空深度/模板缓冲区（深度值设为1.0，最远距离）
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 设置渲染目标：指定要绘制的缓冲区
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    // 设置描述符堆：告诉GPU使用哪个CBV堆
    ID3D12DescriptorHeap *descriptorHeaps[] = {mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 设置图形根签名
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // 计算Pass常量缓冲区的GPU描述符句柄
    int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
    // 绑定Pass CBV到根签名插槽1
    mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    // 绘制所有不透明物体
    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    // 资源屏障：将后台缓冲区转换回"呈现状态"，准备显示
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 关闭命令列表：录制完成
    ThrowIfFailed(mCommandList->Close());

    // 将命令列表提交到命令队列，GPU开始执行
    ID3D12CommandList *cmdsLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 交换前后台缓冲区：显示渲染结果
    ThrowIfFailed(mSwapChain->Present(0, 0));
    // 切换下一个后台缓冲区
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // 更新围栏值：标记当前帧命令完成
    mCurrFrameResource->Fence = ++mCurrentFence;

    // 命令队列发送信号：GPU执行完后更新围栏
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

#pragma region 鼠标键盘输入处理

// 鼠标按下事件
void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    // 记录当前鼠标位置
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    // 捕获鼠标：即使移出窗口也能接收鼠标消息
    SetCapture(mhMainWnd);
}

// 鼠标抬起事件
void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    // 释放鼠标捕获
    ReleaseCapture();
}

// 鼠标移动事件：控制相机旋转/缩放
void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    // 左键拖动：旋转相机
    if ((btnState & MK_LBUTTON) != 0)
    {
        // 计算鼠标偏移量，转换为弧度
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // 更新相机球坐标角度
        mTheta += dx;
        mPhi += dy;

        // 限制垂直角度，避免相机翻转
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    // 右键拖动：缩放相机（拉近/拉远）
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // 计算鼠标偏移
        float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

        // 更新相机半径
        mRadius += dx - dy;

        // 限制半径范围
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    }

    // 更新上一帧鼠标位置
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

// 键盘输入处理
void ShapesApp::OnKeyboardInput(const GameTimer &gt)
{
    // 判断按键'1'是否按下（0x8000表示按键按下状态）
    if (GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true; // 开启线框
    else
        mIsWireframe = false; // 关闭线框
}

#pragma endregion

// 更新相机：球坐标转笛卡尔坐标，生成观察矩阵
void ShapesApp::UpdateCamera(const GameTimer &gt)
{
    // 球坐标转三维坐标
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // 构建观察矩阵：相机位置 → 观察目标(原点) → 上方向(Y轴)
    XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
}

// 更新物体常量缓冲区
void ShapesApp::UpdateObjectCBs(const GameTimer &gt)
{
    // 获取当前帧的物体常量缓冲区
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    // 遍历所有渲染项
    for (auto &e : mAllRitems)
    {
        // 只有标记为脏数据才更新
        if (e->NumFramesDirty > 0)
        {
            // 加载世界矩阵
            XMMATRIX world = XMLoadFloat4x4(&e->World);

            ObjectConstants objConstants;
            // 矩阵转置：HLSL着色器要求列优先矩阵
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

            // 将数据复制到GPU常量缓冲区
            currObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // 脏标记减1，3帧后停止更新
            e->NumFramesDirty--;
        }
    }
}
