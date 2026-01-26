#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>

#include <dinput.h>
#include <wrl.h>
#include <cassert>      

#include "WinApp.h"

// 入力
class Input {
public:

	// 初期化
	void Initialize(WinApp* winApp);

	// 更新
	void Update();

	bool PushKey(BYTE keyNumber);

	// トリガー判定
	bool TriggerKey(BYTE keyNumber);

private:

	// DirectInputのインスタンス
	Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;

	// キーボードデバイスの生成
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;

	BYTE key[256] = {};
	BYTE preKey[256] = {};

	// WinAPI
	WinApp* winApp = nullptr;
};