#include "BoxApp.h"

// WinMain函数：Windows应用程序入口点（替代控制台程序的main函数）
// 参数说明：
//   hInstance：应用程序实例句柄
//   prevInstance：已废弃，始终为NULL
//   cmdLine：命令行参数
//   showCmd：窗口显示方式（最大化/最小化/正常）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
    // 调试模式下启用内存泄漏检测（C++调试宏）
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // C++异常处理：捕获DX12相关异常
    try
    {
        // 创建应用程序对象
        BoxApp theApp(hInstance);

        // 初始化应用程序，失败则退出
        if (!theApp.Initialize())
            return 0;

        // 运行应用程序主循环
        return theApp.Run();
    }
    // 捕获DX12异常并显示错误信息
    catch (DxException &e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

// BoxApp类构造函数实现
// 参数：hInstance - 应用程序实例句柄
// 目的：调用基类D3DApp的构造函数初始化窗口和DX12设备
BoxApp::BoxApp(HINSTANCE hInstance)
    : D3DApp(hInstance) // C++初始化列表：调用基类构造函数
{
    // 为什么不在构造函数体内调用基类构造函数？
    // 因为在C++中，基类构造函数必须在派生类构造函数的初始化列表中调用，不能在构造函数体内调用。
    // 这样可以确保基类部分在派生类对象创建之前正确初始化，避免资源管理和对象状态不一致的问题。
    //  通过初始化列表调用基类构造函数，确保D3DApp的成员变量（如窗口句柄、DX12设备等）在BoxApp对象创建时正确初始化，
    //  为后续的DX12资源创建和渲染操作提供基础。
}

// BoxApp类析构函数实现
// 目的：释放资源（这里主要依靠智能指针自动释放，所以为空）
BoxApp::~BoxApp()
{
}

// BoxApp类成员函数实现：Initialize
// 目的：初始化应用程序资源，构建DX12管线所需的各种资源（描述符堆、常量缓冲区、根签名、着色器、几何数据、PSO等）
bool BoxApp::Initialize()
{
    // 先调用基类的初始化函数（初始化窗口、DX12设备、交换链等）
    if (!D3DApp::Initialize())
        return false;

    // 重置命令列表：准备记录初始化命令（如资源创建等）
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // 按顺序构建所有DX12资源
    BuildDescriptorHeaps();       // 构建描述符堆
    BuildConstantBuffers();       // 构建常量缓冲区
    BuildRootSignature();         // 构建根签名
    BuildShadersAndInputLayout(); // 构建着色器和输入布局
    BuildBoxGeometry();           // 构建立方体几何数据
    BuildPSO();                   // 构建管线状态对象

    // 关闭命令列表（表示命令记录完成）
    ThrowIfFailed(mCommandList->Close());

    // 执行命令列表（将初始化命令提交给GPU执行）
    ID3D12CommandList *cmdsLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists); //_countof(cmdsLists)：计算命令列表数组的元素数量，这里是1

    // 等待GPU完成初始化命令的执行（确保资源创建完成）
    FlushCommandQueue();

    return true;
}

// OnResize函数实现：窗口大小改变时的处理
// 目的：更新投影矩阵（窗口宽高比变化会影响透视投影）
void BoxApp::OnResize()
{
    // 先调用基类的OnResize（更新交换链、深度缓冲区等）
    D3DApp::OnResize();

    // 窗口大小改变，更新宽高比并重新计算投影矩阵
    // XMMatrixPerspectiveFovLH：创建左手坐标系的透视投影矩阵
    // 参数说明：
    //   0.25f*MathHelper::Pi：垂直视野角（45度）
    //   AspectRatio()：窗口宽高比
    //   1.0f：近裁剪面（距离摄像机1单位内的物体不可见）
    //   1000.0f：远裁剪面（距离摄像机1000单位外的物体不可见）
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

    // XMStoreFloat4x4：将XMMATRIX（SIMD矩阵）转换为XMFLOAT4X4（普通矩阵）存储
    XMStoreFloat4x4(&mProj, P);
}

// Update函数实现：每一帧更新游戏逻辑
// 参数：gt - 游戏计时器（提供帧时间、总时间等）
// 目的：更新摄像机位置和常量缓冲区数据
void BoxApp::Update(const GameTimer &gt)
{
    // 球面坐标转笛卡尔坐标：计算摄像机的3D位置
    // mTheta：水平角度，mPhi：垂直角度，mRadius：距离
    float x = mRadius * sinf(mPhi) * cosf(mTheta); // X坐标
    float z = mRadius * sinf(mPhi) * sinf(mTheta); // Z坐标
    float y = mRadius * cosf(mPhi);                // Y坐标

    // 构建视图矩阵（摄像机矩阵）
    // XMVectorSet：创建4D向量（x,y,z,w），w=1.0表示位置向量
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);         // 摄像机位置
    XMVECTOR target = XMVectorZero();                  // 观察目标点（原点）
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 摄像机上方向（Y轴）

    // XMMatrixLookAtLH：创建左手坐标系的视图矩阵
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view); // 存储视图矩阵

    // 计算世界-视图-投影组合矩阵（WVP矩阵）
    // XMLoadFloat4x4：将XMFLOAT4X4转换为XMMATRIX进行矩阵运算
    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj; // 矩阵乘法（注意顺序：世界*视图*投影）

    // 更新常量缓冲区：将WVP矩阵传递给GPU
    ObjectConstants objConstants;
    // XMMatrixTranspose：转置矩阵（DX12着色器中矩阵是列主序，需要转置）
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    // CopyData：将CPU数据复制到上传缓冲区（GPU可访问）
    mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer &gt)
{
    // 重置命令分配器（重用命令内存，提高效率）
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // 重置命令列表，关联到PSO（管线状态对象）
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    // 设置视口和裁剪矩形（定义渲染区域）
    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 资源状态转换：将后台缓冲区从展示状态转为渲染目标状态
    // CD3DX12_RESOURCE_BARRIER::Transition：创建资源转换屏障
    // 何时需要资源状态转换？在DX12中，资源必须处于正确的状态才能被GPU访问。
    // 渲染前需要将后台缓冲区转换为渲染目标状态，渲染完成后再转换回展示状态，以确保GPU能够正确访问资源进行渲染和显示。
    // 什么操作属于渲染操作？设置渲染目标、清除渲染目标和深度缓冲区、设置描述符堆和根签名、设置输入装配数据、绘制调用等都属于渲染操作。
    mCommandList->ResourceBarrier(1,
                                  &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 设置渲染目标：指定要渲染到的后台缓冲区和深度缓冲区
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    // 设置描述符堆：告诉GPU使用哪个描述符堆
    ID3D12DescriptorHeap *descriptorHeaps[] = {mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 设置根签名：告诉GPU使用哪个根签名
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // 设置输入装配阶段数据：顶点缓冲区和索引缓冲区
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());        // 设置顶点缓冲区
    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());                 // 设置索引缓冲区
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 设置图元类型（三角形列表）

    // 设置根参数：常量缓冲区视图(CBV)
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // 绘制索引化图元：渲染立方体
    // DrawIndexedInstanced参数：
    //   索引数量、实例数量、起始索引、顶点偏移、起始实例
    mCommandList->DrawIndexedInstanced(
        mBoxGeo->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);

    // 资源状态转换：将后台缓冲区从渲染目标状态转为展示状态
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 关闭命令列表（渲染命令记录完成）
    ThrowIfFailed(mCommandList->Close());

    // 执行命令列表：将渲染命令提交给GPU
    ID3D12CommandList *cmdsLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 交换前后缓冲区：将渲染好的后台缓冲区显示到屏幕
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // 等待GPU完成当前帧的渲染命令（简单实现，效率较低，仅用于演示）
    FlushCommandQueue();
}

#pragma region 鼠标事件处理
// OnMouseDown函数实现：鼠标按下事件处理
// 参数：
//   btnState：鼠标按键状态（MK_LBUTTON/MK_RBUTTON等）
//   x,y：鼠标位置坐标
// 目的：记录鼠标按下时的位置，捕获鼠标
void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    // 记录鼠标按下时的位置
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    // 捕获鼠标：让鼠标输入只发送给当前窗口
    SetCapture(mhMainWnd);
}

// OnMouseUp函数实现：鼠标抬起事件处理
// 目的：释放鼠标捕获
void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    // 释放鼠标捕获
    ReleaseCapture();
}

// OnMouseMove函数实现：鼠标移动事件处理
// 目的：根据鼠标移动更新摄像机参数（旋转/缩放）
void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    // 左键按下：旋转摄像机（改变角度）
    if ((btnState & MK_LBUTTON) != 0)
    {
        // 计算鼠标移动的差值，并转换为弧度（0.25度/像素）
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // 更新旋转角度
        mTheta += dx; // 水平角度
        mPhi += dy;   // 垂直角度

        // 限制垂直角度范围（防止摄像机翻转）
        // MathHelper::Clamp：将值限制在最小值和最大值之间
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    // 右键按下：缩放（改变摄像机距离）
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // 计算鼠标移动的差值（0.005单位/像素）
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // 更新摄像机距离
        mRadius += dx - dy;

        // 限制距离范围（3-15单位）
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    // 更新最后鼠标位置
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
#pragma endregion

// 构建描述符堆：创建一个描述符堆，用于存储常量缓冲区视图（CBV）的描述符，并使其对着色器可见
void BoxApp::BuildDescriptorHeaps()
{
    // 描述符堆描述结构体
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;                                // 描述符数量（1个CBV）
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;     // 描述符类型（CBV/SRV/UAV）
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 着色器可见（GPU可访问）
    cbvHeapDesc.NodeMask = 0;                                      // 多GPU节点掩码（单GPU设为0）

    // 创建描述符堆
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
                                                   IID_PPV_ARGS(&mCbvHeap)));
}

// BuildConstantBuffers函数实现：构建常量缓冲区
// 目的：创建CPU到GPU的上传缓冲区，并创建CBV描述符
void BoxApp::BuildConstantBuffers()
{
    // 创建上传缓冲区（大小：1个ObjectConstants，可动态更新）
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

    // 计算常量缓冲区的字节大小（DX12要求常量缓冲区大小为256字节的倍数）
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    // 获取缓冲区的GPU虚拟地址
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    // 计算第0个常量缓冲区的偏移地址（这里只有1个，偏移为0）
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objCBByteSize;

    // 常量缓冲区视图描述结构体
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;                                                 // 缓冲区GPU地址
    cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants)); // 缓冲区大小

    // 创建常量缓冲区视图(CBV)，并存储到描述符堆中
    md3dDevice->CreateConstantBufferView(
        &cbvDesc,
        mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

// 构建根签名：定义着色器可以访问的资源（这里是1个CBV）
void BoxApp::BuildRootSignature()
{
    // 根签名的作用：定义着色器程序的输入资源（类似函数参数）

    // -------------------------- 步骤1：定义根参数（对应HLSL的单个资源需求） --------------------------
    // 根参数 = 着色器的单个资源入口（这里只有1个：CBV）
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // -------------------------- 步骤2：定义描述符范围（指定HLSL的寄存器绑定） --------------------------
    // 描述符范围 = 告诉DX12：这个资源是哪种类型、占几个寄存器、从哪个寄存器开始
    // 创建描述符表（包含1个CBV）
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    // Init参数详解（直接对应HLSL的cbuffer）：
    // 1. D3D12_DESCRIPTOR_RANGE_TYPE_CBV：资源类型是常量缓冲区（对应HLSL的cbuffer）
    // 2. 1：数量是1个CBV（我们只有1个cbPerObject）
    // 3. 0：从b0寄存器开始（对应HLSL的register(b0)）
    // Init参数：描述符类型、数量、基寄存器
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    // -------------------------- 步骤3：将根参数关联到描述符范围 --------------------------
    // InitAsDescriptorTable：表示这个根参数是一个「描述符表」（管理多个描述符的集合）
    // 参数：1个描述符范围（&cbvTable） → 对应我们的1个CBV
    // 作用：把“根参数0”和“b0寄存器的CBV”绑定起来
    // 将根参数初始化为描述符表
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // -------------------------- 步骤4：定义根签名描述（汇总所有资源需求） --------------------------
    // 根签名描述 = 把所有根参数打包，告诉DX12“这是着色器需要的所有资源”
    // 根签名描述结构体
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        1,                 // 根参数数量
        slotRootParameter, // 根参数数组
        0,                 // 静态采样器数量
        nullptr,           // 静态采样器数组
        // 根签名标志：允许输入汇编阶段使用输入布局
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // -------------------------- 步骤5：序列化根签名（转换为GPU能识别的格式） --------------------------
    // 根签名描述是“人类可读的结构”，需要转成二进制数据（序列化）才能给GPU用
    // 序列化根签名（将根签名描述转换为GPU可识别的二进制格式）
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    // 如果序列化失败，输出错误信息
    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char *)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    // -------------------------- 步骤6：创建根签名对象（最终生效） --------------------------
    // 用序列化后的二进制数据，创建DX12的根签名对象（mRootSignature）
    // 后续PSO会绑定这个根签名，告诉GPU“按这个规则匹配着色器资源”
    // 创建根签名对象
    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

// BuildShadersAndInputLayout函数实现：构建着色器和输入布局
// 目的：编译着色器代码，定义顶点输入布局
void BoxApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    // 编译顶点着色器和像素着色器
    // d3dUtil::CompileShader：自定义函数，编译HLSL着色器代码
    // 参数：着色器文件路径、宏定义、着色器入口点、着色器模型
    // vs_5_0：顶点着色器模型5.0，ps_5_0：像素着色器模型5.0
    // 着色器模型5.0是DX12的最低要求，支持更高级的着色器功能和更大的资源绑定空间
    mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    // 定义顶点输入布局（与Vertex结构体对应）
    mInputLayout =
        {
            // POSITION：顶点位置，格式R32G32B32_FLOAT（3个32位浮点数）
            // 偏移0字节，每顶点数据，实例数据步长0
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            // COLOR：顶点颜色，格式R32G32B32A32_FLOAT（4个32位浮点数）
            // 偏移12字节（3*4），每顶点数据
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
}

void BoxApp::BuildBoxGeometry()
{
    // 定义立方体的8个顶点（每个顶点包含位置和颜色）
    std::array<Vertex, 8> vertices =
        {
            Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),  // 顶点0：白色
            Vertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),  // 顶点1：黑色
            Vertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),    // 顶点2：红色
            Vertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),  // 顶点3：绿色
            Vertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),   // 顶点4：蓝色
            Vertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}), // 顶点5：黄色
            Vertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),   // 顶点6：青色
            Vertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)}) // 顶点7：品红
        };

    // 定义立方体的索引数据（36个索引，12个三角形，每个面2个三角形）
    std::array<std::uint16_t, 36> indices =
        {
            // 前面
            0, 1, 2,
            0, 2, 3,

            // 背面
            4, 6, 5,
            4, 7, 6,

            // 左面
            4, 5, 1,
            4, 1, 0,

            // 右面
            3, 2, 6,
            3, 6, 7,

            // 顶面
            1, 5, 6,
            1, 6, 2,

            // 底面
            4, 0, 3,
            4, 3, 7};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    // 创建MeshGeometry对象，管理立方体几何数据
    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "boxGeo";

    // 创建CPU端顶点缓冲区（存储顶点数据）
    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    // 创建CPU端索引缓冲区（存储索引数据）
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // 创建GPU端顶点缓冲区（默认堆，GPU可读写）
    // d3dUtil::CreateDefaultBuffer：将CPU数据复制到GPU默认堆
    mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                                                            mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

    // 创建GPU端索引缓冲区
    mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
                                                           mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

    // 设置MeshGeometry的元数据
    mBoxGeo->VertexByteStride = sizeof(Vertex);  // 每个顶点的字节大小
    mBoxGeo->VertexBufferByteSize = vbByteSize;  // 顶点缓冲区总大小
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT; // 索引格式（16位无符号整数）
    mBoxGeo->IndexBufferByteSize = ibByteSize;   // 索引缓冲区总大小

    // 定义子网格信息（立方体作为一个子网格）
    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size(); // 索引数量
    submesh.StartIndexLocation = 0;            // 起始索引位置
    submesh.BaseVertexLocation = 0;            // 基准顶点位置

    // 将子网格信息添加到MeshGeometry中（键值对："box"对应立方体）
    mBoxGeo->DrawArgs["box"] = submesh;
}

// BuildPSO函数实现：构建管线状态对象(PSO)
// 目的：封装整个渲染管线的状态（输入布局、着色器、光栅化、混合等）
void BoxApp::BuildPSO()
{
    // 管线状态描述结构体
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)); // 初始化为0

    // 设置输入布局
    psoDesc.InputLayout = {mInputLayout.data(), (UINT)mInputLayout.size()};

    // 设置根签名
    psoDesc.pRootSignature = mRootSignature.Get();

    // 设置顶点着色器
    psoDesc.VS =
        {
            reinterpret_cast<BYTE *>(mvsByteCode->GetBufferPointer()), // 着色器字节码指针
            mvsByteCode->GetBufferSize()                               // 字节码大小
        };

    // 设置像素着色器
    psoDesc.PS =
        {
            reinterpret_cast<BYTE *>(mpsByteCode->GetBufferPointer()), // 着色器字节码指针
            mpsByteCode->GetBufferSize()                               // 字节码大小
        };

    // 设置光栅化状态（默认状态：启用深度测试、背面剔除等）
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    // 设置混合状态（默认状态：禁用混合，使用源颜色）
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // 设置深度模板状态（默认状态：启用深度测试，禁用模板测试）
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // 采样掩码（全部启用）
    psoDesc.SampleMask = UINT_MAX;

    // 图元拓扑类型（三角形）
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // 渲染目标数量（1个）
    psoDesc.NumRenderTargets = 1;

    // 渲染目标格式（与后台缓冲区格式一致）
    psoDesc.RTVFormats[0] = mBackBufferFormat;

    // 多重采样设置（根据4xMSAA状态调整）
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

    // 深度模板缓冲区格式
    psoDesc.DSVFormat = mDepthStencilFormat;

    // 创建管线状态对象
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}