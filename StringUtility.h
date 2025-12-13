#pragma once
#include <string>

namespace StringUtility {
	/// ログを変換する関数
	// stringをwstringに変換
	std::wstring ConvertString(const std::string& str);

	// wstringをstringに変換
	std::string ConvertString(const std::wstring& str);
}