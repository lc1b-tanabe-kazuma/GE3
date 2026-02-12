#include "TextureManager.h"
#include "StringUtility.h"

using namespace StringUtility;

// インスタンスの初期化
TextureManager* TextureManager::instance = nullptr;

// ImGuiで0以外のTextureを使うときに便利
uint32_t TextureManager::kSRVtIndexTop = 1;

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}

void TextureManager::Initialize(DirectXCommon* dxCommon) {
	this->dxCommon_ = dxCommon;
	// SRVの数と同数
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath) {

	// 読み込みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(), textureDatas.end(),
		[&](const TextuerData& data) {
			return data.filePath == filePath;
		});

	if (it != textureDatas.end()) {
		// 既に読み込まれている場合は終了
		return;
	}

	// テクスチャの上限数をチェック
	assert(textureDatas.size() + kSRVtIndexTop < DirectXCommon::kMaxSRVCount);

	// テクスチャファイルを読んでプログラムで扱えるようにする
	std::wstring wFilePath = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(wFilePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップを生成する
	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);

	// 追加したテクスチャデータの参照を取得する
	TextuerData& textureData = textureDatas.back();

	// ファイルパスを保存
	textureData.filePath = filePath;
	// メタデータを保存
	textureData.metadata = mipImage.GetMetadata();
	// テクスチャリソースを生成
	textureData.resource = dxCommon_->CreateTextuerResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックス番号とする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVtIndexTop;
	// SRVのCPUハンドルを取得
	textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
	// SRVのGPUハンドルを取得
	textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

	// SRVを設定
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);

	// 設定を基にSRVを生成
	dxCommon_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(), // リソース
		&srvDesc, // SRVの設定
		textureData.srvHandleCPU); // ハンドル

	// すぐに転送完了をまつ
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		dxCommon_->UploadTextureData(textureData.resource, mipImage);

	// commandListをCloseし、commandQueueに投げる
	ID3D12CommandList* commandLists[] = {
		dxCommon_->GetCommandList()
	};
	dxCommon_->GetCommandList()->Close();
	dxCommon_->GetCommandQueue()->ExecuteCommandLists(1, commandLists);

	// 実行完了を待つ（Fence）
	dxCommon_->WaitForGPU();

	// コマンドアロケーターとコマンドリストをリセット
	dxCommon_->GetCommandAllocator()->Reset();
	dxCommon_->ResetCommandList();
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
	auto it = std::find_if(
		textureDatas.begin(), textureDatas.end(),
		[&filePath](const TextuerData& data) {
			return data.filePath == filePath;
		});

	assert(it != textureDatas.end());

	uint32_t dataIndex =
		static_cast<uint32_t>(std::distance(textureDatas.begin(), it));

	// ★ ここが重要 ★
	return dataIndex + kSRVtIndexTop;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSRVHandleGPU(uint32_t textureIndex) {
	// SRV index は ImGui 領域より後である必要がある
	assert(textureIndex >= kSRVtIndexTop);

	uint32_t dataIndex = textureIndex - kSRVtIndexTop;

	// 実際のテクスチャ数を超えていないか
	assert(dataIndex < textureDatas.size());

	return textureDatas[dataIndex].srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetTextureMetadata(uint32_t textureIndex) {
	// SRV index は ImGui 領域より後である必要がある
	assert(textureIndex >= kSRVtIndexTop);
	uint32_t dataIndex = textureIndex - kSRVtIndexTop;
	// 実際のテクスチャ数を超えていないか
	assert(dataIndex < textureDatas.size());
	return textureDatas[dataIndex].metadata;
}

void TextureManager::Finalize() {
	delete instance;
	instance = nullptr;
}