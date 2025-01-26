/*
* File: overlay.h
* DirectX overlay class declaration for Winter Survival ESP
*/

#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "memory.h"

class Overlay {
public:
	bool Initialize();
	void BeginScene();
	void Render(MemoryReader& memory);
	void EndScene();
	~Overlay();

private:
	HWND gameWindow;
	HWND overlayWindow;  // Add this line
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	ID3D11RenderTargetView* renderTarget = nullptr;
	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;
	ID3D11InputLayout* inputLayout = nullptr;

	bool InitializeDirectX();
	void CleanupDirectX();
	DirectX::XMFLOAT2 WorldToScreen(const DirectX::XMFLOAT3& pos, const DirectX::XMMATRIX& viewProj);
	void DrawBox(const DirectX::XMFLOAT2& screenPos, float width, float height, const DirectX::XMFLOAT4& color);
};