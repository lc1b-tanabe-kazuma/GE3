#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "mymath.h"
#include <string>  

class  SpriteCommon;

class Sprite {
public:

	void Initialize(SpriteCommon* spriteCommon, std::string textureFilePath);

	void Update();

	void Draw();

	const Math::Vector2& GetPosition() { return position; }

	void SetPosition(const Math::Vector2& pos) { this->position = pos; }

	float GetRotation() { return rotation; }
	void SetRotation(float rot) { this->rotation = rot; }

	// 色変更
	const Math::Vector4& GetColor() { return materialData->color; }
	void SetColor(const Math::Vector4& color) { this->materialData->color = color; }

	// サイズ変更
	const Math::Vector2& GetSize()const { return size; }
	void SetSize(const Math::Vector2& size) { this->size = size; }

	// アンカーのゲッター
	const Math::Vector2& GetAnchorPoint() const { return anchorPoint; }
	// アンカーのセッター
	void SetAnchorPoint(const Math::Vector2& anchor) { anchorPoint = anchor; }

	// 左右フリップのゲッター
	bool GetFlipX() const { return isFlipX; }

	// 左右フリップのセッター
	void SetFlipX(bool flip) { isFlipX = flip; }

	// 上下フリップのゲッター
	bool GetFlipY() const { return isFlipY; }

	// 上下フリップのセッター
	void SetFlipY(bool flip) { isFlipY = flip; }

	// テクスチャ切り出し左上座標のセッター
	void SetTextureLeftTop(const Math::Vector2& leftTop) { textureLeftTop = leftTop; }
	// テクスチャ切り出しサイズのセッター
	void SetTextureSize(const Math::Vector2& size) { textureSize = size; }
	// テクスチャ切り出し左上座標のゲッター
	const Math::Vector2& GetTextureLeftTop() const { return textureLeftTop; }
	// テクスチャ切り出しサイズのゲッター
	const Math::Vector2& GetTextureSize() const { return textureSize; }

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

	float rotation = 0.0f;

	Math::Vector2 size = { 640.0f,360.0f };

	// テクスチャ番号
	uint32_t textureIndex = 0;

	// アンカーポイント(0.0~1.0)
	Math::Vector2 anchorPoint = { 0.0f,0.0f };

	// 左右フリップ
	bool isFlipX = false;
	bool isFlipY = false;

	// テクスチャ左上座標
	Math::Vector2 textureLeftTop = { 0.0f,0.0f };
	// テクスチャ切り出しサイズ
	Math::Vector2 textureSize = { 100.0f,100.0f };

	// テクスチャサイズをイメージに合わせる
	void AbjustSizeToTexture();
};