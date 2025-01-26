/*
* File: overlay.cpp
* DirectX overlay class implementation for Winter Survival ESP
*/

#include "overlay.h"
#include <d3dcompiler.h>
#include <dwmapi.h>
#include <iostream>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dwmapi.lib")

bool Overlay::Initialize() {
    WNDCLASSEXA wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "OverlayWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExA(&wc);

    gameWindow = FindWindowA(NULL, "WSS 64  ");
    if (!gameWindow) {
        MessageBoxA(NULL, "Failed to find game window", "Debug", MB_OK);
        return false;
    }

    RECT rect;
    GetWindowRect(gameWindow, &rect);

    overlayWindow = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        "OverlayWindow",
        "Overlay",
        WS_POPUP,
        rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL,
        NULL,
        NULL,
        NULL);

    if (!overlayWindow) {
        MessageBoxA(NULL, "Failed to create overlay window", "Debug", MB_OK);
        return false;
    }

    SetLayeredWindowAttributes(overlayWindow, RGB(0, 0, 0), 255, LWA_ALPHA);
    ShowWindow(overlayWindow, SW_SHOW);

    // Make the window transparent for clicks
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(overlayWindow, &margins);

    if (!InitializeDirectX()) {
        MessageBoxA(NULL, "Failed to initialize DirectX", "Debug", MB_OK);
        return false;
    }

    return true;
}

bool Overlay::InitializeDirectX() {
    RECT rect;
    GetClientRect(overlayWindow, &rect);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = rect.right - rect.left;
    sd.BufferDesc.Height = rect.bottom - rect.top;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = overlayWindow;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &swapChain,
        &device,
        &featureLevel,
        &context
    );

    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create D3D device", "Debug", MB_OK);
        return false;
    }

    ID3D11Texture2D* backBuffer;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to get back buffer", "Debug", MB_OK);
        return false;
    }

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
    backBuffer->Release();

    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create render target", "Debug", MB_OK);
        return false;
    }

    // Compile and create shaders
    const char* vertexShaderCode = R"(
   struct VS_INPUT {
       float3 pos : POSITION;
       float4 color : COLOR;
   };

   struct VS_OUTPUT {
       float4 pos : SV_POSITION;
       float4 color : COLOR;
   };

   VS_OUTPUT main(VS_INPUT input) {
       VS_OUTPUT output;
       output.pos = float4(input.pos, 1.0f);
       output.color = input.color;
       return output;
   })";

    const char* pixelShaderCode = R"(
   struct PS_INPUT {
       float4 pos : SV_POSITION;
       float4 color : COLOR;
   };

   float4 main(PS_INPUT input) : SV_Target {
       return input.color;
   })";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), nullptr, nullptr, nullptr,
        "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(NULL, (char*)errorBlob->GetBufferPointer(), "Vertex Shader Error", MB_OK);
            errorBlob->Release();
        }
        return false;
    }

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), nullptr, nullptr, nullptr,
        "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(NULL, (char*)errorBlob->GetBufferPointer(), "Pixel Shader Error", MB_OK);
            errorBlob->Release();
        }
        vsBlob->Release();
        return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create vertex shader", "Error", MB_OK);
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create pixel shader", "Error", MB_OK);
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create input layout", "Error", MB_OK);
        vsBlob->Release();
        psBlob->Release();
        return false;
    }

    vsBlob->Release();
    psBlob->Release();

    // Create blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* blendState;
    hr = device->CreateBlendState(&blendDesc, &blendState);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to create blend state", "Error", MB_OK);
        return false;
    }

    context->OMSetBlendState(blendState, nullptr, 0xffffffff);
    blendState->Release();

    return true;
}

void Overlay::BeginScene() {
    // Update overlay position to match game window
    RECT rect;
    GetWindowRect(gameWindow, &rect);
    SetWindowPos(overlayWindow, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);

    context->IASetInputLayout(inputLayout);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(renderTarget, clearColor);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set viewport
    GetClientRect(overlayWindow, &rect);
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f,
        static_cast<float>(rect.right - rect.left),
        static_cast<float>(rect.bottom - rect.top),
        0.0f, 1.0f };
    context->RSSetViewports(1, &viewport);
}

void Overlay::Render(MemoryReader& memory) {
    // Draw test box in bright red
    DrawBox(XMFLOAT2(100.0f, 100.0f), 100.0f, 100.0f, XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
}

void Overlay::EndScene() {
    swapChain->Present(1, 0);
}

DirectX::XMFLOAT2 Overlay::WorldToScreen(const DirectX::XMFLOAT3& pos, const DirectX::XMMATRIX& viewProj) {
    RECT rect;
    GetClientRect(overlayWindow, &rect);
    float width = (float)(rect.right - rect.left);
    float height = (float)(rect.bottom - rect.top);

    auto worldPos = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    auto transformed = XMVector3Transform(worldPos, viewProj);

    XMFLOAT4 clip;
    XMStoreFloat4(&clip, transformed);

    if (clip.w < 0.1f) return XMFLOAT2(-1, -1);

    float x = (clip.x / clip.w + 1.0f) * width / 2;
    float y = (-clip.y / clip.w + 1.0f) * height / 2;

    return XMFLOAT2(x, y);
}

void Overlay::DrawBox(const DirectX::XMFLOAT2& screenPos, float width, float height, const DirectX::XMFLOAT4& color) {
    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT4 color;
    };

    RECT rect;
    GetClientRect(overlayWindow, &rect);
    float windowWidth = (float)(rect.right - rect.left);
    float windowHeight = (float)(rect.bottom - rect.top);

    float normalizedX = (screenPos.x / windowWidth) * 2.0f - 1.0f;
    float normalizedY = -(screenPos.y / windowHeight) * 2.0f + 1.0f;
    float normalizedWidth = (width / windowWidth) * 2.0f;
    float normalizedHeight = (height / windowHeight) * 2.0f;

    float lineWidth = 2.0f / windowWidth; // 2 pixel line width

    // Draw 4 lines for the box
    Vertex vertices[] = {
        // Top line
        { XMFLOAT3(normalizedX, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX, normalizedY - lineWidth, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY - lineWidth, 0.0f), color },

        // Right line
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY - normalizedHeight, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth - lineWidth, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth - lineWidth, normalizedY - normalizedHeight, 0.0f), color },

        // Bottom line
        { XMFLOAT3(normalizedX, normalizedY - normalizedHeight, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY - normalizedHeight, 0.0f), color },
        { XMFLOAT3(normalizedX, normalizedY - normalizedHeight + lineWidth, 0.0f), color },
        { XMFLOAT3(normalizedX + normalizedWidth, normalizedY - normalizedHeight + lineWidth, 0.0f), color },

        // Left line
        { XMFLOAT3(normalizedX, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX, normalizedY - normalizedHeight, 0.0f), color },
        { XMFLOAT3(normalizedX + lineWidth, normalizedY, 0.0f), color },
        { XMFLOAT3(normalizedX + lineWidth, normalizedY - normalizedHeight, 0.0f), color }
    };

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    ID3D11Buffer* vertexBuffer;
    device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Draw each line
    for (int i = 0; i < 4; i++) {
        context->Draw(4, i * 4);
    }

    vertexBuffer->Release();
}

void Overlay::CleanupDirectX() {
    if (inputLayout) inputLayout->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (renderTarget) renderTarget->Release();
    if (context) context->Release();
    if (device) device->Release();
    if (swapChain) swapChain->Release();
    if (overlayWindow) DestroyWindow(overlayWindow);
}

Overlay::~Overlay() {
    CleanupDirectX();
}