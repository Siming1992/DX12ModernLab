
#include "d3dUtil.h"
#include "GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:
    // 构造函数和析构函数为保护成员，禁止外部直接创建对象，确保此类只能被继承和使用。
    D3DApp(HINSTANCE hInstance);                   // 构造函数，接受一个HINSTANCE参数，通常用于Windows应用程序的实例句柄。
    D3DApp(const D3DApp &rhs) = delete;            // 删除拷贝构造函数，禁止对象的复制，确保每个D3DApp对象都是唯一的。
    D3DApp &operator=(const D3DApp &rhs) = delete; // 删除赋值运算符，禁止对象的赋值，确保每个D3DApp对象都是唯一的。
    virtual ~D3DApp();                             // 虚析构函数，确保在删除派生类对象时能够正确调用派生类的析构函数，释放资源。

public:
    // 公开的静态方法，提供访问D3DApp实例的接口。
    static D3DApp *GetApp(); // static关键字：静态成员函数，属于类而非对象，可以直接通过类名调用。返回类型为D3DApp*，返回指向D3DApp实例的指针。函数名为GetApp，表示获取当前D3DApp实例。

    // 获取应用程序实例句柄、主窗口句柄和窗口的宽高比等信息的成员函数。
    // C++语法：const成员函数，const关键字放在函数参数列表后，表示该函数不会修改对象的状态，可以在常量对象上调用。
    // 好处：提高代码的安全性和可读性，明确函数的行为，允许在常量对象上调用，提高代码的灵活性。
    HINSTANCE AppInst() const;
    HWND MainWnd() const;
    float AspectRatio() const;

    // 4x MSAA状态的获取和设置函数，允许用户查询和修改4x MSAA（多重采样抗锯齿）状态。
    bool Get4xMsaaState() const;     // const成员函数，表示该函数不会修改对象的状态。
    void Set4xMsaaState(bool valus); // 非const成员函数，允许修改对象的状态。

    int Run(); // 运行应用程序的主循环函数，返回一个整数值，通常用于表示程序的退出状态。

    virtual bool Initialize();                                                  // 初始化函数，虚函数允许派生类重写该函数以提供特定的初始化逻辑，返回一个布尔值表示初始化是否成功。
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // Windows消息处理函数，虚函数允许派生类重写该函数以处理特定的Windows消息，返回一个LRESULT值表示消息处理结果。

protected:
    // 以下为派生类可以（或必须）实现的虚函数
    // 什么情况下必须实现：当基类提供了纯虚函数（pure virtual function）时，派生类必须实现该函数，否则派生类也将成为抽象类，无法实例化。
    // 什么情况下可以实现：当基类提供了虚函数（virtual function）但不是纯虚函数时，派生类可以选择是否重写该函数。如果派生类不重写该函数，则会使用基类提供的默认实现。
    // 什么是纯虚函数：在基类中声明为纯虚函数的函数没有实现，必须在派生类中提供实现。纯虚函数的语法是在函数声明后面加上=0，例如：virtual void FunctionName() = 0;。
    // 纯虚函数的作用：纯虚函数使得基类成为抽象类，不能直接实例化。它强制派生类提供特定的实现，从而确保派生类具有某些功能。
    // 什么是虚函数：在基类中声明为虚函数的函数可以有一个默认实现，派生类可以选择重写该函数以提供特定的实现，也可以使用基类的默认实现。
    // 虚函数的作用：虚函数允许派生类重写基类的函数，以提供特定的实现。它支持多态性，使得通过基类指针或引用调用函数时能够正确地调用派生类的实现。

    // 创建渲染目标视图（RTV）和深度模版试图（DSV）描述符堆的虚函数，允许派生类重写该函数以提供特定的RTV和DSV描述符堆创建逻辑。
    virtual void CreateRtvAndDsvDescriptorHeaps();
    virtual void OnResize(); // 处理窗口大小调整的虚函数，允许派生类重写该函数以提供特定的窗口调整逻辑。

    //=0表示纯虚函数，派生类必须实现该函数以提供特定的更新和渲染逻辑。
    virtual void Update(const GameTimer &gt) = 0; // 更新函数，纯虚函数，派生类必须实现该函数以提供特定的更新逻辑，接受一个GameTimer对象作为参数。
    virtual void Draw(const GameTimer &gt) = 0;   // 渲染帧

    // 鼠标事件处理函数，虚函数允许派生类重写这些函数以提供特定的鼠标事件处理逻辑。
    // 默认实现为空函数（无任何操作），派生类可按需重写；
    // 注：虚函数因运行时动态绑定特性，编译器无法对其进行内联优化（即使类内定义+空函数体），
    // 但空函数体本身开销极小，对性能几乎无影响。
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {} // 鼠标按下事件处理函数
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}   // 鼠标抬起事件处理函数
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {} // 鼠标移动事件处理函数

protected:
    bool InitMainWindow();       // 初始化主窗口的函数，返回一个布尔值表示初始化是否成功。
    bool InitDirect3D();         // 初始化Direct3D的函数，返回一个布尔值表示初始化是否成功。
    void CreateCommandObjects(); // 创建命令对象的函数，负责创建Direct3D命令队列、命令分配器和命令列表。
    void CreateSwapChain();      // 创建交换链的函数，负责创建Direct3D交换链。

    void FlushCommandQueue(); // 刷新命令队列的函数，等待GPU完成所有命令执行，确保所有命令都已执行完成。

    // 获取当前后备缓冲区资源的函数，返回一个指向ID3D12Resource的指针,这是一个COM接口指针。
    ID3D12Resource *CurrentBackBuffer() const;
    // 获取当前后备缓冲区视图（RTV）的函数，返回一个D3D12_CPU_DESCRIPTOR_HANDLE类型的值，一个结构体，表示当前后备缓冲区视图的CPU描述符句柄。
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    // 获取深度模版试图（DSV）的函数，返回一个D3D12_CPU_DESCRIPTOR_HANDLE类型的值，一个结构体，表示深度模版试图的CPU描述符句柄。
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    void CalculateFrameStats(); // 计算帧统计信息的函数，通常用于计算每秒帧数（FPS）和每帧时间（ms/frame）等性能指标。

    void LogAdapters();                                                  // 日志适配器信息的函数，通常用于输出系统中可用的图形适配器（GPU）信息。
    void LogAdapterOutputs(IDXGIAdapter *adapter);                       // 日志适配器输出信息的函数，通常用于输出指定适配器的输出信息。
    void LogOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format); // 日志输出显示模式信息的函数，通常用于输出指定输出设备支持的显示模式信息。

protected:
    // 静态成员变量，指向当前D3DApp实例的指针，允许通过类名访问当前实例。
    static D3DApp *mApp;

    HINSTANCE mhAppInst = nullptr; // 应用程序实例句柄，通常用于Windows应用程序的实例句柄。
    HWND mhMainWnd = nullptr;      // 主窗口句柄，表示应用程序的主窗口。
    bool mAppPaused = false;       // 应用程序是否暂停的标志，表示应用程序当前是否处于暂停状态。
    bool mMinimized = false;       // 窗口是否最小化的标志，表示窗口当前是否处于最小化状态。
    bool mMaximized = false;       // 窗口是否最大化的标志，表示窗口当前是否处于最大化状态。
    bool mResizing = false;        // 窗口是否正在调整大小的标志，表示窗口当前是否正在调整大小。
    bool mfullscreenState = false; // 全屏状态的标志，表示应用程序当前是否处于全屏模式。

    bool m4xMsaaState = false; // 4x MSAA状态的标志，表示是否启用4x MSAA（多重采样抗锯齿）。
    UINT m4xMsaaQuality = 0;   // 4x MSAA质量级别，表示系统支持的4x MSAA质量级别数量，0表示不支持4x MSAA。

    GameTimer mTimer; // 游戏计时器对象，用于跟踪游戏时间和帧时间等性能指标。

    // DirectX图形基础设施(DXGI)
    // DXGI工厂对象，用于创建交换链和枚举适配器。使用Microsoft::WRL::ComPtr智能指针管理，表示用于创建DXGI对象的工厂。
    // IDXGIFactory4,5,6,7等版本的工厂接口，提供了不同版本的功能和特性。根据需要选择合适版本的工厂接口来创建DXGI对象。
    // 如何判断系统支持哪个版本的DXGI工厂接口：可以使用QueryInterface方法查询系统支持的DXGI工厂接口版本。
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;

    // Direct3D 12核心对象
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain; // DXGI交换链对象，表示用于交换前后缓冲区的交换链。
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;   // Direct3D 12设备对象，表示用于创建Direct3D资源和执行渲染操作的设备。

    // Direct3D 12同步对象
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence; // Direct3D 12栅栏对象，表示用于同步CPU和GPU操作的栅栏。
    UINT64 mCurrentFence = 0;                   // 当前栅栏值，表示当前栅栏的值，用于跟踪GPU操作的完成状态。

    // Direct3D 12命令对象
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;           // Direct3D 12命令队列对象，表示用于执行Direct3D命令的命令队列。
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc; // Direct3D 12命令分配器对象，表示用于分配Direct3D命令列表内存的命令分配器。
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;     // Direct3D 12图形命令列表对象，表示用于记录和执行图形命令的命令列表。

    // static const 类内常量，表示交换链缓冲区的数量，通常为2或3，定义为静态常量以便在类内使用，并且值不会改变。
    static const int SwapChainBufferCount = 2; // 交换链缓冲区数量，表示交换链中缓冲区的数量，通常为2或3。
    int mCurrBackBuffer = 0;                   // 当前后备缓冲区索引，表示当前使用的后备缓冲区的索引。

    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount]; // 交换链缓冲区资源数组，表示交换链中每个缓冲区的资源对象。
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;                    // 深度模版缓冲区资源，表示用于深度测试和模版测试的缓冲区资源对象。

    // 描述符堆（描述符的连续内存块）
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap; // 渲染目标视图(RTV)描述符堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap; // 深度模板视图(DSV)描述符堆

    D3D12_VIEWPORT mScreenViewport; // 屏幕视口，表示渲染目标的视口信息，包括位置和大小等。
    D3D12_RECT mScissorRect;        // 定义剪裁矩形，表示渲染目标的剪裁区域，用于限制渲染操作的范围。

    // RTV描述符大小，表示渲染目标视图（RTV）描述符的大小，用于计算描述符堆中的偏移量。
    UINT mRtvDescriptorSize = 0;
    // DSV描述符大小，表示深度模版视图（DSV）描述符的大小，用于计算描述符堆中的偏移量。
    UINT mDsvDescriptorSize = 0;
    // CBV/SRV/UAV描述符大小，表示常量缓冲区视图（CBV）、着色器资源视图（SRV）和无序访问视图（UAV）描述符的大小，用于计算描述符堆中的偏移量。
    UINT mCbvSrvUavDescriptorSize = 0;

    // 以下成员可在派生类中访问和修改，表示应用程序的主窗口标题、Direct3D驱动类型、后备缓冲区格式和深度模版缓冲区格式等信息。
    std::wstring mMainWndCaption = L"D3D12 Application";             // 主窗口标题，表示应用程序主窗口的标题文本。
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;       // Direct3D驱动类型，表示使用的Direct3D驱动类型，通常为硬件驱动（D3D_DRIVER_TYPE_HARDWARE）。
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;      // 后备缓冲区格式，表示交换链中后备缓冲区的像素格式，通常为RGBA 8位无符号整数格式。
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度模版缓冲区格式，表示深度模版缓冲区的像素格式，通常为24位无符号整数深度和8位无符号整数模版格式。

    int mClientWidth = 800;  // 客户区宽度，表示应用程序窗口客户区的宽度，默认值为800像素。
    int mClientHeight = 600; // 客户区高度，表示应用程序窗口客户区的高度，默认值为600像素。

private:
};
