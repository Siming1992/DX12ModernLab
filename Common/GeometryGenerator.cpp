//***************************************************************************************
// GeometryGenerator.cpp 作者：Frank Luna (C) 2011 保留所有权利
// 功能：实现几何体生成器的所有方法，生成立方体、球体、圆柱、网格、四边形等3D模型
//***************************************************************************************

// 包含头文件，使用类定义、结构体、函数声明
#include "GeometryGenerator.h"
// C++ 标准库算法，用于 std::min（取最小值）
#include <algorithm>

// 使用 DirectX 命名空间，避免每次写 DirectX::XMFLOAT3 这么长
using namespace DirectX;

//=====================================================================================
// 函数：创建立方体
// 参数：宽度、高度、深度、细分次数（细分越多表面越平滑）
// 返回：包含顶点和索引的 MeshData 模型数据
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateBox(
    float width, float height, float depth, uint32 numSubdivisions)
{
    // 创建网格数据对象，用于存储最终生成的顶点和索引
    MeshData meshData;

    //--------------------------
    // 步骤1：创建 8个顶点 × 6面 = 24个顶点
    // 注：立方体6个面，每个面4个顶点，互不共享（法线不同）
    //--------------------------
    Vertex v[24]; // 定义24个顶点的数组

    // 立方体半宽、半高、半深（让立方体中心在原点）
    float w2 = 0.5f * width;
    float h2 = 0.5f * height;
    float d2 = 0.5f * depth;

    // 前面 4 个顶点
    v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // 后面 4 个顶点
    v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // 上面 4 个顶点
    v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // 下面 4 个顶点
    v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // 左面 4 个顶点
    v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

    // 右面 4 个顶点
    v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    // C++ 语法：vector::assign → 将数组数据复制到 vector 中
    // 作用：把上面定义的24个顶点存入 meshData
    meshData.Vertices.assign(&v[0], &v[24]);

    //--------------------------
    // 步骤2：创建索引（36个索引 = 6面 × 2三角形 × 3顶点）
    //--------------------------
    uint32 i[36];

    // 前面索引
    i[0] = 0;
    i[1] = 1;
    i[2] = 2;
    i[3] = 0;
    i[4] = 2;
    i[5] = 3;

    // 后面索引
    i[6] = 4;
    i[7] = 5;
    i[8] = 6;
    i[9] = 4;
    i[10] = 6;
    i[11] = 7;

    // 上面索引
    i[12] = 8;
    i[13] = 9;
    i[14] = 10;
    i[15] = 8;
    i[16] = 10;
    i[17] = 11;

    // 下面索引
    i[18] = 12;
    i[19] = 13;
    i[20] = 14;
    i[21] = 12;
    i[22] = 14;
    i[23] = 15;

    // 左面索引
    i[24] = 16;
    i[25] = 17;
    i[26] = 18;
    i[27] = 16;
    i[28] = 18;
    i[29] = 19;

    // 右面索引
    i[30] = 20;
    i[31] = 21;
    i[32] = 22;
    i[33] = 20;
    i[34] = 22;
    i[35] = 23;

    // 将索引数组存入 meshData
    meshData.Indices32.assign(&i[0], &i[36]);

    //--------------------------
    // 步骤3：限制细分次数（最多6次，防止性能爆炸）
    //--------------------------
    numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

    // 循环细分，让立方体表面更平滑
    for (uint32 i = 0; i < numSubdivisions; ++i)
        Subdivide(meshData);

    // 返回生成好的立方体模型数据
    return meshData;
}

//=====================================================================================
// 函数：创建球体（经纬度球体）
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateSphere(
    float radius, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;

    // 顶部极点顶点
    Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    // 底部极点顶点
    Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // 添加顶部顶点
    meshData.Vertices.push_back(topVertex);

    // 球体经纬度步进值
    float phiStep = XM_PI / stackCount;          // 纵向角度步长
    float thetaStep = 2.0f * XM_PI / sliceCount; // 横向角度步长

    // 循环生成每一层环的顶点
    for (uint32 i = 1; i <= stackCount - 1; ++i)
    {
        float phi = i * phiStep;

        for (uint32 j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            Vertex v;

            // 球坐标 → 笛卡尔坐标（x,y,z）
            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * sinf(theta);

            // 计算切线
            v.TangentU.x = -radius * sinf(phi) * sinf(theta);
            v.TangentU.y = 0.0f;
            v.TangentU.z = +radius * sinf(phi) * cosf(theta);

            // 向量归一化
            XMVECTOR T = XMLoadFloat3(&v.TangentU);
            XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

            // 法线 = 从原点指向顶点的方向（归一化）
            XMVECTOR p = XMLoadFloat3(&v.Position);
            XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

            // 纹理坐标
            v.TexC.x = theta / XM_2PI;
            v.TexC.y = phi / XM_PI;

            // 添加顶点
            meshData.Vertices.push_back(v);
        }
    }

    // 添加底部顶点
    meshData.Vertices.push_back(bottomVertex);

    // 生成顶部三角形索引
    for (uint32 i = 1; i <= sliceCount; ++i)
    {
        meshData.Indices32.push_back(0);
        meshData.Indices32.push_back(i + 1);
        meshData.Indices32.push_back(i);
    }

    // 生成中间层索引
    uint32 baseIndex = 1;
    uint32 ringVertexCount = sliceCount + 1;
    for (uint32 i = 0; i < stackCount - 2; ++i)
    {
        for (uint32 j = 0; j < sliceCount; ++j)
        {
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // 生成底部三角形索引
    uint32 southPoleIndex = (uint32)meshData.Vertices.size() - 1;
    baseIndex = southPoleIndex - ringVertexCount;

    for (uint32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(southPoleIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }

    return meshData;
}

//=====================================================================================
// 函数：细分三角形（让模型更平滑）
// 原理：1个三角形 → 拆分成 4个小三角形
//=====================================================================================
void GeometryGenerator::Subdivide(MeshData &meshData)
{
    // 先保存原始模型数据
    MeshData inputCopy = meshData;

    // 清空原有顶点和索引，准备存放新的细分数据
    meshData.Vertices.resize(0);
    meshData.Indices32.resize(0);

    // 三角形数量 = 总索引数 / 3
    uint32 numTris = (uint32)inputCopy.Indices32.size() / 3;

    // 遍历每个三角形
    for (uint32 i = 0; i < numTris; ++i)
    {
        // 取出三角形的三个顶点
        Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
        Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
        Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

        // 计算三条边的中点
        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

        // 添加6个新顶点
        meshData.Vertices.push_back(v0);
        meshData.Vertices.push_back(v1);
        meshData.Vertices.push_back(v2);
        meshData.Vertices.push_back(m0);
        meshData.Vertices.push_back(m1);
        meshData.Vertices.push_back(m2);

        // 生成4个小三角形的索引
        meshData.Indices32.push_back(i * 6 + 0);
        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 5);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 2);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 1);
        meshData.Indices32.push_back(i * 6 + 4);
    }
}

//=====================================================================================
// 函数：计算两个顶点的中点（插值所有属性：位置、法线、切线、UV）
//=====================================================================================
GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex &v0, const Vertex &v1)
{
    // 加载顶点数据到XMVECTOR（DX数学加速）
    XMVECTOR p0 = XMLoadFloat3(&v0.Position);
    XMVECTOR p1 = XMLoadFloat3(&v1.Position);
    XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
    XMVECTOR n1 = XMLoadFloat3(&v1.Normal);
    XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
    XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);
    XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
    XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

    // 计算平均值（中点）
    XMVECTOR pos = 0.5f * (p0 + p1);
    XMVECTOR normal = XMVector3Normalize(0.5f * (n0 + n1));
    XMVECTOR tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
    XMVECTOR tex = 0.5f * (tex0 + tex1);

    // 输出到新顶点
    Vertex v;
    XMStoreFloat3(&v.Position, pos);
    XMStoreFloat3(&v.Normal, normal);
    XMStoreFloat3(&v.TangentU, tangent);
    XMStoreFloat2(&v.TexC, tex);

    return v;
}

//=====================================================================================
// 函数：创建二十面体球体（更均匀、更标准的球体）
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(
    float radius, uint32 numSubdivisions)
{
    MeshData meshData;

    // 限制最大细分次数
    numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

    // 二十面体原始顶点坐标
    const float X = 0.525731f;
    const float Z = 0.850651f;

    XMFLOAT3 pos[12] =
        {
            XMFLOAT3(-X, 0.0f, Z), XMFLOAT3(X, 0.0f, Z),
            XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
            XMFLOAT3(0.0f, Z, X), XMFLOAT3(0.0f, Z, -X),
            XMFLOAT3(0.0f, -Z, X), XMFLOAT3(0.0f, -Z, -X),
            XMFLOAT3(Z, X, 0.0f), XMFLOAT3(-Z, X, 0.0f),
            XMFLOAT3(Z, -X, 0.0f), XMFLOAT3(-Z, -X, 0.0f)};

    // 二十面体原始索引
    uint32 k[60] =
        {
            1, 4, 0, 4, 9, 0, 4, 5, 9, 8, 5, 4, 1, 8, 4,
            1, 10, 8, 10, 3, 8, 8, 3, 5, 3, 2, 5, 3, 7, 2,
            3, 10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6, 1, 0,
            10, 1, 6, 11, 0, 9, 2, 11, 9, 5, 2, 9, 11, 2, 7};

    // 初始化顶点和索引
    meshData.Vertices.resize(12);
    meshData.Indices32.assign(&k[0], &k[60]);

    for (uint32 i = 0; i < 12; ++i)
        meshData.Vertices[i].Position = pos[i];

    // 细分
    for (uint32 i = 0; i < numSubdivisions; ++i)
        Subdivide(meshData);

    // 将所有顶点投影到球面上，形成标准球体
    for (uint32 i = 0; i < meshData.Vertices.size(); ++i)
    {
        XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));
        XMVECTOR p = radius * n;

        XMStoreFloat3(&meshData.Vertices[i].Position, p);
        XMStoreFloat3(&meshData.Vertices[i].Normal, n);

        // 计算纹理坐标
        float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);
        if (theta < 0.0f)
            theta += XM_2PI;
        float phi = acosf(meshData.Vertices[i].Position.y / radius);

        meshData.Vertices[i].TexC.x = theta / XM_2PI;
        meshData.Vertices[i].TexC.y = phi / XM_PI;

        // 计算切线
        meshData.Vertices[i].TangentU.x = -radius * sinf(phi) * sinf(theta);
        meshData.Vertices[i].TangentU.y = 0.0f;
        meshData.Vertices[i].TangentU.z = +radius * sinf(phi) * cosf(theta);

        XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
        XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
    }

    return meshData;
}

//=====================================================================================
// 函数：创建圆柱（可做圆锥、圆台）
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(
    float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;

    // 每一层高度
    float stackHeight = height / stackCount;
    // 每层半径变化步长
    float radiusStep = (topRadius - bottomRadius) / stackCount;
    uint32 ringCount = stackCount + 1;

    // 生成每一层环顶点
    for (uint32 i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;

        float dTheta = 2.0f * XM_PI / sliceCount;
        for (uint32 j = 0; j <= sliceCount; ++j)
        {
            Vertex vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.Position = XMFLOAT3(r * c, y, r * s);
            vertex.TexC.x = (float)j / sliceCount;
            vertex.TexC.y = 1.0f - (float)i / stackCount;
            vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

            // 计算法线
            float dr = bottomRadius - topRadius;
            XMFLOAT3 bitangent(dr * c, -height, dr * s);
            XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.Normal, N);

            meshData.Vertices.push_back(vertex);
        }
    }

    uint32 ringVertexCount = sliceCount + 1;

    // 生成侧面索引
    for (uint32 i = 0; i < stackCount; ++i)
    {
        for (uint32 j = 0; j < sliceCount; ++j)
        {
            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);

            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
            meshData.Indices32.push_back(i * ringVertexCount + j + 1);
        }
    }

    // 生成顶部、底部盖子
    BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
    BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

    return meshData;
}

//=====================================================================================
// 函数：创建圆柱顶部盖子
//=====================================================================================
void GeometryGenerator::BuildCylinderTopCap(
    float bottomRadius, float topRadius, float height,
    uint32 sliceCount, uint32 stackCount, MeshData &meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();
    float y = 0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;

    // 盖子边缘顶点
    for (uint32 i = 0; i <= sliceCount; ++i)
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    // 盖子中心顶点
    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
    uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

    // 生成盖子三角形
    for (uint32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i + 1);
        meshData.Indices32.push_back(baseIndex + i);
    }
}

//=====================================================================================
// 函数：创建圆柱底部盖子
//=====================================================================================
void GeometryGenerator::BuildCylinderBottomCap(
    float bottomRadius, float topRadius, float height,
    uint32 sliceCount, uint32 stackCount, MeshData &meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();
    float y = -0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;

    // 底部边缘顶点
    for (uint32 i = 0; i <= sliceCount; ++i)
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    // 底部中心顶点
    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
    uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

    // 生成三角形
    for (uint32 i = 0; i < sliceCount; ++i)
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }
}

//=====================================================================================
// 函数：创建XZ平面网格（地形、地面）
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
    MeshData meshData;

    uint32 vertexCount = m * n;
    uint32 faceCount = (m - 1) * (n - 1) * 2;

    // 半宽、半深
    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);
    float du = 1.0f / (n - 1);
    float dv = 1.0f / (m - 1);

    // 生成顶点
    meshData.Vertices.resize(vertexCount);
    for (uint32 i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dz;
        for (uint32 j = 0; j < n; ++j)
        {
            float x = -halfWidth + j * dx;

            meshData.Vertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
            meshData.Vertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            meshData.Vertices[i * n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
            meshData.Vertices[i * n + j].TexC.x = j * du;
            meshData.Vertices[i * n + j].TexC.y = i * dv;
        }
    }

    // 生成索引
    meshData.Indices32.resize(faceCount * 3);
    uint32 k = 0;
    for (uint32 i = 0; i < m - 1; ++i)
    {
        for (uint32 j = 0; j < n - 1; ++j)
        {
            meshData.Indices32[k] = i * n + j;
            meshData.Indices32[k + 1] = i * n + j + 1;
            meshData.Indices32[k + 2] = (i + 1) * n + j;

            meshData.Indices32[k + 3] = (i + 1) * n + j;
            meshData.Indices32[k + 4] = i * n + j + 1;
            meshData.Indices32[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }

    return meshData;
}

//=====================================================================================
// 函数：创建2D屏幕四边形（UI、后处理）
//=====================================================================================
GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
    MeshData meshData;

    meshData.Vertices.resize(4);
    meshData.Indices32.resize(6);

    // 四个顶点
    meshData.Vertices[0] = Vertex(x, y - h, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    meshData.Vertices[1] = Vertex(x, y, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    meshData.Vertices[2] = Vertex(x + w, y, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    meshData.Vertices[3] = Vertex(x + w, y - h, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // 两个三角形
    meshData.Indices32[0] = 0;
    meshData.Indices32[1] = 1;
    meshData.Indices32[2] = 2;
    meshData.Indices32[3] = 0;
    meshData.Indices32[4] = 2;
    meshData.Indices32[5] = 3;

    return meshData;
}