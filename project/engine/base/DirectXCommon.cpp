#include "DirectXCommon.h"
#include "StringUtility.h"
#include "Logger.h"
#include <cassert>
#include "DirectXTex/d3dx12.h"
#include <thread>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace Logeer;
using namespace StringUtility;
using namespace std;

const uint32_t DirectXCommon::kMaxSRVCount = 512;

void DirectXCommon::Initialize(WinApp* winApp) {

	// FPS固定初期化
	InitializeFixFps();

	// Null
	assert(winApp);

	// メンバ変数に記録
	this->winApp = winApp;

	// デバイスの生成
	DeviceInitialize();

	// コマンド関連の初期化
	CommandInitialize();

	// スワップチェーンの初期化
	SwapChainInitialize();

	// 深度バッファの初期化
	DepthBufferInitialize();

	// 各種デスクリプターヒープの初期化
	DescriptorHeapsInitialize();

	// レンダーターゲットビューの初期化
	RTVInitialize();

	// 深度ステンシルビューの初期化
	DSVInitialize();

	// フェンスの生成
	FenceInitialize();

	// ビューポート矩形の初期化
	ViewportInitialize();

	// シザリング矩形の初期化
	ScissorRectInitialize();

	// DXCコンパイラの生成
	DxcCompilerInitialize();

	// ImGuiの初期化
	ImGuiInitialize();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
	return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile) {

	//Log(logStream, ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));

	// hlslファイルを読む
	Microsoft::WRL::ComPtr <IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	// コンパイルする
	hr = dxCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includehandler.Get(),
		IID_PPV_ARGS(&shaderResult)
	);

	// コンパイルエラーの時に止まる
	assert(SUCCEEDED(hr));

	// エラーが出たらログに出して止める
	Microsoft::WRL::ComPtr <IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {

		// エラーがあったら出力する
		//Log(logStream, shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイル結果からバイナリ部分を取得する
	Microsoft::WRL::ComPtr <IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したらログを出す
//	Log(logStream, ConvertString(std::format(L"Compile Success,path:{},profile:{}\n", filePath, profile)));

	// shaderBlobを返す
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
	// UploadHeapを使う
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// バッファリソース。てぅすちゃの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	// リソースのサイズ。今回はVector4を3頂点分
	vertexResourceDesc.Width = sizeInBytes;

	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));
	return vertexResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextuerResource(const DirectX::TexMetadata& metadata) {
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // ミップマップの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // テクスチャの深さまたはアレイサイズ
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// 利用するするHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // コピー先の状態
		nullptr, // 初期化しない
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImage) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImage.GetImages(), mipImage.GetImageCount(), mipImage.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	// Textureへの転送後は利用できるよう、
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

void DirectXCommon::DeviceInitialize() {

	assert(SUCCEEDED(hr));

	//
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {

		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		//
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

			//Log(logStream, ConvertString(std::format(L"USE Adaptere:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}

	assert(useAdapter != nullptr);

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelString[] = { "12.2","12.1","12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {
			//	Log(logStream, std::format("feturelevel : {}\n", featureLevelString[i]));
			break;
		}
	}

	assert(device != nullptr);
	Log("Compleate create D3D12Device!!\n");

	// logのデバッグを出力
	Log("Hello, DirectX!\n");

	// logに画面の幅の数値を出力
	//Log(logStream, ConvertString(std::format(L"width : {}, {}\n", WinApp::kClientWidth, WinApp::kClientHeight)));

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		//やばいエラーの時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		// エラーの時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 警告の時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
	}
#endif // _DEBUG
}

void DirectXCommon::CommandInitialize() {

	// コマンドアロケーターを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	//コマンドリストの生成がうまく行かなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	// コマンドキューの生成がうまく行かなかったので起動できない
	assert(SUCCEEDED(hr));
}

void DirectXCommon::SwapChainInitialize() {
	swapChainDesc.Width = WinApp::kClientWidth;
	swapChainDesc.Height = WinApp::kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void DirectXCommon::DepthBufferInitialize() {
	// 生成するResourceの設定
	resourceDesc.Width = WinApp::kClientWidth;
	resourceDesc.Height = WinApp::kClientHeight;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するHeapの設定
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Resourceの生成
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthStencilResource));
	assert(SUCCEEDED(hr));
}

// DescriptorHeapを作成する関数
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = heapType;
	desc.NumDescriptors = numDescriptors;

	// ★ここでガードする
	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		desc.Flags = shaderVisible
			? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
			: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	} else {
		// RTV / DSV は強制的に NONE
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	assert(SUCCEEDED(hr));

	return heap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

void DirectXCommon::InitializeFixFps() {
	reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFps() {
	// 1/60秒の時間
	const std::chrono::microseconds KMinTime(uint64_t(1000000.0f / 60.0f));

	// 1/60秒より短い時間
	const std::chrono::microseconds KMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在の時間を取得
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 経過時間を取得
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60秒立ってないとき
	if (elapsed < KMinCheckTime) {

		// 1/60経過するまでスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < KMinTime) {

			// 少しだけスリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	// 現在の時間を記録
	reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::DescriptorHeapsInitialize() {

	// DescriptorSizeを取得しておく
	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// RTVのディスクリプタヒープの生成
	rtvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kMaxSRVCount, true);

	// SRVのディスクリプタヒープを作成する
	srvDescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// DSV用のディスクリプタヒープの数は1
	dsvdescriptorHeap = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));
}

void DirectXCommon::RTVInitialize() {

	// swapChainからResourceを引っ張ってくる
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	// RTVの設定
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// ディスクリプタの先頭を取得する
	rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVを2つ作るのでディスクリプタを2つ用意する
	// 表裏二つ分
	for (uint32_t i = 0; i < 2; ++i) {

		// RTVハンドルを取得
		rtvHandles[i].ptr = rtvStartHandle.ptr + i * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// RTVを作成
		device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);
	}
}

void DirectXCommon::DSVInitialize() {
	// DSVの設定
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVヒープの先頭ハンドルを取得
	dsvHandle = dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// DSVを作成
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvHandle);
}

void DirectXCommon::FenceInitialize() {
	hr = device->CreateFence(fencevalue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::ViewportInitialize() {
	// ビューポートのサイズ
	viewport.Width = static_cast<float>(WinApp::kClientWidth);
	viewport.Height = static_cast<float>(WinApp::kClientHeight);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXCommon::ScissorRectInitialize() {
	// シザー矩形のサイズ
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;
}

void DirectXCommon::DxcCompilerInitialize() {
	// ユーティリティの生成
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));

	// コンパイラの生成
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxCompiler));
	assert(SUCCEEDED(hr));

	// デフォルトインクルードハンドラの生成
	hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::ImGuiInitialize() {
	// IMGUIの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(), 2,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void DirectXCommon::PreDraw() {
	// バックバッファのインデックスを取得
	backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

	// 遷移前のresorceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

	// 遷移後のresorceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// TransitionBarrrierを張る
	commandList->ResourceBarrier(1, &barrier);

	dsvHandle = dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 描画先のレンダーターゲットを設定する
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

	// 指定する色で画面全体をクリアする
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画用のDesxriptorHeapを設定する
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());

	// ビューポートを設定する
	commandList->RSSetViewports(1, &viewport);

	// シザー矩形を設定する
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw() {
	// バックバッファのインデックスを取得
	backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// 今回はRenderTargetからpresentにする
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	commandList->ResourceBarrier(1, &barrier);

	// コマンドリストの内容を確定させる
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);

	// GPUとOSに画面の交換を行なうように通知する
	swapChain->Present(1, 0);

	// fenceの値を更新
	fencevalue++;

	// GPUがここまでたどり着いたときに、fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal(fence.Get(), fencevalue);

	// コマンド完了待ち
	if (fence->GetCompletedValue() < fencevalue) {
		fence->SetEventOnCompletion(fencevalue, fenceEvent);

		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// FPS固定更新
	UpdateFixFps();

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset(); // アロケーターのリセット
	assert(SUCCEEDED(hr));

	// コマンドリストのリセット
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::WaitForGPU() {
	fencevalue++;
	commandQueue->Signal(fence.Get(), fencevalue);

	if (fence->GetCompletedValue() < fencevalue) {
		fence->SetEventOnCompletion(fencevalue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void DirectXCommon::ResetCommandList() {
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);
}