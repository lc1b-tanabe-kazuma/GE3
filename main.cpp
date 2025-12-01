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

// ログを変換する関数
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

void Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

/*
Microsoft::WRL::ComPtr <IDxcBlob> CompileShader(

	// complierするShaderファイルへのパス
	const std::wstring& filePath,

	// Compilerに使用するprofaile
	const wchar_t* profile,

	// 初期化で生成した物を3つ
	Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils,
	Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler,
	Microsoft::WRL::ComPtr <IDxcIncludeHandler> includehandler,
	std::ostream& os) {

	Log(os, ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));

	// hlslファイルを読む
	Microsoft::WRL::ComPtr <IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	// コンパイルする
	Microsoft::WRL::ComPtr <IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includehandler.Get(),
		IID_PPV_ARGS(&shaderResult)
	);

	// コンパイルエラーの時に止まる
	assert(SUCCEEDED(hr));

	// エラーが出たらログに出して止める
	Microsoft::WRL::ComPtr <IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {

		// エラーがあったら出力する
		Log(os, shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイル結果からバイナリ部分を取得する
	Microsoft::WRL::ComPtr <IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したらログを出す
	Log(os, ConvertString(std::format(L"Compile Success,path:{},profile:{}\n", filePath, profile)));

	// shaderBlobを返す
	return shaderBlob;
}*/

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

// リソースを作成する関数
Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr <ID3D12Device>& device, size_t sizeInBytes) {

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	// UploadHeapを使う
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。てぅすちゃの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	// リソースのサイズ。今回はVector4を3頂点分
	vertexResourceDesc.Width = sizeInBytes;
	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点にリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));
	return vertexResource;
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

// LoadTexture関数
DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring wFilePath = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(wFilePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップを生成する
	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert(SUCCEEDED(hr));

	// ミップマップを返す
	return mipImage;
}

Microsoft::WRL::ComPtr <ID3D12Resource> CreateTextuerResource(const Microsoft::WRL::ComPtr <ID3D12Device>& device, const DirectX::TexMetadata& metdata) {
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metdata.width); // Textureの幅
	resourceDesc.Height = UINT(metdata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metdata.mipLevels); // ミップマップの数
	resourceDesc.DepthOrArraySize = UINT16(metdata.arraySize); // テクスチャの深さまたはアレイサイズ
	resourceDesc.Format = metdata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metdata.dimension);

	// 利用するするHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // コピー先の状態
		nullptr, // 初期化しない
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
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

[[nodiscard]]
Microsoft::WRL::ComPtr <ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr <ID3D12Resource>& texture, const DirectX::ScratchImage& mipImage, const Microsoft::WRL::ComPtr <ID3D12Device>& device,
	const Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& commandList) {

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImage.GetImages(), mipImage.GetImageCount(), mipImage.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	// Textureへの転送後は利用できるよう、
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
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

	// ウィンドウクラスの設定
	WinApp* winApp = nullptr;
	winApp = new WinApp();
	winApp->Initialize();

	// DirectX12初期化処理
	DirectXCommon* dxCommon = nullptr;
	dxCommon = new DirectXCommon();
	dxCommon->Initialize();

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif // _DEBUG

	// DXGIファクトリーの作成
	Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory = nullptr;

	// 関数が成功したか判定する
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	assert(SUCCEEDED(hr));

	//
	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter = nullptr;

	//
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {

		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		//
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

			Log(logStream, ConvertString(std::format(L"USE Adaptere:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}

	assert(useAdapter != nullptr);

	Microsoft::WRL::ComPtr< ID3D12Device> device = nullptr;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelString[] = { "12.2","12.1","12.0" };

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

		if (SUCCEEDED(hr)) {
			Log(logStream, std::format("feturelevel : {}\n", featureLevelString[i]));
			break;
		}
	}

	assert(device != nullptr);
	Log(logStream, "Compleate create D3D12Device!!\n");

	// logのデバッグを出力
	Log(logStream, "Hello,DirectX!\n");

	// logに画面の幅の数値を出力
	Log(logStream, ConvertString(std::format(L"width : {}, {}\n", WinApp::kClientWidth, WinApp::kClientHeight)));

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;

	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		//やばいエラーの時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		// エラーの時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 警告の時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
	}
#endif // _DEBUG

	// 入力の初期化
	Input* input = nullptr;

	input = new Input();
	input->Initialize(winApp);

	// コマンドキューを生成する
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));

	// コマンドキューの生成がうまく行かなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドアロケーターを生成する
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	//コマンドリストの生成がうまく行かなかったので起動できない
	assert(SUCCEEDED(hr));

	// スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;
	swapChainDesc.Height = WinApp::kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// ディスクリプタヒープの生成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// SRVのディスクリプタヒープを作成する
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));

	// swapChainからResourceを引っ張ってくる
	Microsoft::WRL::ComPtr <ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVを2つ作るのでディスクリプタを2つ用意する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};

	// まずは1つ目を作る、1つ目は最初の所に作る．
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

	// 2つ目のディスクリプタハンドルを得る
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//2つ目を作る
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

	// 初期値0でfenceを作る
	Microsoft::WRL::ComPtr <ID3D12Fence> fence = nullptr;
	uint32_t fencevalue = 0;
	hr = device->CreateFence(fencevalue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// 
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

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

	// シリアライズしてバイナリする
	Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferSize()));
		assert(false);
	}

	// バイナリを元に作成
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(0,
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
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = CompileShader(L"resources/shaders/Object3D.VS.hlsl",
		L"vs_6_0", dxcUtils, dxCompiler, includehandler, logStream);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = CompileShader(L"resources/shaders/Object3D.PS.hlsl",
		L"ps_6_0", dxcUtils, dxCompiler, includehandler, logStream);
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
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
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

	DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);

	// 頂点リソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = CreateBufferResource(device.Get(), sizeof(VertexData) * modelData.vertices.size());

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
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResource = CreateBufferResource(device.Get(), sizeof(uint32_t) * indexCount);

	// 7. インデックスバッファビュー設定
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// マテリアルのデータを作成する
	Material* materialData = nullptr;

	// マテリアルのリソースを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = CreateBufferResource(device.Get(), sizeof(Material));

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
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSprite = CreateBufferResource(device.Get(), sizeof(Material));

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
	Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));

	// 書き込むアドレスを取得する
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	// アドレスに書き込む
	directionalLightData->color = Vector4{ 1.0f,1.0f,1.0f,1.0f }; // 白色
	directionalLightData->direction = { 0.0f,-1.0f,0.0f }; // 向き
	directionalLightData->intensity = 1.0f; // 強さ

	directionalLightResource->Unmap(0, nullptr);

	// WVP用のリソースを作成する
	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = CreateBufferResource(device.Get(), sizeof(TransfomationMatrix));

	// WVPのデータを作成する
	TransfomationMatrix* wvpData = nullptr;

	// 書き込むアドレスを取得する
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	// 単位行列を書き込む
	wvpData->World = makeIdentity4x4();

	// Sprite用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 4);

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
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);

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
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(TransfomationMatrix));

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

	// IMGUIの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(), 2,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	/*
	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 0);
*/
	// Textureの読んで転送する
	DirectX::ScratchImage mipImage = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImage.GetMetadata();
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = CreateTextuerResource(device.Get(), metadata);
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImage, device.Get(), commandList.Get());

	// 2枚目のTextureの読んで転送する
	DirectX::ScratchImage mipImage2 = LoadTexture("resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImage2.GetMetadata();
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = CreateTextuerResource(device.Get(), metadata2);
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImage2, device.Get(), commandList.Get());

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

	// SRVのDiscriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);

	// 先頭はImGuiで使うのでずらす
	textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// SRVを作成する
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);

	// DepthStencilTexture
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStencilTexturResource(
		device.Get(), WinApp::kClientWidth, WinApp::kClientHeight);

	// DSV用のヒープディスクリプタの数は1
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvdescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Textuer切り替え変数
	bool useMonsterBall = false;

	transform.rotate.y = 3.0f;

	// XAudio2の初期化
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
	HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));

	// 音声ファイルの読み込み
	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");

	// 音声再生
	SoundPlayWave(xAudio2.Get(), soundData1);

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

		// バックバッファのインデックスを取得
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		// TransitionBarrierの設定
		D3D12_RESOURCE_BARRIER barrier{};

		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

		// 遷移前のresorceState
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

		// 遷移後のresorceState
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// TransitionBarrrierを張る
		commandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvdescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// 描画先のレンダーターゲットを設定する
		commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

		// 指定する色で画面全体をクリアする
		float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
		commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 描画用のDesxriptorHeapを設定する
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());

		// ビューポートを設定する
		commandList->RSSetViewports(1, &viewport);

		// シザー矩形を設定する
		commandList->RSSetScissorRects(1, &scissorRect);

		// RootSignatureを設定する
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// PSOを設定する
		commandList->SetPipelineState(graphicsPipelineState.Get());
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

		// 形状の設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 三角形のCBufferを設定
		commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

		// 平行光源用のCBufferを設定
		commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

		// SRVのDescriptorTableの設定
		commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

		// 球体用のIBVを設定
		commandList->IASetIndexBuffer(&indexBufferView);

		// 三角形を描画する
		//commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
		commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		// IBVを設定
		commandList->IASetIndexBuffer(&indexBufferViewSprite);

		// SpriteのマテリアルCBufferの場所を設定
		commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

		// TransformationMatrixCBufferの場所を設定
		commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

		// Spriteの表示する画像を設定
		commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

		// Spriteの描画
		commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

		// 描画する
		commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// 実際のcommandListのImGuiの描画を行う
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());


		// 描画前処理
		dxCommon->PreDraw();

		
		// 今回はRenderTargetからpresentにする
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		commandList->ResourceBarrier(1, &barrier);

		// コマンドリストの内容を確定させる
		hr = commandList->Close();
		assert(SUCCEEDED(hr));

		// GPUにコマンドリストの実行を行わせる
		ID3D12CommandList* commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		// GPUとOSに画面の交換を行なうように通知する
		swapChain->Present(1, 0);

		// 描画後処理
		dxCommon->PostDraw();

		// fenceの値を更新
		fencevalue++;

		// GPUがここまでたどり着いたときに、fenceの値を指定した値に代入するようにSignalを送る
		commandQueue->Signal(fence.Get(), fencevalue);

		if (fence->GetCompletedValue() < fencevalue) {
			fence->SetEventOnCompletion(fencevalue, fenceEvent);

			WaitForSingleObject(fenceEvent, INFINITE);
		}

		// 次のフレーム用のコマンドリストを準備
		hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList->Reset(commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}

	// 解放処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// xAudio2のマスターボイスを解放
	xAudio2.Reset();

	// 音声データの解放
	SoundUnload(&soundData1);

	// WindowsAPIの終了処理
	winApp->Finalize();

	delete input;
	delete winApp;
	delete dxCommon;

	return 0;
}