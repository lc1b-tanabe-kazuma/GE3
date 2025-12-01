#include "Logger.h"
#include <debugapi.h>

void Logeer::Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}