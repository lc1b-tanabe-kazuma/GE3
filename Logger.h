#pragma once
#include <string>
#include <ostream>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace Logeer {
	void Log(std::ostream& os, const std::string& message);

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
}