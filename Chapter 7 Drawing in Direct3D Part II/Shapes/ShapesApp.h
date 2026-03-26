//***************************************************************************************
// ShapesApp.cpp 作者：Frank Luna (C) 2015 保留所有权利
//
// 按住键盘数字键 '1' 可以将场景切换为线框模式显示
//***************************************************************************************

#pragma once

// 包含通用的D3D应用框架（封装了窗口、D3D设备、交换链等基础功能）
#include "../../Common/d3dApp.h"
// 包含数学工具类（矩阵、向量、角度转换等常用数学运算）
#include "../../Common/MathHelper.h"
// 包含上传缓冲区类（用于CPU向GPU上传数据的缓冲区封装）
#include "../../Common/UploadBuffer.h"
// 包含几何体生成器（用于生成立方体、球体、圆柱等基础3D模型）
#include "../../Common/GeometryGenerator.h"
// 包含帧资源头文件（每帧需要的GPU资源：常量缓冲区、命令分配器等）
#include "FrameResource.h"

// 使用微软WRL库的智能指针ComPtr，自动管理COM对象内存，避免内存泄漏
using Microsoft::WRL::ComPtr;
// 使用DirectX命名空间，简化向量、矩阵等数学API的调用
using namespace DirectX;
// 使用DirectX压缩向量命名空间
using namespace DirectX::PackedVector;

// 定义帧资源数量：DX12采用"帧重叠"技术，同时缓存3帧数据，让CPU和GPU并行工作
// 避免CPU等待GPU，提升渲染性能
const int gNumFrameResources = 3;

// 轻量级结构体：存储绘制一个3D物体所需的所有参数
// 不同应用的渲染参数不同，所以这个结构是自定义的
struct RenderItem
{
    // C++11语法：默认构造函数，编译器自动生成，不做任何初始化操作
    RenderItem() = default;

    // 世界矩阵：描述物体在3D世界中的位置、旋转、缩放
    // 将物体的局部坐标转换为世界坐标
    XMFLOAT4X4 World = MathHelper::Identity4x4();

    // 脏标记：表示物体数据已修改，需要更新常量缓冲区
    // 因为每个帧资源都有独立的物体常量缓冲区，所以需要更新全部3帧
    // 初始化为3，代表连续3帧都需要更新
    int NumFramesDirty = gNumFrameResources;

    // 物体常量缓冲区在GPU中的索引：对应每个物体的CBV（常量缓冲区视图）
    UINT ObjCBIndex = -1;

    // 指向几何体数据的指针：顶点缓冲区、索引缓冲区、子网格信息
    MeshGeometry *Geo = nullptr;

    // 图元拓扑类型：默认使用三角形列表（最常用的3D渲染图元）
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced（索引实例化绘制）的参数
    UINT IndexCount = 0;         // 索引数量
    UINT StartIndexLocation = 0; // 起始索引位置
    int BaseVertexLocation = 0;  // 基准顶点位置
};

// 主应用类：继承自D3DApp（通用D3D应用基类）
// C++继承语法：public表示公有继承，基类的公有成员变为派生类公有成员
class ShapesApp : public D3DApp
{
public:
    // 构造函数：接收窗口句柄HINSTANCE（Windows程序必备的实例句柄）
    ShapesApp(HINSTANCE hInstance);
    // C++语法：删除拷贝构造函数 → 禁止对象拷贝（单例模式思想，避免资源重复创建）
    ShapesApp(const ShapesApp &rhs) = delete;
    // C++语法：删除赋值运算符 → 禁止对象赋值
    ShapesApp &operator=(const ShapesApp &rhs) = delete;
    // 析构函数：释放资源
    ~ShapesApp();

    // C++语法：override关键字 → 显式重写基类的虚函数，编译器会检查是否匹配
    // 初始化函数：初始化D3D、资源、管线等所有内容
    virtual bool Initialize() override;

private:
    // 窗口大小改变时调用：更新投影矩阵
    virtual void OnResize() override;
    // 每帧更新函数：更新相机、常量缓冲区、物体状态
    virtual void Update(const GameTimer &gt) override;
    // 每帧绘制函数：执行DX12渲染命令
    virtual void Draw(const GameTimer &gt) override;

    // 鼠标事件重载函数
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override; // 鼠标按下
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;   // 鼠标抬起
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override; // 鼠标移动

    // 键盘输入处理：监听按键切换线框模式
    void OnKeyboardInput(const GameTimer &gt);
    // 更新相机：根据球坐标计算相机位置，生成观察矩阵
    void UpdateCamera(const GameTimer &gt);
    // 更新物体常量缓冲区：将CPU数据上传到GPU
    void UpdateObjectCBs(const GameTimer &gt);
    // 更新主pass常量缓冲区：传递视口、时间、矩阵等全局数据
    void UpdateMainPassCB(const GameTimer &gt);

    // 构建DX12资源的函数（初始化阶段一次性调用）
    void BuildDescriptorHeaps();       // 构建描述符堆（存储CBV/SRV/UAV视图）
    void BuildConstantBufferViews();   // 构建常量缓冲区视图
    void BuildRootSignature();         // 构建根签名（着色器与资源的桥梁）
    void BuildShadersAndInputLayout(); // 编译着色器+定义顶点输入布局
    void BuildShapeGeometry();         // 生成立方体、网格、球体、圆柱几何体
    void BuildPSOs();                  // 构建管线状态对象（渲染流水线配置）
    void BuildFrameResources();        // 构建帧资源（3组）
    void BuildRenderItems();           // 构建渲染项（所有3D物体）
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<RenderItem *> &ritems);

private:
    // C++：vector容器+unique_ptr智能指针 → 自动管理帧资源生命周期，安全释放内存
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource *mCurrFrameResource = nullptr; // 当前正在使用的帧资源
    int mCurrFrameResourceIndex = 0;             // 当前帧资源索引（0/1/2循环）

    // DX12根签名：定义着色器可以访问哪些资源
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    // 描述符堆：存放所有常量缓冲区视图(CBV)
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    // 着色器资源视图(SRV)描述符堆（本程序未使用，预留扩展）
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    // C++：unordered_map哈希表 → 键值对存储，通过字符串快速查找资源
    // 存储所有几何体数据
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    // 存储编译好的着色器二进制数据
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    // 存储管线状态对象(PSO)
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    // 顶点输入布局：告诉GPU顶点数据的格式（位置、颜色等）
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // 存储所有渲染项（所有3D物体）
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    // 按PSO分类的渲染项：所有不透明物体
    std::vector<RenderItem *> mOpaqueRitems;

    // 全局渲染常量（视图矩阵、投影矩阵、相机位置、时间等）
    PassConstants mMainPassCB;

    // Pass常量缓冲区在描述符堆中的偏移量
    UINT mPassCbvOffset = 0;

    // 是否开启线框模式标记
    bool mIsWireframe = false;

    // 相机位置
    XMFLOAT3 mEyePos = {0.0f, 0.0f, 0.0f};
    // 观察矩阵：相机视角
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    // 投影矩阵：3D转2D，产生透视效果
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    // 相机球坐标参数
    float mTheta = 1.5f * XM_PI; // 水平角度
    float mPhi = 0.2f * XM_PI;   // 垂直角度
    float mRadius = 15.0f;       // 相机距离原点的半径

    POINT mLastMousePos; // 记录上一帧鼠标位置，用于计算鼠标移动
};
