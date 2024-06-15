//
// Created by ME on 2023/12/12.
//
#include "ConfigData.h"
#include "Log.h"
#include "common.h"

using json = nlohmann::json;

ConfigData ConfigData::instance;

ConfigData *ConfigData::getInstance() {
    return &instance;
}

void ConfigData::read_config() {
    availablePaths = getAvailablePath();
    startRecordImmediately = false;
    splitTimeInMinutes = 60;
    defaultRecordPath = currentRecordPath = ConfigData::getInstance()->availablePaths[0];

    std::ifstream t(getHomeDirectory() + "/RecorderConfig.json");
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    if (str.empty()) {
        t.close();
        save_config();
        return;
    }
    try {
        json j = json::parse(str);
        if (j.contains("startRecordImmediately")) {
            startRecordImmediately = j["startRecordImmediately"];
        }
        if (j.contains("splitTimeInMinutes")) {
            splitTimeInMinutes = j["splitTimeInMinutes"];
        }
        if (j.contains("defaultRecordPath")) {
            defaultRecordPath = j["defaultRecordPath"];
        }
        if (!std::filesystem::exists(defaultRecordPath)) {
            logging("defaultRecordPath %s not found in availablePaths, will use first available path %s", defaultRecordPath.c_str(), currentRecordPath.c_str());
        } else {
            currentRecordPath = defaultRecordPath;
            logging("defaultRecordPath %s found, will use it", defaultRecordPath.c_str());
        }
        if (j.contains("configPassword")) {
            configPassword = j["configPassword"];
        }
    } catch (json::parse_error &e) {
        logging("parse error: %s", e.what());
    }
    t.close();
    save_config();
}

void ConfigData::save_config() {
    FILE *fp;
    json j;
    j["startRecordImmediately"] = startRecordImmediately;
    j["splitTimeInMinutes"] = splitTimeInMinutes;
    j["defaultRecordPath"] = defaultRecordPath;
    j["configPassword"] = configPassword;
    std::string s = j.dump(2);
    fp = fopen((getHomeDirectory() + "/RecorderConfig.json").c_str(), "w");
    if (fp == nullptr) {
        perror("Error opening config file for write");
        logging("Can't open RPiConfig.json for write");
    } else {
        fprintf(fp, "%s", s.c_str());
        fclose(fp);
        logging("Config saved");
    }
}
