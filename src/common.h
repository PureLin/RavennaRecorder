//
// Created by ME on 2024/5/5.
//

#ifndef RAVENNARECORDER_COMMON_H
#define RAVENNARECORDER_COMMON_H

#include "pch.h"

std::string getHomeDirectory();

bool directoryExists(const std::string &dirPath);

std::string getCurrentTime();

std::vector<std::string> getAvailablePath();

std::string sanitizeFilePath(const std::string &input);

#endif //RAVENNARECORDER_COMMON_H
