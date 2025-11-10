#pragma once
#include <windows.h>
#include <cstdint>

class WinApp {
public: // 静的メンバ関数

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public: // メンバ関数
	void Initialize();
	void Update();

	// 終了
	void Finalize();

	// ウィンドウハンドルを取得
	HWND GetHwnd() { return hwnd; }

	// ウィンドウクラスの取得
	HINSTANCE GetHinstance() const { return wc.hInstance; }

	// メッセージの処理
	bool ProcessMessage();

private:
	// ウィンドウハンドル
	HWND hwnd = nullptr;

	// ウィンドウクラスの登録
	WNDCLASS wc{};
};

