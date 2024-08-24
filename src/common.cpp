//
// Created by ME on 2024/5/5.
//
#include "common.h"
#include "Log.h"
#include "ConfigData.h"

using namespace std;

static std::thread updateThread;
static vector<string> updateLogs;

std::string getHomeDirectory() {
    const char *homeDir = std::getenv("HOME");
    if (homeDir == nullptr) {
        // Environment variable not found
        return "/home/pi";
    } else {
        return {homeDir};
    }
}

std::string getLogDirectory() {
    return getHomeDirectory() + "/Logs";
}


void updateThreadFunction() {
    logging(LogLevel::WARN, "Checking for update");
    ConfigData::getInstance()->isCheckingForUpdate = true;
    string updateCommand = "curl -sL -m 3 https://raw.githubusercontent.com/PureLin/RavennaRecorder/master/update.sh | sudo -E bash ";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((updateCommand.c_str()), "r"), pclose);
    char line[1024];
    while (fgets(line, sizeof(line), pipe.get())) {
        logging(LogLevel::INFO, "Update logs: %s", line);
        std::string log(line);
        updateLogs.push_back(log);
    }
    logging(LogLevel::WARN, "Update finished");
    ConfigData::getInstance()->isCheckingForUpdate = false;
}

void checkForUpdate() {
    if (ConfigData::getInstance()->isCheckingForUpdate) {
        logging(LogLevel::WARN, "Already checking for update");
        return;
    }
    if (updateThread.joinable()) {
        updateThread.join();
    }
    updateLogs = {};
    updateThread = std::thread(updateThreadFunction);
}

vector<string> getUpdateLogs() {
    return updateLogs;
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

//std::vector<string> getAllMountPoint() {
//    string command = "mount | grep \"/media/\" | awk '{print $3}'";
//    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
//    if (!pipe) {
//        throw std::runtime_error("popen() for mount point failed!");
//    }
//    char line[512];
//    std::vector<string> result;
//    while (fgets(line, sizeof(line), pipe.get())) {
//        std::string mountPoint(line);
//        mountPoint = mountPoint.substr(0, mountPoint.size() - 1); // remove newline character
//        logging(LogLevel::DEBUG, "Found mount point \"%s\"", mountPoint.c_str());
//        result.push_back(mountPoint);
//    }
//    return result;
//}

std::string getDeviceId(string diskName) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(("udevadm info /dev/" + diskName + "| grep -E \"S: disk/by-id/|E: ID_SERIAL_SHORT=\" ").c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() for deviceId failed!");
    }
    char line[512];
    string deviceId;
    string serial;
    while (fgets(line, sizeof(line), pipe.get())) {
        if (strstr(line, "S: disk/by-id/")) {
            deviceId = line;
            deviceId = deviceId.substr(14, deviceId.length() - 15);
        }
        if (strstr(line, "E: ID_SERIAL_SHORT=")) {
            serial = line;
            serial = serial.substr(19, serial.length() - 20);
            serial = "_" + serial;
        }
    }
    if (!serial.empty()) {
        deviceId.replace(deviceId.find(serial), serial.length(), "");
    }
    deviceId = sanitizeFilePath(deviceId);
    logging(LogLevel::DEBUG, "Driver %s has device id %s", diskName.c_str(), deviceId.c_str());
    return deviceId;
}

std::vector<std::string> getAvailablePath() {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("lsblk -o NAME,TYPE,MOUNTPOINT", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() for drive list failed!");
    }
    char line[256];
    std::vector<std::string> currentMountPoints;
    std::vector<std::string> result;
    std::string currentDisk;
    vector<string> currentPaths;
    int partId = 0;
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
            currentDisk = getDeviceId(name);
            partId = 0;
            if (currentDisk.empty()) {
                continue;
            }
        } else if (type == "part") {
            ++partId;
            if (mountPoint.empty()) {
                if (currentDisk.empty()) {
                    continue;
                }
                string partName;
                for (int i = 0; i != name.length(); ++i) {
                    if (isalnum(name[i])) {
                        partName = name.substr(i);
                        break;
                    }
                }
                logging(LogLevel::WARN, "Found a usb drive partition %s", partName.c_str());
                string path = "/media/" + currentDisk + "_part" + to_string(partId);
                if (!directoryExists(path)) {
                    if (mkdir(path.c_str(), 0777) == -1) {
                        logging(LogLevel::ERROR, "Failed to create directory %s", path.c_str());
                        continue;
                    }
                }
                std::string mountCommand = "mount /dev/" + partName + " " + path;
                logging(LogLevel::WARN, "Mounting a usb drive %s", mountCommand.c_str());
                int mountResult = system(mountCommand.c_str());
                if (mountResult != 0) {
                    logging("Failed to mount %s", partName.c_str());
                    continue;
                }
                mountPoint = path;
            }
            currentMountPoints.push_back(mountPoint);
            if (!mountPoint.empty() && mountPoint.find("/media/") != std::string::npos) {
                if (!directoryExists(mountPoint + "/RavennaRecords")) {
                    if (mkdir((mountPoint + "/RavennaRecords").c_str(), 0777) == -1) {
                        logging(LogLevel::ERROR, "Failed to create directory %s", mountPoint.c_str());
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
                logging(LogLevel::ERROR, "Failed to create home directory");
            }
        }
        result.push_back(homeDir);
    }
    if (ConfigData::getInstance()->currentRecordPath.empty() && !result.empty()) {
        ConfigData::getInstance()->currentRecordPath = result[0];
    } else {
        bool found = false;
        for (const auto &path: result) {
            if (path == ConfigData::getInstance()->currentRecordPath) {
                found = true;
                break;
            }
        }
        ConfigData::getInstance()->currentRecordPathAvailable = found;
    }
    return result;
}

std::string sanitizeFilePath(const std::string &input) {
    std::string output = input;
    std::string invalidChars = "\\/:*?\"<>| ";
    for (char &c: output) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }
    return output;
}