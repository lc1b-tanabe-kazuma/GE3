#pragma once
#include <windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <dxcapi.h>
#include <string>
#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#include "WinApp.h"
#include "DirectXTex/DirectXTex.h"

class DirectXCommon {
public:
	void Initialize(WinApp* winApp);

	// SRVの指定番号のCPUデスクリプタハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	// SRVの指定番号のGPUデスクリプタハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	// 描画前処理
	void PreDraw();

	// 描画後処理
	void PostDraw();

	void WaitForGPU();

	void ResetCommandList();

	// getter
	ID3D12Device* GetDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return commandAllocator; }

	// バッファリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource>CreateBufferResource(size_t sizeInBytes);

	// テクスチャーリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource>CreateTextuerResource(
		const DirectX::TexMetadata& metadata);

	// テクスチャーファイルの読み込み
	Microsoft::WRL::ComPtr <ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr <ID3D12Resource>& texture, const DirectX::ScratchImage& mipImage);

	// シェーダーのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile);

	// 最大SRV数
	static const uint32_t kMaxSRVCount;

private:

	// デバイス初期化
	void DeviceInitialize();

	// コマンド関連の初期化
	void CommandInitialize();

	// スワップチェーンの初期化
	void SwapChainInitialize();

	// 深度バッファの初期化
	void DepthBufferInitialize();

	// 各種デスクリプターヒープの初期化
	void DescriptorHeapsInitialize();

	// レンダーターゲットビューの初期化
	void RTVInitialize();

	// 深度ステンシルビューの初期化
	void DSVInitialize();

	// フェンスの生成
	void FenceInitialize();

	// ビューポート矩形の初期化
	void ViewportInitialize();

	// シザリング矩形の初期化
	void ScissorRectInitialize();

	// DXCコンパイラの生成
	void DxcCompilerInitialize();

	// ImGuiの初期化
	void ImGuiInitialize();

	// DescriptorHeapを作成する関数
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisble);

	// CPUディスクリプタハンドルを取得する関数
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// GPUディスクリプタハンドルを取得する関数
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

	// FPS固定初期化
	void InitializeFixFps();

	// FPS固定更新
	void UpdateFixFps();

	// winApp
	WinApp* winApp = nullptr;

	// DirectX12デバイス
	Microsoft::WRL::ComPtr <ID3D12Device> device = nullptr;

	// DXGIファクトリーの作成
	Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory = nullptr;

	// 使用するアダプター
	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter = nullptr;

	// コマンドアロケーターを生成する
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> commandAllocator = nullptr;

	// コマンドリストを生成する
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList = nullptr;

	// コマンドキューを生成する
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> commandQueue = nullptr;

	// スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	// 生成する深度バッファリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};

	// 深度リソースの生成
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = nullptr;

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};

	// ヒープディスクリプタ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};

	// RTVのディスクリプタヒープの生成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;

	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvdescriptorHeap = nullptr;

	// スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle{};

	// srvのディスクリプタヒープを作成する
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;

	// デスクリプタサイズ
	uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};

	// DSVヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};

	// 関数が成功したか判定する
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	// fence
	Microsoft::WRL::ComPtr <ID3D12Fence> fence = nullptr;

	// 初期値0でfenceを作る
	uint32_t fencevalue = 0;

	HANDLE fenceEvent = nullptr;

	// ビューポート
	D3D12_VIEWPORT viewport{};

	// シザー矩形
	D3D12_RECT scissorRect{};

	// dxcCompilerを初期化
	Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr <IDxcCompiler3> dxCompiler = nullptr;

	// includeHandlerを初期化
	Microsoft::WRL::ComPtr <IDxcIncludeHandler> includehandler = nullptr;

	// バックバッファのインデックスを取得
	UINT backBufferIndex = 0;

	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;

	// コンパイルする
	Microsoft::WRL::ComPtr <IDxcResult> shaderResult = nullptr;

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	// 実際に頂点にリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = nullptr;

	DirectX::ScratchImage image{};

	// 記録時間
	std::chrono::steady_clock::time_point reference_;
};