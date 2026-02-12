#include "SpriteCommon.h"

void SpriteCommon::Initialize(DirectXCommon* dxCommon) {

	// 引数をメンバ変数にセット
	dxCommon_ = dxCommon;

	// グラフィックスパイプラインの作成
	CreateGraphicsPipeline();
}

void SpriteCommon::CreateRootSignature() {

	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// DescriptorRange（SRV）
	descriptorRange.BaseShaderRegister = 0;
	descriptorRange.NumDescriptors = 1;
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameters
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	staticSamlers[0].Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	staticSamlers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamlers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamlers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamlers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamlers);

	// ===== ここからが重要 =====

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob
	);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);
	assert(SUCCEEDED(hr));
}

void SpriteCommon::CreateGraphicsPipeline() {

	// ① RootSignature を作成
	CreateRootSignature();

	// ② Shader
	Microsoft::WRL::ComPtr <IDxcBlob> vsBlob = dxCommon_->CompileShader(
		L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
	Microsoft::WRL::ComPtr <IDxcBlob> psBlob = dxCommon_->CompileShader(
		L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
	assert(vsBlob && psBlob);

	// ③ BlendState（最低限）
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// ④ RasterizerState
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.DepthClipEnable = TRUE;

	// ⑤ InputLayout
	inputElementDescs[0] = {
		"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};
	inputElementDescs[1] = {
		"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};
	inputElementDescs[2] = {
		"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
		0, D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
	};

	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// ⑥ PSO Desc を埋める
	graphicsPipelineStateDesc = {};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = {
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()
	};
	graphicsPipelineStateDesc.PS = {
		psBlob->GetBufferPointer(), psBlob->GetBufferSize()
	};
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] =
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ⑦ PSO 作成（これが本体）
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}

void SpriteCommon::SetCommonDrawSetting() {

	// ルートシグネチャーをセット
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

	// パイプラインステートをセット
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

	// プリミティブトポロジーを設定
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}