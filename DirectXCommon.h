#pragma once
#include <windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class DirectXCommon {
public:
	static DirectXCommon* GetInstance();
	void Initialize(HWND hwnd, int width, int height);
	void PreDraw();
	void PostDraw();
	ID3D12Device* GetDevice();
	ID3D12GraphicsCommandList* GetCommandList();
	IDXGISwapChain4* GetSwapChain();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

private:

	// デバイス初期化
	void DeviceInitialize();

	// コマンド関連の初期化
	void CommandInitialize();

	// DirectX12デバイス
	Microsoft::WRL::ComPtr <ID3D12Device> device = nullptr;

	// DXGIファクトリーの作成
	Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory = nullptr;
};