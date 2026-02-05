#include "Sprite.h"
#include "SpriteCommon.h"

using namespace Math;

void Sprite::Initialize(SpriteCommon* spriteCommon) {
	// 引数をメンバ変数にセット
	this->spriteCommon_ = spriteCommon;

	// Sprite用のリソースを作る
	vertexResource = spriteCommon_->GetDirectXCommon()->CreateBufferResource(sizeof(VertexData) * 4);

	// Indexの頂点リソース
	indexResource = spriteCommon_->GetDirectXCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

	// Vertexバッファビュー設定
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * 4);
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// Indexバッファビュー設定
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// マテリアルリソースを作成する
	materialResource = spriteCommon_->GetDirectXCommon()->CreateBufferResource(sizeof(Material));

	// マテリアルリソースにデータを書き込む為のアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	// マテリアルデータの初期化
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false; // スプライトにライティングは不要
	materialData->uvTranseform = makeIdentity4x4();

	// 座標変換リソースを作れる
	transformationMatrixResource = spriteCommon_->GetDirectXCommon()->CreateBufferResource(sizeof(TransfomationMatrix));

	// 座標変換リソースにデータを書き込む為のアドレスを取得してtransformationMatrixDataにセット
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transeformationMatrixData));

	// 単位行列を書き込む
	transeformationMatrixData->WVP = makeIdentity4x4();
	transeformationMatrixData->World = makeIdentity4x4();

	// transformの初期化
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
}

void Sprite::Update() {

	// 頂点リソースにデータを書き込む(4点分)
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 左下
	vertexData[0].position = { 0.0f,1.0f,0.0f,1.0f };
	vertexData[0].texcoord = { 0.0f,1.0f };

	// 上
	vertexData[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexData[1].texcoord = { 0.0f,0.0f };

	// 右下
	vertexData[2].position = { 1.0f,1.0f,0.0f,1.0f };
	vertexData[2].texcoord = { 1.0f,1.0f };

	// 右上
	vertexData[3].position = { 1.0f,0.0f,0.0f,1.0f };
	vertexData[3].texcoord = { 1.0f,0.0f };

	// 法線
	vertexData[0].nomal = { 0.0f,0.0f,1.0f };
	vertexData[1].nomal = { 0.0f,0.0f,1.0f };
	vertexData[2].nomal = { 0.0f,0.0f,1.0f };
	vertexData[3].nomal = { 0.0f,0.0f,-1.0f };

	// インデックスリソースにデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;

	// 書き込む為のアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transeformationMatrixData));

	// 単位行列を書き込む
	transeformationMatrixData->World = makeIdentity4x4();

	/// ============= 諸々の処理 =================
	transform.translate = { position.x,position.y,0.0f };

	transform.rotate = { 0.0f,0.0f,rotation };

	// サイズ反映
	transform.scale = { size.x,size.y,1.0f };

	// スプライト用のレンダリングパイプライン
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewmatrixSprite = makeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	transeformationMatrixData->WVP = Multiply(Multiply(worldMatrix, viewmatrixSprite), projectionMatrixSprite);
	transeformationMatrixData->World = worldMatrix;
}

void Sprite::Draw() {

	// 頂点バッファをセット
	spriteCommon_->GetDirectXCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// インデックスバッファをセット
	spriteCommon_->GetDirectXCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	// 形状の設定
	spriteCommon_->GetDirectXCommon()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// スプライト用のマテリアルCBufferを設定
	spriteCommon_->GetDirectXCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	// スプライト用のTransformationMatrixCBufferを設定
	spriteCommon_->GetDirectXCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

	// 描画コマンド
	spriteCommon_->GetDirectXCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}