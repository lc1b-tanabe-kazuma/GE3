#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "Math.h"

class  SpriteCommon;

class Sprite {
public:

	void Initialize(SpriteCommon* spriteCommon);

	void Update();

	void Draw();

	const Math::Vector2& GetPosition() { return position; }

	void SetPosition(const Math::Vector2& pos) { this->position = pos; }
private:
	SpriteCommon* spriteCommon_ = nullptr;

	// 頂点データ
	struct VertexData {
		Math::Vector4 position; // 頂点の位置
		Math::Vector2 texcoord;
		Math::Vector3 nomal;
	};

	// マテリアルデータ
	struct Material {
		Math::Vector4 color;
		int32_t enableLighting;
		float padding[3]; // パディングを追加して16バイト境界に揃える
		Math::Matrix4x4 uvTranseform;
	};

	// 座標変換データ
	struct TransfomationMatrix {
		Math::Matrix4x4 WVP;
		Math::Matrix4x4 World;
	};

	// Sprite用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource;
	// Indexの頂点リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResource;

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// マテリアルのリソースを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;

	// マテリアルのデータを作成する
	Material* materialData = nullptr;

	// TransformationMatrixのリソースを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResource;

	// バッファリソース内のデータを指すポインタ
	TransfomationMatrix* transeformationMatrixData = nullptr;

	// transformの初期化
	Math::TransForm transform;

	Math::Vector2 position = { 0.0f,0.0f };
};