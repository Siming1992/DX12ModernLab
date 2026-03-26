//***************************************************************************************
// BoxApp.cpp (注释：原作者 Frank Luna (C) 2015 版权所有)
// 功能：演示如何在 Direct3D 12 中绘制一个立方体
// 操作控制：
//   - 按住鼠标左键并移动：旋转立方体
//   - 按住鼠标右键并移动：拉近/拉远视角（缩放）
//***************************************************************************************

// 包含必要的头文件
// ../../Common/ 路径下是DX12的通用工具类和辅助函数
#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

// 使用命名空间，避免每次都写完整命名空间前缀
// DirectX：包含DirectX数学库（XMFLOAT3/XMMATRIX等）
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// C++结构体定义：顶点结构体
// 用途：定义立方体的顶点数据格式（位置 + 颜色）
// DX12要求明确定义顶点布局，供输入汇编阶段使用
struct Vertex
{
    XMFLOAT3 Pos;   // 3D位置坐标 (XMFLOAT3：DirectX定义的3浮点数向量，对应x,y,z)
    XMFLOAT4 Color; // 顶点颜色 (XMFLOAT4：4浮点数向量，对应r,g,b,a)
};

// C++结构体定义：常量缓冲区结构体
// 用途：存储需要传递给着色器的常量数据（世界-视图-投影矩阵）
// 常量缓冲区是CPU向GPU着色器传递数据的主要方式
struct ObjectConstants
{
    // 世界视图投影矩阵（初始化为单位矩阵）
    // MathHelper::Identity4x4()：返回4x4单位矩阵，避免垃圾值
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

// C++类定义：BoxApp（继承自D3DApp基类）
// 用途：实现整个DX12应用程序的核心逻辑
// 继承语法：public D3DApp 表示公有继承，BoxApp可以使用D3DApp的公有/保护成员
class BoxApp : public D3DApp
{
public:
    // C++构造函数：初始化基类D3DApp，参数是应用程序实例句柄
    BoxApp(HINSTANCE hInstance);

    // C++11特性：删除拷贝构造函数和拷贝赋值运算符
    // 目的：防止对象被拷贝（DX12应用程序对象通常是单例，拷贝会导致资源管理混乱）
    BoxApp(const BoxApp &rhs) = delete;
    BoxApp &operator=(const BoxApp &rhs) = delete;

    // C++析构函数：释放资源（虽然ComPtr会自动释放，但显式声明是好习惯）
    ~BoxApp();

    // 虚函数重写：初始化应用程序（override关键字确保重写基类的虚函数）
    virtual bool Initialize() override;

private:
    // 虚函数重写：窗口大小改变时的处理
    virtual void OnResize() override;

    // 虚函数重写：更新游戏逻辑（每一帧调用）
    virtual void Update(const GameTimer &gt) override;

    // 虚函数重写：渲染画面（每一帧调用）
    virtual void Draw(const GameTimer &gt) override;

    // 虚函数重写：鼠标按下事件处理
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

    // 虚函数重写：鼠标抬起事件处理
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;

    // 虚函数重写：鼠标移动事件处理
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    // 私有成员函数：构建各种DX12资源（按功能拆分，代码更清晰）
    void BuildDescriptorHeaps();       // 构建描述符堆
    void BuildConstantBuffers();       // 构建常量缓冲区
    void BuildRootSignature();         // 构建根签名
    void BuildShadersAndInputLayout(); // 构建着色器和输入布局
    void BuildBoxGeometry();           // 构建立方体几何数据
    void BuildPSO();                   // 构建管线状态对象(PSO)

private:
    // DX12核心资源成员变量（全部私有，封装性原则）

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr; // 根签名：定义着色器资源绑定方式
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;      // 描述符堆：存储常量缓冲区视图（CBV）的描述符

    // 上传缓冲区（unique_ptr：C++11智能指针，独占所有权，自动释放）
    // UploadBuffer<ObjectConstants>：模板类，专门处理CPU到GPU的数据上传，ObjectConstants是模板参数，表示缓冲区中存储的数据类型
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr; // 常量缓冲区：存储对象常量数据（如世界-视图-投影矩阵）

    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr; // 网格几何数据：存储立方体的顶点和索引数据

    // 着色器字节码（ID3DBlob：存储二进制数据的接口，这里存储编译后的着色器代码）
    ComPtr<ID3DBlob> mvsByteCode = nullptr; // 顶点着色器字节码
    ComPtr<ID3DBlob> mpsByteCode = nullptr; // 像素着色器字节码

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; // 输入布局：定义顶点数据的格式和语义，供输入汇编阶段使用

    ComPtr<ID3D12PipelineState> mPSO = nullptr; // 管线状态对象：封装了渲染管线的所有状态设置，供绘制调用使用

    // 变换矩阵（存储世界、视图、投影矩阵）
    XMFLOAT4X4 mWorld = MathHelper::Identity4x4(); // 世界矩阵（物体在世界空间的位置）
    XMFLOAT4X4 mView = MathHelper::Identity4x4();  // 视图矩阵（摄像机位置和朝向）
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();  // 投影矩阵（透视投影）

    // 摄像机控制变量（球面坐标）
    float mTheta = 1.5f * XM_PI; // 水平旋转角度（初始值：1.5π，对应摄像机在后方）
    float mPhi = XM_PIDIV4;      // 垂直旋转角度（初始值：π/4，45度仰角）
    float mRadius = 5.0f;        // 摄像机到目标点的距离（初始值：5个单位）

    // 鼠标位置记录（POINT：Windows API结构体，存储x/y坐标）
    POINT mLastMousePos;
};
