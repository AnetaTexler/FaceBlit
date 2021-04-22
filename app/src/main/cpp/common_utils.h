#ifndef COMMON_UTILS_1564664221
#define COMMON_UTILS_1564664221

#ifndef WINDOWS
#include <android/log.h>
#else
#include <iostream>
#endif

#include <string>

void Log_e(std::string tag, std::string message);
void Log_i(std::string tag, std::string message);
bool makeDir(const std::string& path);
std::string padLeft(std::string target, int totalWidth, char paddingChar);


#endif //COMMON_UTILS_1564664221
