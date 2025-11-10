#pragma once
#include <windows.h>

#include <cstdint>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

class WinApp {
public: // 静的メンバ関数

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public: // メンバ関数
	void Initialize();
	void Update();

	// ウィンドウハンドルを取得
	HWND GetHwnd() { return hwnd; }

	// ウィンドウクラスの取得
	HINSTANCE GetHinstance() const { return wc.hInstance; }

private:
	// ウィンドウハンドル
	HWND hwnd = nullptr;

	// ウィンドウクラスの登録
	WNDCLASS wc{};
};

