#include "Logger.h"
#include <iostream>
#include <Windows.h>

namespace Logeer {
    void Log(const std::string& message) {
        // デバッグ出力
        OutputDebugStringA(message.c_str());
        OutputDebugStringA("er\n");
    }
}