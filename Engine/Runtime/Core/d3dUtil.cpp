
#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

// 构造函数初始化列表：在进入函数体前初始化成员，效率高且可初始化 const/引用成员。
DxException::DxException(HRESULT hr, const std::wstring &functionName, const std::wstring &filename, int lineNumber) : ErrorCode(hr),
                                                                                                                       FunctionName(functionName),
                                                                                                                       Filename(filename),
                                                                                                                       LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    // 按位与 0x8000：检测高位是否为 1，表示按键当前处于按下状态。
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring &filename)
{
    // 以二进制模式打开文件，避免文本模式对字节流做换行转换。
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    // pos_type 表示流位置类型；这里先取文件大小，再回到开头读取。
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    // 强转为 char* 以按字节写入 Blob 缓冲区。
    fin.read((char *)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device *device,
    ID3D12GraphicsCommandList *cmdList,
    const void *initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer)
{
    // RAII 智能指针：ComPtr 会自动调用 Release，减少 COM 资源泄漏风险。
    ComPtr<ID3D12Resource> defaultBuffer;

    // Create the actual default buffer resource.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                                      D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    // 模板函数调用：UpdateSubresources<1> 中的 <1> 是模板参数，表示子资源数量。
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.

    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
    const std::wstring &filename,
    const D3D_SHADER_MACRO *defines,
    const std::string &entrypoint,
    const std::string &target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    // 条件编译：仅在 Debug 构建中启用调试信息和跳过优化。
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
        OutputDebugStringA((char *)errors->GetBufferPointer());

    ThrowIfFailed(hr);

    return byteCode;
}

std::wstring DxException::ToString() const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    // _com_error::ErrorMessage returns TCHAR*, which maps to wchar_t* when UNICODE is enabled.
    std::wstring msg = err.ErrorMessage();

    // L"..." 是宽字符串字面量；std::to_wstring 把数字转为宽字符串便于拼接。
    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}
