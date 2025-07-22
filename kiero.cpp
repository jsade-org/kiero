#include "kiero.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cassert>
#include <iterator>
#include <new>
#include <wrl/client.h>

#if KIERO_INCLUDE_D3D9
# include <d3d9.h>
#endif

#if KIERO_INCLUDE_D3D10
# include <dxgi.h>
# include <d3d10_1.h>
# include <d3d10.h>
#endif

#if KIERO_INCLUDE_D3D11
# include <dxgi.h>
# include <d3d11.h>
#endif

#if KIERO_INCLUDE_D3D12
# include <dxgi.h>
# include <d3d12.h>
#endif

#if KIERO_INCLUDE_OPENGL
# include <gl/GL.h>
#endif

#if KIERO_INCLUDE_VULKAN
# include <vulkan/vulkan.h>
#endif

#if KIERO_USE_MINHOOK
# include <MinHook.h>
#endif

#ifdef _UNICODE
# define KIERO_TEXT(text) L##text
#else
# define KIERO_TEXT(text) text
#endif

namespace kiero
{

static RenderType g_renderType = RenderType::None;
static void** g_methodsTable = nullptr;

Status init(const RenderType renderType)
{
    if(g_renderType != RenderType::None)
    {
        return Status::AlreadyInitializedError;
    }

    if(renderType != RenderType::None)
    {
        if(renderType >= RenderType::D3D9 && renderType <= RenderType::D3D12)
        {
            WNDCLASSEX windowClass;
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = DefWindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = GetModuleHandle(nullptr);
            windowClass.hIcon = nullptr;
            windowClass.hCursor = nullptr;
            windowClass.hbrBackground = nullptr;
            windowClass.lpszMenuName = nullptr;
            windowClass.lpszClassName = KIERO_TEXT("Kiero");
            windowClass.hIconSm = nullptr;

            ::RegisterClassEx(&windowClass);

            HWND window = ::CreateWindow(
                windowClass.lpszClassName,
                KIERO_TEXT("Kiero DirectX Window"),
                WS_OVERLAPPEDWINDOW,
                0,
                0,
                100,
                100,
                nullptr,
                nullptr,
                windowClass.hInstance,
                nullptr
            );

            using Microsoft::WRL::ComPtr;

            if(renderType == RenderType::D3D9)
            {
#if KIERO_INCLUDE_D3D9
                HMODULE libD3D9 = ::GetModuleHandle(KIERO_TEXT("d3d9.dll"));

                if(!libD3D9)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::ModuleNotFoundError;
                }

                auto Direct3DCreate9 = reinterpret_cast<decltype(&::Direct3DCreate9)>(::GetProcAddress(libD3D9, "Direct3DCreate9"));
                if(!Direct3DCreate9)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<IDirect3D9> direct3D9 = Direct3DCreate9(D3D_SDK_VERSION);
                if(!direct3D9)
                {
                    direct3D9 = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                D3DPRESENT_PARAMETERS params { };
                params.BackBufferWidth = 0;
                params.BackBufferHeight = 0;
                params.BackBufferFormat = D3DFMT_UNKNOWN;
                params.BackBufferCount = 0;
                params.MultiSampleType = D3DMULTISAMPLE_NONE;
                params.MultiSampleQuality = 0;
                params.SwapEffect = D3DSWAPEFFECT_DISCARD;
                params.hDeviceWindow = window;
                params.Windowed = 1;
                params.EnableAutoDepthStencil = 0;
                params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                params.Flags = 0;
                params.FullScreen_RefreshRateInHz = 0;
                params.PresentationInterval = 0;

                ComPtr<IDirect3DDevice9> device;
                HRESULT status = direct3D9->CreateDevice(
                    D3DADAPTER_DEFAULT,
                    D3DDEVTYPE_NULLREF,
                    window,
                    D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                    &params,
                    &device
                );

                if(!SUCCEEDED(status))
                {
                    device = nullptr;
                    direct3D9 = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                g_methodsTable = new(::std::nothrow) void* [119];
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable), *reinterpret_cast<void**>(device.Get()), 119 * sizeof(void*));

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                device = nullptr;
                direct3D9 = nullptr;

                g_renderType = RenderType::D3D9;

                ::DestroyWindow(window);
                ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

                return Status::Success;
#endif
            }
            else if(renderType == RenderType::D3D10)
            {
#if KIERO_INCLUDE_D3D10
                HMODULE libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"));
                HMODULE libD3D10 = ::GetModuleHandle(KIERO_TEXT("d3d10.dll"));

                if(!libDXGI || !libD3D10)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::ModuleNotFoundError;
                }

                auto CreateDXGIFactory = reinterpret_cast<decltype(&::CreateDXGIFactory)>(::GetProcAddress(libDXGI, "CreateDXGIFactory"));
                if(!CreateDXGIFactory)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<IDXGIFactory> factory;
                HRESULT status = CreateDXGIFactory(IID_PPV_ARGS(&factory));

                if(!SUCCEEDED(status))
                {
                    factory = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<IDXGIAdapter> adapter;
                status = factory->EnumAdapters(0, &adapter);

                if(!SUCCEEDED(status))
                {
                    adapter = nullptr;
                    factory = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                auto D3D10CreateDeviceAndSwapChain = reinterpret_cast<decltype(&::D3D10CreateDeviceAndSwapChain)>(::GetProcAddress(libD3D10, "D3D10CreateDeviceAndSwapChain"));
                if(!D3D10CreateDeviceAndSwapChain)
                {
                    adapter = nullptr;
                    factory = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                DXGI_RATIONAL refreshRate { };
                refreshRate.Numerator = 60;
                refreshRate.Denominator = 1;

                DXGI_MODE_DESC bufferDesc { };
                bufferDesc.Width = 100;
                bufferDesc.Height = 100;
                bufferDesc.RefreshRate = refreshRate;
                bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

                DXGI_SAMPLE_DESC sampleDesc { };
                sampleDesc.Count = 1;
                sampleDesc.Quality = 0;

                DXGI_SWAP_CHAIN_DESC swapChainDesc { };
                swapChainDesc.BufferDesc = bufferDesc;
                swapChainDesc.SampleDesc = sampleDesc;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 1;
                swapChainDesc.OutputWindow = window;
                swapChainDesc.Windowed = 1;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

                ComPtr<ID3D10Device> device;
                ComPtr<IDXGISwapChain> swapChain;
                status = D3D10CreateDeviceAndSwapChain(
                    adapter.Get(),
                    D3D10_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    0,
                    D3D10_SDK_VERSION,
                    &swapChainDesc,
                    &swapChain,
                    &device
                );

                if(!SUCCEEDED(status))
                {
                    swapChain = nullptr;
                    device = nullptr;
                    adapter = nullptr;
                    factory = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                g_methodsTable = new(::std::nothrow) void* [116];
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable), *reinterpret_cast<void**>(swapChain.Get()), 18 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 18), *reinterpret_cast<void**>(device.Get()), 98 * sizeof(void*));

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                swapChain = nullptr;
                device = nullptr;
                adapter = nullptr;
                factory = nullptr;

                ::DestroyWindow(window);
                ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

                g_renderType = RenderType::D3D10;

                return Status::Success;
#endif
            }
            else if(renderType == RenderType::D3D11)
            {
#if KIERO_INCLUDE_D3D11
                HMODULE libD3D11 = ::GetModuleHandle(KIERO_TEXT("d3d11.dll"));

                if(!libD3D11)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::ModuleNotFoundError;
                }

                PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN D3D11CreateDeviceAndSwapChain = reinterpret_cast<PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN>(::GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain"));

                if(!D3D11CreateDeviceAndSwapChain)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                D3D_FEATURE_LEVEL featureLevel;
                constexpr D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

                DXGI_RATIONAL refreshRate { };
                refreshRate.Numerator = 60;
                refreshRate.Denominator = 1;

                DXGI_MODE_DESC bufferDesc { };
                bufferDesc.Width = 100;
                bufferDesc.Height = 100;
                bufferDesc.RefreshRate = refreshRate;
                bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

                DXGI_SAMPLE_DESC sampleDesc { };
                sampleDesc.Count = 1;
                sampleDesc.Quality = 0;

                DXGI_SWAP_CHAIN_DESC swapChainDesc { };
                swapChainDesc.BufferDesc = bufferDesc;
                swapChainDesc.SampleDesc = sampleDesc;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 1;
                swapChainDesc.OutputWindow = window;
                swapChainDesc.Windowed = 1;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

                ComPtr<ID3D11DeviceContext> context;
                ComPtr<IDXGISwapChain> swapChain;
                ComPtr<ID3D11Device> device;
                HRESULT status = D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    0,
                    featureLevels,
                    2,
                    D3D11_SDK_VERSION,
                    &swapChainDesc,
                    &swapChain,
                    &device,
                    &featureLevel,
                    &context
                );

                if(!SUCCEEDED(status))
                {
                    context = nullptr;
                    swapChain = nullptr;
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                g_methodsTable = new(::std::nothrow) void* [205];
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable), *reinterpret_cast<void**>(swapChain.Get()), 18 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 18), *reinterpret_cast<void**>(device.Get()), 43 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 18 + 43), *reinterpret_cast<void**>(context.Get()), 144 * sizeof(void*));

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                context = nullptr;
                swapChain = nullptr;
                device = nullptr;

                ::DestroyWindow(window);
                ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

                g_renderType = RenderType::D3D11;

                return Status::Success;
#endif
            }
            else if(renderType == RenderType::D3D12)
            {
#if KIERO_INCLUDE_D3D12
                HMODULE libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"));
                HMODULE libD3D12 = ::GetModuleHandle(KIERO_TEXT("d3d12.dll"));

                if(!libDXGI || !libD3D12)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::ModuleNotFoundError;
                }

                auto CreateDXGIFactory = reinterpret_cast<decltype(&::CreateDXGIFactory)>(::GetProcAddress(libDXGI, "CreateDXGIFactory"));

                if(!CreateDXGIFactory)
                {
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<IDXGIFactory> factory;
                HRESULT status = CreateDXGIFactory(IID_PPV_ARGS(&factory));

                if(!SUCCEEDED(status))
                {
                    factory = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<IDXGIAdapter> adapter;
                status = factory->EnumAdapters(0, &adapter);
                factory = nullptr;

                if(!SUCCEEDED(status))
                {
                    adapter = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(::GetProcAddress(libD3D12, "D3D12CreateDevice"));

                if(!D3D12CreateDevice)
                {
                    adapter = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<ID3D12Device> device;
                status = D3D12CreateDevice(
                    adapter.Get(),
                    D3D_FEATURE_LEVEL_11_0,
                    IID_PPV_ARGS(&device)
                );

                adapter = nullptr;

                if(!SUCCEEDED(status))
                {
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                D3D12_COMMAND_QUEUE_DESC queueDesc {};
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                queueDesc.Priority = 0;
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                queueDesc.NodeMask = 0;

                ComPtr<ID3D12CommandQueue> commandQueue;
                status = device->CreateCommandQueue(
                    &queueDesc,
                    IID_PPV_ARGS(&commandQueue)
                );

                if(!SUCCEEDED(status))
                {
                    commandQueue = nullptr;
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<ID3D12CommandAllocator> commandAllocator;
                status = device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    IID_PPV_ARGS(&commandAllocator)
                );

                if(!SUCCEEDED(status))
                {
                    commandAllocator = nullptr;
                    commandQueue = nullptr;
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                ComPtr<ID3D12GraphicsCommandList> commandList;
                status = device->CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    commandAllocator.Get(),
                    nullptr,
                    IID_PPV_ARGS(&commandList)
                );

                if(!SUCCEEDED(status))
                {
                    commandList = nullptr;
                    commandAllocator = nullptr;
                    commandQueue = nullptr;
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                DXGI_RATIONAL refreshRate { };
                refreshRate.Numerator = 60;
                refreshRate.Denominator = 1;

                DXGI_MODE_DESC bufferDesc { };
                bufferDesc.Width = 100;
                bufferDesc.Height = 100;
                bufferDesc.RefreshRate = refreshRate;
                bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

                DXGI_SAMPLE_DESC sampleDesc { };
                sampleDesc.Count = 1;
                sampleDesc.Quality = 0;

                DXGI_SWAP_CHAIN_DESC swapChainDesc { };
                swapChainDesc.BufferDesc = bufferDesc;
                swapChainDesc.SampleDesc = sampleDesc;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.BufferCount = 2;
                swapChainDesc.OutputWindow = window;
                swapChainDesc.Windowed = 1;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

                ComPtr<IDXGISwapChain> swapChain;
                status = factory->CreateSwapChain(
                    commandQueue.Get(),
                    &swapChainDesc,
                    &swapChain
                );

                if(!SUCCEEDED(status))
                {
                    swapChain = nullptr;
                    commandList = nullptr;
                    commandAllocator = nullptr;
                    commandQueue = nullptr;
                    device = nullptr;
                    ::DestroyWindow(window);
                    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
                    return Status::UnknownError;
                }

                g_methodsTable = new(::std::nothrow) void* [150];
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable), *reinterpret_cast<void**>(device.Get()), 44 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 44), *reinterpret_cast<void**>(commandQueue.Get()), 19 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 44 + 19), *reinterpret_cast<void**>(commandAllocator.Get()), 9 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 44 + 19 + 9), *reinterpret_cast<void**>(commandList.Get()), 60 * sizeof(void*));
                (void) ::std::memcpy(static_cast<void*>(g_methodsTable + 44 + 19 + 9 + 60), *reinterpret_cast<void**>(swapChain.Get()), 18 * sizeof(void*));

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                swapChain = nullptr;
                commandList = nullptr;
                commandAllocator = nullptr;
                commandQueue = nullptr;
                device = nullptr;

                ::DestroyWindow(window);
                ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

                g_renderType = RenderType::D3D12;

                return Status::Success;
#endif
            }

            ::DestroyWindow(window);
            ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

            return Status::NotSupportedError;
        }
        else if(renderType != RenderType::Auto)
        {
            if(renderType == RenderType::OpenGL)
            {
#if KIERO_INCLUDE_OPENGL
                HMODULE libOpenGL32 = ::GetModuleHandle(KIERO_TEXT("opengl32.dll"));

                if(!libOpenGL32)
                {
                    return Status::ModuleNotFoundError;
                }

                constexpr const char* const methodsNames[] = {
                    "glAccum", "glAlphaFunc", "glAreTexturesResident", "glArrayElement", "glBegin", "glBindTexture", "glBitmap", "glBlendFunc", "glCallList", "glCallLists", "glClear", "glClearAccum",
                    "glClearColor", "glClearDepth", "glClearIndex", "glClearStencil", "glClipPlane", "glColor3b", "glColor3bv", "glColor3d", "glColor3dv", "glColor3f", "glColor3fv", "glColor3i", "glColor3iv",
                    "glColor3s", "glColor3sv", "glColor3ub", "glColor3ubv", "glColor3ui", "glColor3uiv", "glColor3us", "glColor3usv", "glColor4b", "glColor4bv", "glColor4d", "glColor4dv", "glColor4f",
                    "glColor4fv", "glColor4i", "glColor4iv", "glColor4s", "glColor4sv", "glColor4ub", "glColor4ubv", "glColor4ui", "glColor4uiv", "glColor4us", "glColor4usv", "glColorMask", "glColorMaterial",
                    "glColorPointer", "glCopyPixels", "glCopyTexImage1D", "glCopyTexImage2D", "glCopyTexSubImage1D", "glCopyTexSubImage2D", "glCullFaceglCullFace", "glDeleteLists", "glDeleteTextures",
                    "glDepthFunc", "glDepthMask", "glDepthRange", "glDisable", "glDisableClientState", "glDrawArrays", "glDrawBuffer", "glDrawElements", "glDrawPixels", "glEdgeFlag", "glEdgeFlagPointer",
                    "glEdgeFlagv", "glEnable", "glEnableClientState", "glEnd", "glEndList", "glEvalCoord1d", "glEvalCoord1dv", "glEvalCoord1f", "glEvalCoord1fv", "glEvalCoord2d", "glEvalCoord2dv",
                    "glEvalCoord2f", "glEvalCoord2fv", "glEvalMesh1", "glEvalMesh2", "glEvalPoint1", "glEvalPoint2", "glFeedbackBuffer", "glFinish", "glFlush", "glFogf", "glFogfv", "glFogi", "glFogiv",
                    "glFrontFace", "glFrustum", "glGenLists", "glGenTextures", "glGetBooleanv", "glGetClipPlane", "glGetDoublev", "glGetError", "glGetFloatv", "glGetIntegerv", "glGetLightfv", "glGetLightiv",
                    "glGetMapdv", "glGetMapfv", "glGetMapiv", "glGetMaterialfv", "glGetMaterialiv", "glGetPixelMapfv", "glGetPixelMapuiv", "glGetPixelMapusv", "glGetPointerv", "glGetPolygonStipple",
                    "glGetString", "glGetTexEnvfv", "glGetTexEnviv", "glGetTexGendv", "glGetTexGenfv", "glGetTexGeniv", "glGetTexImage", "glGetTexLevelParameterfv", "glGetTexLevelParameteriv",
                    "glGetTexParameterfv", "glGetTexParameteriv", "glHint", "glIndexMask", "glIndexPointer", "glIndexd", "glIndexdv", "glIndexf", "glIndexfv", "glIndexi", "glIndexiv", "glIndexs", "glIndexsv",
                    "glIndexub", "glIndexubv", "glInitNames", "glInterleavedArrays", "glIsEnabled", "glIsList", "glIsTexture", "glLightModelf", "glLightModelfv", "glLightModeli", "glLightModeliv", "glLightf",
                    "glLightfv", "glLighti", "glLightiv", "glLineStipple", "glLineWidth", "glListBase", "glLoadIdentity", "glLoadMatrixd", "glLoadMatrixf", "glLoadName", "glLogicOp", "glMap1d", "glMap1f",
                    "glMap2d", "glMap2f", "glMapGrid1d", "glMapGrid1f", "glMapGrid2d", "glMapGrid2f", "glMaterialf", "glMaterialfv", "glMateriali", "glMaterialiv", "glMatrixMode", "glMultMatrixd",
                    "glMultMatrixf", "glNewList", "glNormal3b", "glNormal3bv", "glNormal3d", "glNormal3dv", "glNormal3f", "glNormal3fv", "glNormal3i", "glNormal3iv", "glNormal3s", "glNormal3sv",
                    "glNormalPointer", "glOrtho", "glPassThrough", "glPixelMapfv", "glPixelMapuiv", "glPixelMapusv", "glPixelStoref", "glPixelStorei", "glPixelTransferf", "glPixelTransferi", "glPixelZoom",
                    "glPointSize", "glPolygonMode", "glPolygonOffset", "glPolygonStipple", "glPopAttrib", "glPopClientAttrib", "glPopMatrix", "glPopName", "glPrioritizeTextures", "glPushAttrib",
                    "glPushClientAttrib", "glPushMatrix", "glPushName", "glRasterPos2d", "glRasterPos2dv", "glRasterPos2f", "glRasterPos2fv", "glRasterPos2i", "glRasterPos2iv", "glRasterPos2s",
                    "glRasterPos2sv", "glRasterPos3d", "glRasterPos3dv", "glRasterPos3f", "glRasterPos3fv", "glRasterPos3i", "glRasterPos3iv", "glRasterPos3s", "glRasterPos3sv", "glRasterPos4d",
                    "glRasterPos4dv", "glRasterPos4f", "glRasterPos4fv", "glRasterPos4i", "glRasterPos4iv", "glRasterPos4s", "glRasterPos4sv", "glReadBuffer", "glReadPixels", "glRectd", "glRectdv", "glRectf",
                    "glRectfv", "glRecti", "glRectiv", "glRects", "glRectsv", "glRenderMode", "glRotated", "glRotatef", "glScaled", "glScalef", "glScissor", "glSelectBuffer", "glShadeModel", "glStencilFunc",
                    "glStencilMask", "glStencilOp", "glTexCoord1d", "glTexCoord1dv", "glTexCoord1f", "glTexCoord1fv", "glTexCoord1i", "glTexCoord1iv", "glTexCoord1s", "glTexCoord1sv", "glTexCoord2d",
                    "glTexCoord2dv", "glTexCoord2f", "glTexCoord2fv", "glTexCoord2i", "glTexCoord2iv", "glTexCoord2s", "glTexCoord2sv", "glTexCoord3d", "glTexCoord3dv", "glTexCoord3f", "glTexCoord3fv",
                    "glTexCoord3i", "glTexCoord3iv", "glTexCoord3s", "glTexCoord3sv", "glTexCoord4d", "glTexCoord4dv", "glTexCoord4f", "glTexCoord4fv", "glTexCoord4i", "glTexCoord4iv", "glTexCoord4s",
                    "glTexCoord4sv", "glTexCoordPointer", "glTexEnvf", "glTexEnvfv", "glTexEnvi", "glTexEnviv", "glTexGend", "glTexGendv", "glTexGenf", "glTexGenfv", "glTexGeni", "glTexGeniv", "glTexImage1D",
                    "glTexImage2D", "glTexParameterf", "glTexParameterfv", "glTexParameteri", "glTexParameteriv", "glTexSubImage1D", "glTexSubImage2D", "glTranslated", "glTranslatef", "glVertex2d",
                    "glVertex2dv", "glVertex2f", "glVertex2fv", "glVertex2i", "glVertex2iv", "glVertex2s", "glVertex2sv", "glVertex3d", "glVertex3dv", "glVertex3f", "glVertex3fv", "glVertex3i", "glVertex3iv",
                    "glVertex3s", "glVertex3sv", "glVertex4d", "glVertex4dv", "glVertex4f", "glVertex4fv", "glVertex4i", "glVertex4iv", "glVertex4s", "glVertex4sv", "glVertexPointer", "glViewport"
                };

                ::std::size_t size = ::std::size(methodsNames);

                g_methodsTable = new(::std::nothrow) void* [size];

                for(::std::size_t i = 0; i < size; ++i)
                {
                    g_methodsTable[i] = ::GetProcAddress(libOpenGL32, methodsNames[i]);
                }

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                g_renderType = RenderType::OpenGL;

                return Status::Success;
#endif
            }
            else if(renderType == RenderType::Vulkan)
            {
#if KIERO_INCLUDE_VULKAN
                HMODULE libVulkan = GetModuleHandle(KIERO_TEXT("vulkan-1.dll"));
                if(!libVulkan)
                {
                    return Status::ModuleNotFoundError;
                }

                constexpr const char* const methodsNames[] = {
                    "vkCreateInstance", "vkDestroyInstance", "vkEnumeratePhysicalDevices", "vkGetPhysicalDeviceFeatures", "vkGetPhysicalDeviceFormatProperties", "vkGetPhysicalDeviceImageFormatProperties",
                    "vkGetPhysicalDeviceProperties", "vkGetPhysicalDeviceQueueFamilyProperties", "vkGetPhysicalDeviceMemoryProperties", "vkGetInstanceProcAddr", "vkGetDeviceProcAddr", "vkCreateDevice",
                    "vkDestroyDevice", "vkEnumerateInstanceExtensionProperties", "vkEnumerateDeviceExtensionProperties", "vkEnumerateDeviceLayerProperties", "vkGetDeviceQueue", "vkQueueSubmit", "vkQueueWaitIdle",
                    "vkDeviceWaitIdle", "vkAllocateMemory", "vkFreeMemory", "vkMapMemory", "vkUnmapMemory", "vkFlushMappedMemoryRanges", "vkInvalidateMappedMemoryRanges", "vkGetDeviceMemoryCommitment",
                    "vkBindBufferMemory", "vkBindImageMemory", "vkGetBufferMemoryRequirements", "vkGetImageMemoryRequirements", "vkGetImageSparseMemoryRequirements", "vkGetPhysicalDeviceSparseImageFormatProperties",
                    "vkQueueBindSparse", "vkCreateFence", "vkDestroyFence", "vkResetFences", "vkGetFenceStatus", "vkWaitForFences", "vkCreateSemaphore", "vkDestroySemaphore", "vkCreateEvent", "vkDestroyEvent",
                    "vkGetEventStatus", "vkSetEvent", "vkResetEvent", "vkCreateQueryPool", "vkDestroyQueryPool", "vkGetQueryPoolResults", "vkCreateBuffer", "vkDestroyBuffer", "vkCreateBufferView", "vkDestroyBufferView",
                    "vkCreateImage", "vkDestroyImage", "vkGetImageSubresourceLayout", "vkCreateImageView", "vkDestroyImageView", "vkCreateShaderModule", "vkDestroyShaderModule", "vkCreatePipelineCache",
                    "vkDestroyPipelineCache", "vkGetPipelineCacheData", "vkMergePipelineCaches", "vkCreateGraphicsPipelines", "vkCreateComputePipelines", "vkDestroyPipeline", "vkCreatePipelineLayout",
                    "vkDestroyPipelineLayout", "vkCreateSampler", "vkDestroySampler", "vkCreateDescriptorSetLayout", "vkDestroyDescriptorSetLayout", "vkCreateDescriptorPool", "vkDestroyDescriptorPool",
                    "vkResetDescriptorPool", "vkAllocateDescriptorSets", "vkFreeDescriptorSets", "vkUpdateDescriptorSets", "vkCreateFramebuffer", "vkDestroyFramebuffer", "vkCreateRenderPass", "vkDestroyRenderPass",
                    "vkGetRenderAreaGranularity", "vkCreateCommandPool", "vkDestroyCommandPool", "vkResetCommandPool", "vkAllocateCommandBuffers", "vkFreeCommandBuffers", "vkBeginCommandBuffer", "vkEndCommandBuffer",
                    "vkResetCommandBuffer", "vkCmdBindPipeline", "vkCmdSetViewport", "vkCmdSetScissor", "vkCmdSetLineWidth", "vkCmdSetDepthBias", "vkCmdSetBlendConstants", "vkCmdSetDepthBounds",
                    "vkCmdSetStencilCompareMask", "vkCmdSetStencilWriteMask", "vkCmdSetStencilReference", "vkCmdBindDescriptorSets", "vkCmdBindIndexBuffer", "vkCmdBindVertexBuffers", "vkCmdDraw", "vkCmdDrawIndexed",
                    "vkCmdDrawIndirect", "vkCmdDrawIndexedIndirect", "vkCmdDispatch", "vkCmdDispatchIndirect", "vkCmdCopyBuffer", "vkCmdCopyImage", "vkCmdBlitImage", "vkCmdCopyBufferToImage", "vkCmdCopyImageToBuffer",
                    "vkCmdUpdateBuffer", "vkCmdFillBuffer", "vkCmdClearColorImage", "vkCmdClearDepthStencilImage", "vkCmdClearAttachments", "vkCmdResolveImage", "vkCmdSetEvent", "vkCmdResetEvent", "vkCmdWaitEvents",
                    "vkCmdPipelineBarrier", "vkCmdBeginQuery", "vkCmdEndQuery", "vkCmdResetQueryPool", "vkCmdWriteTimestamp", "vkCmdCopyQueryPoolResults", "vkCmdPushConstants", "vkCmdBeginRenderPass", "vkCmdNextSubpass",
                    "vkCmdEndRenderPass", "vkCmdExecuteCommands"
                };

                ::std::size_t size = ::std::size(methodsNames);

                g_methodsTable = new(::std::nothrow) void* [size];

                for(::std::size_t i = 0; i < size; ++i)
                {
                    g_methodsTable[i] = ::GetProcAddress(libVulkan, methodsNames[i]);
                }

#if KIERO_USE_MINHOOK
                MH_Initialize();
#endif

                g_renderType = RenderType::Vulkan;

                return Status::Success;
#endif
            }

            return Status::NotSupportedError;
        }
        else
        {
            RenderType type = RenderType::None;

            if(::GetModuleHandle(KIERO_TEXT("d3d9.dll")))
            {
                type = RenderType::D3D9;
            }
            else if(::GetModuleHandle(KIERO_TEXT("d3d10.dll")))
            {
                type = RenderType::D3D10;
            }
            else if(::GetModuleHandle(KIERO_TEXT("d3d11.dll")))
            {
                type = RenderType::D3D11;
            }
            else if(::GetModuleHandle(KIERO_TEXT("d3d12.dll")))
            {
                type = RenderType::D3D12;
            }
            else if(::GetModuleHandle(KIERO_TEXT("opengl32.dll")))
            {
                type = RenderType::OpenGL;
            }
            else if(::GetModuleHandle(KIERO_TEXT("vulkan-1.dll")))
            {
                type = RenderType::Vulkan;
            }
            else
            {
                return Status::NotSupportedError;
            }

            return init(type);
        }
    }

    return Status::Success;
}

void shutdown()
{
    if(g_renderType != RenderType::None)
    {
#if KIERO_USE_MINHOOK
        MH_DisableHook(MH_ALL_HOOKS);
#endif

        delete[] g_methodsTable;
        g_methodsTable = nullptr;
        g_renderType = RenderType::None;
    }
}

Status bind(const ::std::uint16_t index, void** const original, void* const function)
{
    // TODO: Need own detour function

    assert(original != nullptr && function != nullptr);

    if(g_renderType != RenderType::None)
    {
#if KIERO_USE_MINHOOK
        void* target = g_methodsTable[index];
        if(MH_CreateHook(target, function, original) != MH_OK || MH_EnableHook(target) != MH_OK)
        {
            return Status::UnknownError;
        }
#endif

        return Status::Success;
    }

    return Status::NotInitializedError;
}

void unbind(const ::std::uint16_t index)
{
    if(g_renderType != RenderType::None)
    {
#if KIERO_USE_MINHOOK
        MH_DisableHook(g_methodsTable[index]);
#endif
    }
}

[[nodiscard]] RenderType getRenderType() noexcept
{
    return g_renderType;
}

[[nodiscard]] void** getMethodsTable() noexcept
{
    return g_methodsTable;
}

}