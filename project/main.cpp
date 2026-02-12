#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <fstream>
#include <cassert>
#include <strsafe.h>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#include <wrl.h>
#include <xaudio2.h>
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "D3DResourceLeakChecker.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Mymath.h"
#include "TextureManager.h"
#include <iostream>

#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")

using namespace Math;

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

// チャンクヘッダの構造体
struct ChunkHeader {
	char id[4];
	int32_t size;
};

// RIFFチャンクの構造体
struct RiffHeder {
	ChunkHeader chunk; // チャンクヘッダ
	char type[4]; // フォーマット（RIFFの場合は"RIFF"）
};

// FMTチャンクの構造体
struct FomatChunk {
	ChunkHeader chunk;
	WAVEFORMATEX fmt; // WAVEフォーマット
};

// 音声データ
struct SoundData {
	WAVEFORMATEX wfex; // WAVEフォーマット

	// バッファの先頭アドレス
	BYTE* pBuffer;

	// バッファのサイズ
	unsigned int bufferSize;
};

// DepthStencilTexTure
Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTexturResource(const Microsoft::WRL::ComPtr <ID3D12Device>& device, int32_t width, int32_t height) {
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Resourceの生成
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
	Vector4 color = { 1.0f,1.0f,1.0f,1.0f }; // 拡散反射色
};
/*
// マテリアルデータを読み込む関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	// マテリアルファイルを読み込む
	MaterialData materialData;

	// ファイルから一行格納する
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);

	// 開けないと止める
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			// テクスチャファイルのパスを取得
			s >> textureFilename;
			// 連結して、ファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		} else if (identifier == "Kd") {
			// 拡散反射色をcolorに設定、アルファ値は1.0固定
			s >> materialData.color.x >> materialData.color.y >> materialData.color.z;
			materialData.color.w = 1.0f;
		}
	}

	// 読み込んだマテリアルデータを返す
	return materialData;
}

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material; // マテリアルデータ
};

// Objファイルを読み込む関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions; // 頂点位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "v") {

			// 頂点位置
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f; // w成分は1.0fに設定
			positions.push_back(position);
		} else if (identifier == "vt") {

			// テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y; // OpenGLとDirectXでY軸の方向が逆なので反転
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {

			// 法線
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		} else if (identifier == "f") {

			VertexData triangle[3];

			// 面の定義
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点要素へのindexは「位置/UV/法線」で格納されているので分解してindexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}

				// 要素へのIndexから実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1]; // Objファイルは1始まりなので-1する
				Vector2 texcoord = texcoords[elementIndices[1] - 1]; // Objファイルは1始まりなので-1する
				Vector3 normal = normals[elementIndices[2] - 1]; // Objファイルは1始まりなので-1する

				position.x *= -1.0f; // X軸を反転する（DirectXとOpenGLで座標系が異なるため）
				normal.x *= -1.0f; // 法線も反転する
				triangle[faceVertex] = { position, texcoord, normal };
			}

			// 頂点を逆順に登録する
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		} else if (identifier == "mtllib") {
			// マテリアルファイルの読み込み
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	// ファイルを閉じる
	file.close();
	// 読み込んだ頂点データを返す
	return modelData;
}*/

SoundData SoundLoadWave(const char* filename) {

	// ファイルオープン
	std::ifstream file;

	// .wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);

	// エラー処理
	assert(file.is_open());

	// RIFFヘッダの読み込み
	RiffHeder riff;
	file.read((char*)&riff, sizeof(riff));
	// RIFFヘッダのチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	// WAVEヘッダのチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// fmtチャンクの読み込み
	FomatChunk format = {};
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// fmtチャンクのサイズをチェック
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	// JUNKチャンクの読み飛ばし
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// JUNKチャンクのサイズを読み飛ばす
		file.seekg(data.size, std::ios_base::cur);
		// 次のチャンクヘッダを読み込む
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// データチャンクのサイズをチェック
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	file.close();

	// returnする為の音声データ
	SoundData soundData = {};
	soundData.wfex = format.fmt; // WAVEフォーマット
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer); // 音声データのポインタ
	soundData.bufferSize = data.size; // 音声データのサイズ
	return soundData;
}

// 音声データの解放
void SoundUnload(SoundData* soundData) {
	// バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0; // ポインタを0にする
	soundData->bufferSize = 0; // サイズを0にする
	soundData->wfex = {}; // WAVEフォーマットを初期化
}

// 音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {
	HRESULT result;

	// 波形フォーマットを元にサウンドエフェクトを作成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 音声データを再生する
	XAUDIO2_BUFFER buf = {};
	buf.pAudioData = soundData.pBuffer; // 音声データのポインタ
	buf.AudioBytes = soundData.bufferSize; // 音声データのサイズ
	buf.Flags = XAUDIO2_END_OF_STREAM; // 音声データの終端を示すフラグ

	// 波形データの再生	
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// リリースチェック
	D3DResourceLeakChecker resourceLeakChecker;
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif // _DEBUG

	// ウィンドウクラスの設定
	WinApp* winApp = nullptr;
	winApp = new WinApp();
	winApp->Initialize();

	// DirectX12初期化処理
	DirectXCommon* dxCommon = nullptr;
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	// 入力の初期化
	Input* input = nullptr;

	input = new Input();
	input->Initialize(winApp);

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxCommon);

	// Textureの読んで転送する
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	// 2枚目のTextureの読んで転送する
	TextureManager::GetInstance()->LoadTexture("resources/monsterBall.png");

	// スプライトの初期化
	SpriteCommon* spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	std::vector<std::string> textures = {
	"resources/uvChecker.png",
	"resources/monsterBall.png"
	};

	// スプライトの複数表示	
	std::vector<Sprite*> sprites;
	for (int32_t i = 0; i < 5; ++i) {
		Sprite* sprite = nullptr;
		sprite = new Sprite();
		sprite->Initialize(spriteCommon,
			textures[i % textures.size()]);
		sprite->SetPosition({ i * 128.0f, 100.0f });
		sprites.push_back(sprite);
	}

	// でかい画像を一枚
	Sprite* bigSprite = nullptr;
	bigSprite = new Sprite();
	bigSprite->Initialize(spriteCommon,
		"resources/uvChecker.png");
	bigSprite->SetPosition({ 400.0f,300.0f });
	bigSprite->SetSize({ 512.0f,512.0f });

	/*
	D3D12_STATIC_SAMPLER_DESC staticSamlers[1] = {};
	staticSamlers[0].Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	staticSamlers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamlers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamlers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//descriptionRootSignature.pStaticSamplers = staticSamlers;
	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamlers);
*/
// 関数が成功したか判定する
//HRESULT hr;

/*
// シリアライズしてバイナリする
Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
hr = D3D12SerializeRootSignature(&descriptionRootSignature,
	D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
if (FAILED(hr)) {
	//Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferSize()));
	assert(false);
}

// バイナリを元に作成
Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
hr = dxCommon->GetDevice()->CreateRootSignature(0,
	signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
	IID_PPV_ARGS(&rootSignature));
assert(SUCCEEDED(hr));
*/

// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// Shaderをコンパイルする
	/*Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);
*/
// モデル読み込み
//ModelData modelData = LoadObjFile("resources", "plane.obj");

//DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(modelData.material.textureFilePath);

/*	// 頂点リソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビュー設定
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	// 2. インデックスデータ生成
	uint32_t idx = 0;
	for (uint32_t lat = 0; lat < kSubdivision; ++lat) {
		for (uint32_t lon = 0; lon < kSubdivision; ++lon) {
			uint32_t i0 = lat * (kSubdivision + 1) + lon;
			uint32_t i1 = i0 + 1;
			uint32_t i2 = i0 + (kSubdivision + 1);
			uint32_t i3 = i2 + 1;

			// 三角形1
			indexData[idx++] = i0;
			indexData[idx++] = i2;
			indexData[idx++] = i1;

			// 三角形2
			indexData[idx++] = i1;
			indexData[idx++] = i2;
			indexData[idx++] = i3;
		}
	}

	// インデックスバッファ用GPUリソースを作成
	//Microsoft::WRL::ComPtr <ID3D12Resource> indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * indexCount);

	// 7. インデックスバッファビュー設定
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
*/
// マテリアルのデータを作成する
/*Material* materialData = nullptr;

// マテリアルのリソースを作成する
Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));

// 書き込むアドレスを取得する
materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

// マテリアルの色
materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

// 球体のLighttingをする
materialData->enableLighting = true;

// UVTransformのデータを作成する
materialData->uvTranseform = makeIdentity4x4();

// Sprite用のマテリアルデータ
Material* materialDataSprite = nullptr;

// Sprite用のマテリアルリソースを作る
Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSprite = dxCommon->CreateBufferResource(sizeof(Material));

// 書き込むアドレスを取得する
materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

// アドレスに書き込む
materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

// SpriteはLighttingしない
materialDataSprite->enableLighting = false;

// UVTranseformのデータを作成する
materialDataSprite->uvTranseform = makeIdentity4x4();
*/
//UVTranseformのデータを作成する
	TransForm uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};


	// 平行光源用のデータ
	DirectionalLight* directionalLightData = nullptr;

	// 平行光源用のリソースを作成
	Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = dxCommon->CreateBufferResource(sizeof(DirectionalLight));

	// 書き込むアドレスを取得する
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	// アドレスに書き込む
	directionalLightData->color = Vector4{ 1.0f,1.0f,1.0f,1.0f }; // 白色
	directionalLightData->direction = { 0.0f,-1.0f,0.0f }; // 向き
	directionalLightData->intensity = 1.0f; // 強さ

	directionalLightResource->Unmap(0, nullptr);

	// ビューポート
	D3D12_VIEWPORT viewport{};

	// ビューポートのサイズ
	viewport.Width = static_cast<float>(WinApp::kClientWidth);
	viewport.Height = static_cast<float>(WinApp::kClientHeight);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// シザー矩形
	D3D12_RECT scissorRect{};

	// シザー矩形のサイズ
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;

	// WVP用のリソースを作成する
	//Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = dxCommon->CreateBufferResource(sizeof(TransfomationMatrix));

	// WVPのデータを作成する
	//TransfomationMatrix* wvpData = nullptr;

	// 書き込むアドレスを取得する
//	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	// 単位行列を書き込む
	//wvpData->World = makeIdentity4x4();

	// Sprite用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 4);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	// リソースの先頭アドレスから使う
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	// 使用するリソースもサイズは頂点4つ分
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;

	// 1頂点あたりのサイズ
//	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	/*VertexData* vertexDataSprite = nullptr;

	// 書き込むアドレスを取得する
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	// 左下
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };

	// 上
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };

	// 右下
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };

	// 右上
	vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 1.0f,0.0f };

	vertexDataSprite[0].nomal = { 0.0f,0.0f,1.0f };
	vertexDataSprite[1].nomal = { 0.0f,0.0f,1.0f };
	vertexDataSprite[2].nomal = { 0.0f,0.0f,1.0f };
	vertexDataSprite[3].nomal = { 0.0f,0.0f,1.0f };
*/
// Indexの頂点リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

	// index
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};

	// リソースの先頭アドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();

	// 使用するリソースのサイズインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;

	// インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;

	// Sprite用のTransformationMatrix用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(TransfomationMatrix));

	// データを書き込む
//	TransfomationMatrix* transeformationMatrixDataSprite = nullptr;

	// 書き込む為のアドレスを取得
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transeformationMatrixDataSprite));

	// 単位行列を書き込む
//	transeformationMatrixDataSprite->World = makeIdentity4x4();

	// transformの初期化
	TransForm transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	TransForm cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-5.0f} };

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// DepthStencilTexture
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStencilTexturResource(
		dxCommon->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight);

	transform.rotate.y = 3.0f;

	// ウィンクラのxボタンが押されるまでループ
	while (true) {

		// メッセージ処理
		if (winApp->ProcessMessage()) {

			// ループを抜ける
			break;
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		input->Update();

		//===========================
		// ゲーム処理
		//===========================

		/*
		// ImGuiの処理
		// ImGuiのウィンドウを作成
		ImGui::Begin("Plane.obj");
		// ImGuiで三角形の色を変える
		//ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&materialData->color.x));
		// ImGuiで三角形の位置を変える
		ImGui::DragFloat3("Translate", reinterpret_cast<float*>(&transform.translate.x), 0.01f);
		// ImGuiで三角形の回転を変える
		ImGui::DragFloat3("Rotate", reinterpret_cast<float*>(&transform.rotate.x), 0.01f);
		// ImGuiで三角形のスケールを変える
		ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&transform.scale.x), 0.01f);
		// ImGuiで光源の向きを変える
		ImGui::DragFloat3("LightDirection", reinterpret_cast<float*>(&directionalLightData->direction.x), 0.01f);
		// 光源を正規化する
		directionalLightData->direction = Normalize(directionalLightData->direction);
		// ImGuiで光源の色を変える
		ImGui::ColorEdit3("LightColor", reinterpret_cast<float*>(&directionalLightData->color.x));
		// ImGuiで光源の強さを変える
		ImGui::DragFloat("LightIntensity", &directionalLightData->intensity, 0.01f, 0.0f, 10.0f);
		// ImGuiでTextureを切り替える
		ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		// ImGuiのウィンドウを閉じる
		ImGui::End();
		*/

		// ImGuiのSpriteウィンドウを作成
		ImGui::Begin("Sprite");

		int index = 0;
		for (Sprite* sprite : sprites) {

			Vector2 pos = sprite->GetPosition(); // ← 自分自身！

			// ImGuiでスプライトの位置を変える
			std::string label = "Position##" + std::to_string(index);
			ImGui::DragFloat2(label.c_str(), reinterpret_cast<float*>(&pos.x), 0.1f);

			// imguiで回転させる
			float rotate = sprite->GetRotation();
			ImGui::DragFloat(("Rotate##" + std::to_string(index)).c_str(),
				reinterpret_cast<float*>(&rotate), 0.1f);

			sprite->SetPosition(pos);
			sprite->SetRotation(rotate);
			index++;
		}



		// ImGuiのウィンドウを閉じる
		ImGui::End();

		ImGui::Begin("BigSprite");
		Vector2 bigPos = bigSprite->GetPosition();
		// ImGuiででかいスプライトの位置を変える
		ImGui::DragFloat2("BigSpritePosition", reinterpret_cast<float*>(&bigPos.x), 1.0f);
		bigSprite->SetPosition(bigPos);
		// ImGuiででかいスプライトの回転を変える
		float bigRotate = bigSprite->GetRotation();
		ImGui::DragFloat("BigSpriteRotate", reinterpret_cast<float*>(&bigRotate), 0.01f);
		bigSprite->SetRotation(bigRotate);
		// でかいスプライトのアンカーを変える
		bigPos = bigSprite->GetAnchorPoint();
		ImGui::DragFloat2("BigSpriteAnchor", reinterpret_cast<float*>(&bigPos.x), 0.01f, 0.0f, 1.0f);
		bigSprite->SetAnchorPoint(bigPos);

		// でかいスプライトのフリップを変える
		bool flipX = bigSprite->GetFlipX();
		bool flipY = bigSprite->GetFlipY();
		ImGui::Checkbox("BigSpriteFlipX", &flipX);
		ImGui::Checkbox("BigSpriteFlipY", &flipY);
		bigSprite->SetFlipX(flipX);
		bigSprite->SetFlipY(flipY);

		// ImGuiで切り出しサイズを変える
		Vector2 bigSize = bigSprite->GetTextureSize();
		ImGui::DragFloat2("BigSpriteTextureSize", reinterpret_cast<float*>(&bigSize.x),0.1f);
		bigSprite->SetTextureSize(bigSize);

		ImGui::End();

		// spriteの更新
		for (Sprite* sprite : sprites) {
			sprite->Update();
		}

		// でかいスプライトの更新
		bigSprite->Update();

		// 変更を反映させる

		/*
		// レンダリングパイプライン
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
			0.45f, static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight), 0.1f, 100.0f);
		Matrix4x4 wvpMatrix = Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);
		//wvpData->WVP = wvpMatrix;
		//wvpData->World = wvpMatrix;

		// UVTransformの更新
		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		uvTransformMatrix = Multiply(MakeRotZMatrix(uvTransformSprite.rotate.z), uvTransformMatrix);
		uvTransformMatrix = Multiply(MakeTransMatrix(uvTransformSprite.translate), uvTransformMatrix);
		//materialDataSprite->uvTranseform = uvTransformMatrix;
*/

		ImGui::Render();
		// ↑ ゲーム処理はここまで

		// 書き込むアドレスを取得する
		//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

		// 描画前処理
		dxCommon->PreDraw();

		// Sprite描画前処理
		spriteCommon->SetCommonDrawSetting();

		// RootSignatureを設定する
		//dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

		// PSOを設定する
		//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
		//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

		// 形状の設定
		//dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 三角形のCBufferを設定
		//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

		// 平行光源用のCBufferを設定
		//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

		// SRVのDescriptorTableの設定
		//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

		// 球体用のIBVを設定
		//dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

		// 三角形を描画する
		//commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	//	dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		// IBVを設定
		//dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);

		// SpriteのマテリアルCBufferの場所を設定
		//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

		// TransformationMatrixCBufferの場所を設定
		//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

		// Spriteの表示する画像を設定
		//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

		for (Sprite* sprite : sprites) {
			sprite->Draw();
		}

		// でかいスプライトの描画
		bigSprite->Draw();

		// Spriteの描画
		//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

		// 描画する
		//dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// 実際のcommandListのImGuiの描画を行う
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		// 描画後処理
		dxCommon->PostDraw();
	}

	// 解放処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// WindowsAPIの終了処理
	winApp->Finalize();

	// テクスチャマネージャーの終了処理
	TextureManager::GetInstance()->Finalize();

	delete input;
	delete winApp;
	for (auto sprite : sprites) {
		delete sprite;
	}
	delete bigSprite;
	delete spriteCommon;
	delete dxCommon;
	return 0;
}