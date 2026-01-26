#include"Input.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(WinApp* winApp) {

	// 借りてきたWinAppのインスタンスを記録
	this->winApp = winApp;

	/// DirectInputの初期化 -------------------------------------

	// DirectInputのインスタンス作成
	HRESULT directInputResult = DirectInput8Create(
		winApp->GetHinstance(),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		reinterpret_cast<void**>(directInput.GetAddressOf()),
		nullptr);
	assert(SUCCEEDED(directInputResult));

	// キーボードデバイスの生成
	HRESULT keyboardResult = directInput->CreateDevice(GUID_SysKeyboard,
		keyboard.GetAddressOf(), nullptr);
	assert(SUCCEEDED(keyboardResult));

	// キーボードデバイスのデータフォーマットを設定
	HRESULT dataFormatResult = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(dataFormatResult));

	// 排他制御レベルの設定
	HRESULT cooperativeLevelResult = keyboard->SetCooperativeLevel(
		winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(cooperativeLevelResult));
}

void Input::Update() {

	// 前回のキー状態を保存
	memcpy(preKey, key, sizeof(key));

	// キーボードの情報取得
	keyboard.Get()->Acquire();

	// 全キーの入力状態を取得する
	keyboard.Get()->GetDeviceState(sizeof(key), key);

	// 数字の０が押されたら
	if (TriggerKey(DIK_0)) {
		// 出力ウィンドウにHit0と表示
		OutputDebugStringA("Hit 0\n");
	}
}

bool Input::PushKey(BYTE keyNumber) {
	if (key[keyNumber] & 0x80) {
		return true;
	}
	return false;
}

bool Input::TriggerKey(BYTE keyNumber) {
	bool ret = false;
	// 押された瞬間を検出
	if ((key[keyNumber] & 0x80) && !(preKey[keyNumber])) {
		ret = true;
	}
	// 前回のキー状態を保存
	preKey[keyNumber] = key[keyNumber];
	return ret;
}