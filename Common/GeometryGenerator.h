//***************************************************************************************
// GeometryGenerator.h 作者：Frank Luna (C) 2011 保留所有权利
//
// 定义一个【静态工具类】，用于程序化生成常见的3D几何体（立方体、球体、圆柱、网格等）
//
// 所有三角形都默认【朝外】生成（法线向外）。
// 如果你想让三角形【朝内】（例如把相机放在球体内模拟天空盒），需要：
//   1. 修改D3D剔除模式 或 手动反转三角形顶点顺序
//   2. 反转法线方向
//   3. 更新纹理坐标和切线向量
//***************************************************************************************

#pragma once

// 固定大小整数类型（如uint16_t, uint32_t），保证跨平台字节大小一致
#include <cstdint>
// DirectX数学库：向量、矩阵、几何计算核心头文件
#include <DirectXMath.h>
// C++ 动态数组容器：自动管理内存，用来存储顶点和索引
#include <vector>

// 几何体生成器类：静态工具类，专门用来生成立方体、球体、圆柱、网格等3D模型数据
class GeometryGenerator
{
public:
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

    // 结构体：顶点数据
    // DX12 作用：定义一个完整顶点包含的所有数据
    // C++ 语法：结构体 + 构造函数，用于快速创建顶点对象
    struct Vertex
    {
        // 为什么要特意实现空构造函数？
        // 有没有默认构造函数的区别：如果定义了空构造函数，编译器会使用它来初始化对象；
        // 如果没有定义，编译器会生成一个默认构造函数，但如果类中有其他构造函数，编译器将不会生成默认构造函数。
        Vertex() {}

        // 构造函数 1：接收 XMFLOAT 类型向量
        // 作用：用位置、法线、切线、纹理坐标快速构造顶点
        Vertex(
            const DirectX::XMFLOAT3 &p,    // 顶点位置
            const DirectX::XMFLOAT3 &n,    // 法线（光照计算用）
            const DirectX::XMFLOAT3 &t,    // 切线（法线贴图用）
            const DirectX::XMFLOAT2 &uv) : // 纹理坐标
                                           Position(p),
                                           Normal(n),
                                           TangentU(t),
                                           TexC(uv)
        {
        }

        // 构造函数 2：接收 float 类型数值
        // 作用：直接用数字创建顶点，更灵活
        Vertex(
            float px, float py, float pz, // 位置 xyz
            float nx, float ny, float nz, // 法线 xyz
            float tx, float ty, float tz, // 切线 xyz
            float u, float v) :           // 纹理坐标 uv
                                Position(px, py, pz),
                                Normal(nx, ny, nz),
                                TangentU(tx, ty, tz),
                                TexC(u, v)
        {
        }

        // 顶点位置：3D坐标 (x,y,z)
        DirectX::XMFLOAT3 Position;
        // 顶点法线：用于光照计算，决定明暗
        DirectX::XMFLOAT3 Normal;
        // 顶点切线：用于法线贴图，实现精细凹凸效果
        DirectX::XMFLOAT3 TangentU;
        // 纹理坐标：2D坐标 (u,v)，用于贴图片
        DirectX::XMFLOAT2 TexC;
    };

    // 结构体：网格数据
    // 作用：打包【一组顶点】+【一组索引】，代表一个完整3D模型
    struct MeshData
    {
        // 动态数组：存储所有顶点
        std::vector<Vertex> Vertices;
        // 动态数组：存储32位索引（绘制顺序）
        std::vector<uint32> Indices32;

        // 成员函数：获取16位索引列表
        // 作用：DX12支持16位索引（节省显存），自动从32位索引转换而来
        std::vector<uint16> &GetIndices16()
        {
            if (mIndices16.empty())
            {
                mIndices16.resize(Indices32.size());
                for (size_t i = 0; i < Indices32.size(); i++)
                {
                    // static_cast是C++中的一种类型转换操作符，用于执行明确的类型转换。
                    // 它比C风格的强制类型转换更安全，因为它提供了编译时类型检查，防止不安全的转换。
                    mIndices16[i] = static_cast<uint16>(Indices32[i]);
                }
            }
            return mIndices16;
        }

    private:
        // 作用：存储16位索引数据的缓存，GetIndices16()函数会根据需要将32位索引转换为16位索引并存储在这里。
        std::vector<uint16> mIndices16;
    };

    ///< summary>
    /// 创建一个【立方体】
    /// 中心在坐标原点，每个面可以细分多个小三角形
    /// 参数：宽度、高度、深度、细分次数
    ///</summary>
    MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

    ///< summary>
    /// 创建一个【球体】
    /// 参数：半径、径向分段数、纵向分段数
    ///</summary>
    MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

    ///< summary>
    /// 创建一个【二十面体球体】（更均匀的球体）
    /// 参数：半径、细分深度
    ///</summary>
    MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

    ///< summary>
    /// 创建一个【圆柱】（可做圆锥、圆台）
    /// 参数：底部半径、顶部半径、高度、圆周分段数、纵向分段数
    ///</summary>
    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

    ///< summary>
    /// 创建一个【地面网格】（常用于地形、地面）
    /// 在 XZ 平面上，m行n列
    ///</summary>
    MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

    ///< summary>
    /// 创建一个【屏幕四边形】
    /// 常用于后处理、UI、全屏特效
    ///</summary>
    MeshData CreateQuad(float x, float y, float w, float h, float depth);

    // C++ 语法：私有成员函数
    // 作用：类内部使用的工具函数，外部不能调用
private:
    // 细分几何体：把大三角形切成小三角形，让模型更平滑
    void Subdivide(MeshData &meshData);
    // 计算两个顶点的中点：细分时使用
    Vertex MidPoint(const Vertex &v0, const Vertex &v1);
    // 构建圆柱顶部盖子
    void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData &meshData);
    // 构建圆柱底部盖子
    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData &meshData);
};
