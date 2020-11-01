#ifndef COMMON_UTILS_1564664221
#define COMMON_UTILS_1564664221

#ifndef WINDOWS
#include <android/log.h>
#else
#include <iostream>
#endif

#include <string>

void Log_e(std::string tag, std::string message);


#endif //COMMON_UTILS_1564664221