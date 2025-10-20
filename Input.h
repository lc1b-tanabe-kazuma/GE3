#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>

#include <dinput.h>
#include <wrl.h>
#include <cassert>      

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// 入力
class Input
{
public:

	// 初期化
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	// 更新
	void Update();

	bool PusyKey(BYTE keyNumber);

private:
	// キーボードデバイスの生成
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;
};