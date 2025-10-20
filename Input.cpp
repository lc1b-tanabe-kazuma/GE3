#include"Input.h"

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {

	/// DirectInputの初期化 -------------------------------------

	// DirectInputのインスタンス作成
	Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
	HRESULT directInputResult = DirectInput8Create(
		hInstance,
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
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(cooperativeLevelResult));
}

void Input::Update()
{

	// キーボードの情報取得
	keyboard.Get()->Acquire();

	// 全キーの入力状態を取得する
	BYTE key[256];
	keyboard.Get()->GetDeviceState(sizeof(key), key);

	// 数字の０が押されたら
	if (key[DIK_0]) {
		// 出力ウィンドウにHit0と表示
		OutputDebugStringA("Hit 0\n");
	}
}