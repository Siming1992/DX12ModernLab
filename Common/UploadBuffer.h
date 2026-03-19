#pragma once

#include "d3dUtil.h"

// 模板类定义：T表示缓冲区中存储的元素类型
// 模板允许这个类适配任意数据类型（如矩阵、常量结构体等），提高复用性
template <typename T>
class UploadBuffer
{
public:
    // 构造函数：创建上传缓冲区
    // 参数说明：
    //   device: DX12设备指针，用于创建资源
    //   elementCount: 缓冲区中元素的数量
    //   isConstantBuffer: 是否作为常量缓冲区使用（常量缓冲区有特殊的字节对齐要求）
    UploadBuffer(ID3D12Device *device, UINT elementCount, bool isConstantBuffer)
        : mIsConstantBuffer(isConstantBuffer)
    {
        // 计算每个元素的字节大小，默认是类型T的原始大小，但如果是常量缓冲区，则需要调整为256字节对齐
        mElementByteSize = sizeof(T);

        // DX12常量缓冲区的特殊要求：
        // 硬件只能访问偏移和大小都是256字节倍数的常量缓冲区数据
        // D3D12_CONSTANT_BUFFER_VIEW_DESC结构体中的OffsetInBytes和SizeInBytes都必须是256的倍数
        if (isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        // 创建提交资源（Committed Resource）：这是DX12创建资源的标准方式
        // 1. 堆属性：D3D12_HEAP_TYPE_UPLOAD表示上传堆，CPU可写、GPU可读，用于CPU向GPU传输数据
        // 2. 堆标志：无特殊标志
        // 3. 资源描述：创建缓冲区资源，总大小=单个元素大小×元素数量
        // 4. 资源初始状态：D3D12_RESOURCE_STATE_GENERIC_READ表示GPU可读状态
        // 5. 清除值：空（缓冲区不需要清除值）
        // 6. 接口ID：IID_PPV_ARGS是安全的类型转换宏，获取ID3D12Resource接口指针
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        // 将资源映射到CPU地址空间：
        // Map操作让CPU可以直接访问GPU资源的内存，参数0表示映射整个资源，nullptr表示无读写限制
        // reinterpret_cast<void**>是强制类型转换，将BYTE**转换为void**以匹配函数参数类型，mMappedData将指向映射后的内存地址
        // 这里&mMappedData是传递指针的地址，mMappedData本身是BYTE*类型，是指针变量，&mMappedData是指向这个指针变量的指针（BYTE**），符合Map函数的参数要求
        // 为什么设计这种指针的指针参数？因为Map函数需要修改mMappedData的值，使其指向映射后的内存地址，所以需要传递mMappedData的地址（即BYTE**），以便函数内部可以修改它的值
        // 通过Map函数，mMappedData将指向GPU资源的内存地址，CPU可以通过这个指针直接写入数据到GPU资源中
        // 本质上是为了给CPU提供一个直接访问GPU资源内存的方式，避免了额外的复制步骤，提高性能
        // 理论上是否可以通过指针参数直接传递mMappedData而不是&mMappedData？
        // 不行，因为Map函数需要修改mMappedData的值，使其指向映射后的内存地址，所以必须传递mMappedData的地址（即BYTE**），以便函数内部可以修改它的值
        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mMappedData)));

        // 重要说明：
        // 1. 除非资源不再使用，否则不需要Unmap（解除映射）
        // 2. 当GPU正在使用该资源时，CPU不能写入（必须通过同步机制保证，如Fence）
    }

    // 禁用拷贝构造函数（C++11特性）：防止浅拷贝导致的资源重复释放
    // UploadBuffer的拷贝会导致多个对象指向同一个ID3D12Resource，析构时重复Unmap/释放资源
    // 这里 const的使用表示该函数不会修改参数对象，但由于被删除（= delete），实际上无法调用这个函数
    // 那UploadBuffer(UploadBuffer &rhs) = delete; 也可以吗？不行，因为拷贝构造函数必须接受const参数，才能匹配常规的拷贝语义
    UploadBuffer(const UploadBuffer &rhs) = delete;

    // 禁用赋值运算符重载：同上，防止浅拷贝问题
    UploadBuffer &operator=(const UploadBuffer &rhs) = delete;

    // 析构函数：释放资源
    ~UploadBuffer()
    {
        // 如果上传缓冲区资源有效，则解除CPU地址映射
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        // 将映射的CPU指针置空，避免悬空指针
        mMappedData = nullptr;
    }

    // 常量成员函数（const关键字）：返回上传缓冲区的ID3D12Resource接口指针
    // const保证该函数不会修改类的成员变量，提高代码安全性
    ID3D12Resource *Resource() const
    {
        return mUploadBuffer.Get(); // ComPtr的Get()方法获取原始接口指针
    }

    // 将数据拷贝到上传缓冲区的指定元素位置
    // 参数：
    //   elementIndex: 要写入的元素索引
    //   data: 要拷贝的数据源（const引用避免拷贝，同时保证不修改源数据）
    void CopyData(int elementIndex, const T &data)
    {
        // 计算目标内存地址：基地址 + 索引×单个元素大小
        // memcpy是高效的内存拷贝函数，这里拷贝sizeof(T)字节（原始数据大小，而非对齐后的大小）
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

private:
    // 用于存储上传数据的GPU资源
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;

    // 指向映射后的CPU内存地址的指针（BYTE*是字节级别的通用指针），用于写入数据
    // 这里mMappedData是一个指向BYTE的指针，表示映射后的内存地址，CPU可以通过这个指针直接访问GPU资源的内存
    BYTE *mMappedData = nullptr;

    // 每个元素的字节大小（根据是否为常量缓冲区进行调整）
    UINT mElementByteSize = 0;

    // 标记是否为常量缓冲区（影响元素大小计算）
    bool mIsConstantBuffer;
};