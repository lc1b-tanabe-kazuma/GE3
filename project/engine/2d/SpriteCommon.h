#pragma once
#include "DirectXCommon.h"

class SpriteCommon {
public:
	
	void Initialize(DirectXCommon* dxCommon);
	
	DirectXCommon* GetDirectXCommon() { return dxCommon_; }
	
	// 共通描画設定
	void SetCommonDrawSetting();
private:

	DirectXCommon* dxCommon_;

	// RootSignatureを作成する
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;

	// 
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};

	// PSOを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	// ルートパラメータの数
	D3D12_ROOT_PARAMETER rootParameters[4] = {};

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

	// InputLayout
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 実際に作成
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;

	D3D12_STATIC_SAMPLER_DESC staticSamlers[1] = {};

	// DescriptorRange（SRV）
	D3D12_DESCRIPTOR_RANGE descriptorRange{};

	// ルートシグネチャーの作成
	void CreateRootSignature();

	// グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();
};