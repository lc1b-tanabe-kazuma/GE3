#pragma once
#include <string>
#include <wrl/client.h>
#include <d3d12.h>
#include <vector>
#include <cstdint>
#include "DirectXTex/DirectXTex.h"
#include "DirectXCommon.h"

// 前方宣言
class DirectXCommon;

class TextureManager {
public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();

	// 初期化
	void Initialize(DirectXCommon* dxCommon);

	// 終了
	static void Finalize();

	// LoadTexture関数
	void LoadTexture(const std::string& filePath);

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	// テクスチャ番号からSRVのGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(uint32_t textureIndex);

	// メタデータを取得
	const DirectX::TexMetadata& GetTextureMetadata(uint32_t textureIndex);

private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	// テクスチャ一枚分のデータ
	struct TextuerData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	// テクスチャデータ
	std::vector<TextuerData> textureDatas;

	DirectXCommon* dxCommon_;

	// SRVインデックスの開始番号
	static uint32_t kSRVtIndexTop;

	DirectX::ScratchImage image{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
};