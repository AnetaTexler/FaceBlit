#include "common_utils.h"
#include <filesystem>
#include <sstream>


void Log_e(std::string tag, std::string message)
{
#ifndef WINDOWS
    __android_log_write(ANDROID_LOG_ERROR, tag.c_str(), message.c_str());
#else
	std::cout << "E/" << tag << ": " << message << std::endl;
#endif
}

void Log_i(std::string tag, std::string message)
{
#ifndef WINDOWS
	__android_log_write(ANDROID_LOG_INFO, tag.c_str(), message.c_str());
#else
	std::cout << "I/" << tag << ": " << message << std::endl;
#endif
}

bool makeDir(const std::string& path)
{
	if (!std::filesystem::exists(path)) {
		if (std::filesystem::create_directories(path) == false) {
			std::cout << "Creating an output directory \"" << path << "\" failed." << std::endl;
			system("pause");
			return false;
		}
	}
	return true;
}

std::string padLeft(std::string target, int totalWidth, char paddingChar)
{
	std::ostringstream ss;
	ss << std::setw(totalWidth) << std::setfill(paddingChar) << target;
	std::string result = ss.str();
	if (result.length() > totalWidth)
		result.erase(0, result.length() - totalWidth);

	return result;
}