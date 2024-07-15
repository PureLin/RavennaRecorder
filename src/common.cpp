//
// Created by ME on 2024/5/5.
//
#include "common.h"
#include "Log.h"
#include "ConfigData.h"

using namespace std;

std::string getHomeDirectory() {
    const char *homeDir = std::getenv("HOME");
    if (homeDir == nullptr) {
        // Environment variable not found
        return "/home/pi";
    } else {
        return {homeDir};
    }
}

bool directoryExists(const std::string &dirPath) {
    return std::filesystem::exists(dirPath);
}

std::string getCurrentTime() {
    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);
    char time_string[80];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d_%H-%M-%S", local_time);
    return time_string;
}

std::vector<std::string> getAvailablePath() {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("lsblk -o NAME,TYPE,MOUNTPOINT", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    char line[256];
    std::vector<std::string> result;
    std::string currentDisk;
    vector<string> currentPaths;
    while (fgets(line, sizeof(line), pipe.get())) {
        std::string device(line);
        device = device.substr(0, device.size() - 1); // remove newline character
        std::istringstream iss(device);
        std::string name, type, mountPoint;
        iss >> name >> type;
        std::getline(iss, mountPoint);
        mountPoint = mountPoint.substr(1);// read the rest of the line
        if (type == "disk") {
            if (!currentDisk.empty()) {
                result.insert(result.end(), currentPaths.begin(), currentPaths.end());
            }
            currentPaths.clear();
            currentDisk = name;
        } else if (type == "part") {
            if (!mountPoint.empty() && mountPoint.find("/media/usb") != std::string::npos) {
                if (!directoryExists(mountPoint + "/RavennaRecords")) {
                    if (mkdir((mountPoint + "/RavennaRecords").c_str(), 0777) == -1) {
                        logging("Failed to create directory %s", mountPoint.c_str());
                        continue;
                    }
                }
                currentPaths.push_back(mountPoint + "/RavennaRecords");
            }
        }
    }
    if (!currentDisk.empty()) {
        result.insert(result.end(), currentPaths.begin(), currentPaths.end());
    }
    if (ConfigData::getInstance()->enableSaveToHomeDir) {
        string homeDir = getHomeDirectory() + "/RavennaRecords";
        if (!directoryExists(homeDir)) {
            if (mkdir((homeDir).c_str(), 0777) == -1) {
                logging("Failed to create home directory");
            }
        }
        result.push_back(homeDir);
    }
    if (ConfigData::getInstance()->currentRecordPath.empty() && !result.empty()) {
        ConfigData::getInstance()->currentRecordPath = result[0];
    }
    return result;
}

std::string sanitizeFilePath(const std::string &input) {
    std::string output = input;
    std::string invalidChars = "\\/:*?\"<>|";
    for (char &c: output) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }
    return output;
}