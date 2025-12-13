#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <cstdint>
#include <string>
//#include <format>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <d3d12.h> 
#include <dxgi1_6.h> 
#include <cassert>
#include <dbgHelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <wrl.h>
#include <xaudio2.h>
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DbgHelp.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "xaudio2.lib")

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentProcessId();
	MINIDUMP_EXCEPTION_INFORMATION miniDumpInformation{ 0 };
	miniDumpInformation.ThreadId = threadId;
	miniDumpInformation.ExceptionPointers = exception;
	miniDumpInformation.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &miniDumpInformation, nullptr, nullptr);
	return EXCEPTION_EXECUTE_HANDLER;
}

void Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

// Vector4の構造体
struct Vector4 {
	float x, y, z, w;
};

// Vector3の構造体
struct Vector3 {
	float x, y, z;
};

// Vector2の構造体
struct Vector2 {
	float x, y;
};

// 3x3の行列の構造体
struct Matrix3x3 {
	float m[3][3];
};

// 4x4の行列の構造体
struct Matrix4x4 {
	float m[4][4];
};

struct TransForm {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position; // 頂点の位置
	Vector2 texcoord;
	Vector3 nomal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3]; // パディングを追加して16バイト境界に揃える
	Matrix4x4 uvTranseform;
};

struct TransfomationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

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

// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = 0.0f;
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
};

// 逆行列
// 3x3行列式（余因子計算用）
float Det3(float a1, float a2, float a3,
	float b1, float b2, float b3,
	float c1, float c2, float c3) {
	return a1 * (b2 * c3 - b3 * c2)
		- a2 * (b1 * c3 - b3 * c1)
		+ a3 * (b1 * c2 - b2 * c1);
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result = {};

	float det =
		m.m[0][0] * Det3(m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) -
		m.m[0][1] * Det3(m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) +
		m.m[0][2] * Det3(m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) -
		m.m[0][3] * Det3(m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]);

	if (det == 0.0f) {
		// 行列が正則でない場合、単位行列または0行列などを返してもOK
		return result;
	}

	float invDet = 1.0f / det;

	// 余因子 + 転置で逆行列を一気に展開（各要素ベタ書き）
	result.m[0][0] = Det3(m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
	result.m[1][0] = -Det3(m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
	result.m[2][0] = Det3(m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
	result.m[3][0] = -Det3(m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

	result.m[0][1] = -Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
	result.m[1][1] = Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
	result.m[2][1] = -Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
	result.m[3][1] = Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

	result.m[0][2] = Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
	result.m[1][2] = -Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
	result.m[2][2] = Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
	result.m[3][2] = -Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;

	result.m[0][3] = -Det3(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3]) * invDet;
	result.m[1][3] = Det3(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3]) * invDet;
	result.m[2][3] = -Det3(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3]) * invDet;
	result.m[3][3] = Det3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]) * invDet;

	return result;
}

// 平行移動行列
Matrix4x4 MakeTransMatrix(const Vector3& v) {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	result.m[3][0] = v.x;
	result.m[3][1] = v.y;
	result.m[3][2] = v.z;
	return result;
}

//拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& v) {
	Matrix4x4 result = {};
	result.m[0][0] = v.x;
	result.m[1][1] = v.y;
	result.m[2][2] = v.z;
	result.m[3][3] = 1.0f;
	return result;
}

// 座標変換
Vector3 MakeTransform(const Matrix4x4& m, const Vector3& v) {
	Vector3 result = {};
	result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0];
	result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1];
	result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2];

	// w成分
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

	// wが0でない場合、結果をwで割る
	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}
	return result;
}

// 長さ
float Length(const Vector3& v) {

	float result = {};

	result = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));

	return result;
};

// ベクトルの正規化
Vector3 Normalize(const Vector3& v) {

	Vector3 result = {};
	float length = Length(v);
	if (length != 0.0f) {
		result.x = v.x / Length(v);
		result.y = v.y / Length(v);
		result.z = v.z / Length(v);
	}
	return result;
};

// X軸の回転行列
Matrix4x4 MakeRotXMatrix(float radian) {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = std::cosf(radian);
	result.m[1][2] = std::sinf(radian);
	result.m[2][1] = -std::sinf(radian);
	result.m[2][2] = std::cosf(radian);
	result.m[3][3] = 1.0f;
	return result;
}

// Y軸の回転行列
Matrix4x4 MakeRotYMatrix(float radian) {
	Matrix4x4 result = {};
	result.m[0][0] = std::cosf(radian);
	result.m[0][2] = -std::sinf(radian);
	result.m[1][1] = 1.0f;
	result.m[2][0] = std::sinf(radian);
	result.m[2][2] = std::cosf(radian);
	result.m[3][3] = 1.0f;
	return result;
}

// Z軸の回転行列
Matrix4x4 MakeRotZMatrix(float radian) {
	Matrix4x4 result = {};
	result.m[0][0] = std::cosf(radian);
	result.m[0][1] = std::sinf(radian);
	result.m[1][0] = -std::sinf(radian);
	result.m[1][1] = std::cosf(radian);
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

// 単位行列を作る
Matrix4x4 makeIdentity4x4() {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

// 3次元アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

	// スケーリング行列の作成
	Matrix4x4 matScale = MakeScaleMatrix(scale);

	Matrix4x4 matRotX = MakeRotXMatrix(rotate.x);
	Matrix4x4 matRotY = MakeRotYMatrix(rotate.y);
	Matrix4x4 matRotZ = MakeRotZMatrix(rotate.z);

	// 回転行列の合成
	Matrix4x4 matRot = Multiply(Multiply(matRotY, matRotX), matRotZ);

	// 平行移動行列の作成
	Matrix4x4 matTrans = MakeTransMatrix(translate);

	// スケーリング、回転、平行移動の合成
	Matrix4x4 matTransform = Multiply(Multiply(matScale, matRot), matTrans);

	return matTransform;
}

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result = {};
	float f = 1.0f / std::tanf(fovY / 2.0f);
	result.m[0][0] = (f * (1.0f / aspectRatio));
	result.m[1][1] = f;
	result.m[2][2] = (farClip) / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
	return result;
}

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result = {};
	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (right + left) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = (nearClip) / (nearClip - farClip);
	result.m[3][3] = 1.0f;
	return result;
}

// DescriptorHeapを作成する関数
Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr <ID3D12Device>& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisble) {
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisble ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}


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
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

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

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

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

	/*
	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ダンプファイルを作成
	SetUnhandledExceptionFilter(ExportDump);

	// ログのディレクトリを作成
	std::filesystem::create_directory("logs");

	// 現在の時刻を取得(UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルの名前にコンマはいらないので、秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	//日本時間に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };

	//
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);

	//
	std::string logFilePath = std::string("logs/") + dateString + ".log";

	//
	std::ofstream logStream(logFilePath);
*/
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

	// RootSignatureを作成する
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの数
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	D3D12_STATIC_SAMPLER_DESC staticSamlers[1] = {};
	staticSamlers[0].Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	staticSamlers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamlers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamlers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamlers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamlers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamlers);

	// 関数が成功したか判定する
	HRESULT hr;

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

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(L"resources/shaders/Object3D.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// PSOを作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 利用するトポロジのタイプ(三角形)
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どの様に画面に色を打ち込むか設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// depthの機能を有効化
	depthStencilDesc.DepthEnable = true;

	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	// 比較関数をLessEqual
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 実際に作成
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	// 頂点リソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	// バッファリソーステクスチャの場合は別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeof(VertexData) * 3;

	// バッファの場合1にする
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	// バッファの場合これにする
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 分割数
	uint32_t kSubdivision = 16;

	// 頂点数とインデックス数
	const uint32_t kVertexCount = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t indexCount = kSubdivision * kSubdivision * 6;

	// 頂点用とインデックス用のCPU配列
	//std::vector<VertexData> vertexData(kVertexCount);
	std::vector<uint32_t> indexData(indexCount);

	// 円周率
	const float pi = 3.14159265358979f;
	const float kLatEvery = pi / float(kSubdivision);
	const float kLonEvery = 2.0f * pi / float(kSubdivision);

	// モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");

	DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(modelData.material.textureFilePath);

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

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
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * indexCount);

	// 7. インデックスバッファビュー設定
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// マテリアルのデータを作成する
	Material* materialData = nullptr;

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
	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = dxCommon->CreateBufferResource(sizeof(TransfomationMatrix));

	// WVPのデータを作成する
	TransfomationMatrix* wvpData = nullptr;

	// 書き込むアドレスを取得する
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	// 単位行列を書き込む
	wvpData->World = makeIdentity4x4();

	// Sprite用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 4);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	// リソースの先頭アドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();

	// 使用するリソースもサイズは頂点4つ分
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;

	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	VertexData* vertexDataSprite = nullptr;

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
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(TransfomationMatrix));

	// データを書き込む
	TransfomationMatrix* transeformationMatrixDataSprite = nullptr;

	// 書き込む為のアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transeformationMatrixDataSprite));

	// 単位行列を書き込む
	transeformationMatrixDataSprite->World = makeIdentity4x4();

	// transformの初期化
	TransForm transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	TransForm transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	TransForm cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-5.0f} };

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dxCommon->GetSRVCPUDescriptorHandle(0);

	// Textureの読んで転送する
	DirectX::ScratchImage mipImage = dxCommon->LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImage.GetMetadata();
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = dxCommon->CreateTextuerResource(metadata);
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = dxCommon->UploadTextureData(textureResource, mipImage);

	// 2枚目のTextureの読んで転送する
	DirectX::ScratchImage mipImage2 = dxCommon->LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImage2.GetMetadata();
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = dxCommon->CreateTextuerResource(metadata2);
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource2 = dxCommon->UploadTextureData(textureResource2, mipImage2);

	// metadataを元にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// metadataを元にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// ImGuiを除いた 0番目のテクスチャ
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
		dxCommon->GetSRVCPUDescriptorHandle(1);

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
		dxCommon->GetSRVGPUDescriptorHandle(1);

	// ImGuiを除いた 2番目のテクスチャ
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 =
		dxCommon->GetSRVCPUDescriptorHandle(2);

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 =
		dxCommon->GetSRVGPUDescriptorHandle(2);

	// SRVを作成する
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

	// DepthStencilTexture
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStencilTexturResource(
		dxCommon->GetDevice(), WinApp::kClientWidth, WinApp::kClientHeight);

	// DSV用のヒープディスクリプタの数は1
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvdescriptorHeap = CreateDescriptorHeap(dxCommon->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVHeapの先頭にDSVを作る
	dxCommon->GetDevice()->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Textuer切り替え変数
	bool useMonsterBall = false;

	transform.rotate.y = 3.0f;

	// XAudio2の初期化
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
	HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));

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

		// ImGuiの処理
		// ImGuiのウィンドウを作成
		ImGui::Begin("Plane.obj");
		// ImGuiで三角形の色を変える
		ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&materialData->color.x));
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

		// ImGuiのSpriteウィンドウを作成
		ImGui::Begin("Sprite");
		// ImGuiでSpriteの位置を変える
		ImGui::DragFloat3("TranslateSprite", reinterpret_cast<float*>(&transformSprite.translate.x), 1.0f);
		// ImGuiでSpriteの回転を変える
		ImGui::DragFloat3("RotateSprite", reinterpret_cast<float*>(&transformSprite.rotate.x), 0.01f);
		// ImGuiでSpriteのスケールを変える
		ImGui::DragFloat3("ScaleSprite", reinterpret_cast<float*>(&transformSprite.scale.x), 0.01f);
		// ImGuiでUVTransformの位置を変える
		ImGui::DragFloat2("TranslateUV", reinterpret_cast<float*>(&uvTransformSprite.translate.x), 0.01f, -10.0f, 10.0f);
		// ImGuiでUVTransformの回転を変える
		ImGui::SliderAngle("RotateUV", reinterpret_cast<float*>(&uvTransformSprite.rotate.z));
		// ImGuiでUVTransformのスケールを変える
		ImGui::DragFloat2("ScaleUV", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		// ImGuiのウィンドウを閉じる
		ImGui::End();

		// レンダリングパイプライン
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
			0.45f, static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight), 0.1f, 100.0f);
		Matrix4x4 wvpMatrix = Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);
		wvpData->WVP = wvpMatrix;
		wvpData->World = wvpMatrix;

		// スプライト用のレンダリングパイプライン
		Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		Matrix4x4 viewmatrixSprite = makeIdentity4x4();
		Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
		Matrix4x4 worldViewProjectionMatrixSprite = Multiply(Multiply(worldMatrixSprite, viewmatrixSprite), projectionMatrixSprite);
		transeformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;

		// UVTransformの更新
		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		uvTransformMatrix = Multiply(MakeRotZMatrix(uvTransformSprite.rotate.z), uvTransformMatrix);
		uvTransformMatrix = Multiply(MakeTransMatrix(uvTransformSprite.translate), uvTransformMatrix);
		materialDataSprite->uvTranseform = uvTransformMatrix;

		ImGui::Render();
		// ↑ ゲーム処理はここまで

		// 書き込むアドレスを取得する
		wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// 描画前処理
		dxCommon->PreDraw();

		// RootSignatureを設定する
		dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

		// PSOを設定する
		dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
		dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

		// 形状の設定
		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 三角形のCBufferを設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

		// 平行光源用のCBufferを設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

		// SRVのDescriptorTableの設定
		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

		// 球体用のIBVを設定
		dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

		// 三角形を描画する
		//commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
		dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		// IBVを設定
		dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);

		// SpriteのマテリアルCBufferの場所を設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

		// TransformationMatrixCBufferの場所を設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

		// Spriteの表示する画像を設定
		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

		// Spriteの描画
		dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

		// 描画する
		dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

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

	delete input;
	delete winApp;
	delete dxCommon;

	return 0;
}