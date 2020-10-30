#include "common_utils.h"

void Log_e(std::string tag, std::string message)
{
#ifndef WINDOWS
    __android_log_write(ANDROID_LOG_ERROR, tag.c_str(), message.c_str());
#else
	std::cout << "E/" << tag << ": " << message << std::endl;
#endif
}
