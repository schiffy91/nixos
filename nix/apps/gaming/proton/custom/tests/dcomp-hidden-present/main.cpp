/* Regression test: releasing a DComp-bound D3D11 swap chain whose window was
 * hidden with a FIFO present in flight must not block. On GE-Proton11-5 DXVK's
 * frame thread stayed in vkWaitForPresentKHR forever (the compositor never
 * presents an unmapped surface), the release joined it forever, and the
 * Battle.net launcher's login-to-main switch deadlocked. Exit code: 0 passes,
 * 2 means the release hung (watchdog), anything else is a setup failure. */
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <cstdio>

/* umu/pressure-vessel do not reliably forward the exe's stdout or exit code,
 * so the verdict is also written next to the executable. */
static void report(const char *line)
{
    std::printf("%s\n", line);
    std::fflush(stdout);
    if (FILE *f = std::fopen("result.txt", "a")) { std::fprintf(f, "%s\n", line); std::fclose(f); }
}

static DWORD WINAPI watchdog(LPVOID)
{
    Sleep(20000);
    report("HANG: swap chain release did not return within 20 s");
    ExitProcess(2);
}

static void pump()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
}

int main()
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"dcomp-hidden-present";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"dcomp hidden present", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                100, 100, 640, 480, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) { report("FAIL: CreateWindowExW"); return 10; }
    pump();

    ID3D11Device *device = nullptr;
    ID3D11DeviceContext *context = nullptr;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(hr)) { report("FAIL: D3D11CreateDevice"); return 11; }
    IDXGIDevice *dxgi_device = nullptr;
    IDXGIAdapter *adapter = nullptr;
    IDXGIFactory2 *factory = nullptr;
    device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgi_device));
    dxgi_device->GetAdapter(&adapter);
    adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&factory));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = 640;
    desc.Height = 480;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    IDXGISwapChain1 *swapchain = nullptr;
    hr = factory->CreateSwapChainForComposition(device, &desc, nullptr, &swapchain);
    if (FAILED(hr)) { report("FAIL: CreateSwapChainForComposition"); return 12; }

    IDCompositionDesktopDevice *dcomp = nullptr;  // the v2 entry point Battle.net's CEF uses
    hr = DCompositionCreateDevice2(device, __uuidof(IDCompositionDesktopDevice), reinterpret_cast<void **>(&dcomp));
    if (FAILED(hr)) { report("FAIL: DCompositionCreateDevice2"); return 13; }
    IDCompositionTarget *target = nullptr;
    IDCompositionVisual2 *visual = nullptr;
    dcomp->CreateTargetForHwnd(hwnd, TRUE, &target);
    dcomp->CreateVisual(&visual);
    visual->SetContent(swapchain);
    target->SetRoot(visual);
    dcomp->Commit();

    ID3D11Texture2D *back = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&back));
    device->CreateRenderTargetView(back, nullptr, &rtv);
    const float color[4] = { 0.1f, 0.6f, 0.2f, 1.0f };
    for (int frame = 0; frame < 90; frame++)
    {
        context->ClearRenderTargetView(rtv, color);
        swapchain->Present(1, 0);
        pump();
    }
    report("presented 90 frames, hiding the window with a present in flight");
    swapchain->Present(1, 0);
    ShowWindow(hwnd, SW_HIDE);

    CreateThread(nullptr, 0, watchdog, nullptr, 0, nullptr);
    rtv->Release();
    back->Release();
    visual->SetContent(nullptr);
    dcomp->Commit();
    swapchain->Release();  // joins DXVK's frame thread
    visual->Release();
    target->Release();
    dcomp->Release();
    DestroyWindow(hwnd);
    report("PASS: hidden composition swap chain released");
    return 0;
}
