#include "TextureManager.h"
#include <StringUtility.cpp>
using namespace StringUtility;

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}

void TextureManager::Initialize() {
	// SRVの数と同数
	textureDatas.resize(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath) {

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
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
	dxCommon->CreateTextuerResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックス番号とする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1);
	// SRVのCPUハンドルを取得
	textureData.srvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(srvIndex);
	// SRVのGPUハンドルを取得
	textureData.srvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(srvIndex);

	// ミップマップを返す
	return mipImage;
}

void TextureManager::Finalize() {
	delete instance;
	instance = nullptr;
}