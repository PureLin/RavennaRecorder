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
    enableSaveToHomeDir = false;
    fileWriteIntervalInMs = 100;
    httpServerPort = 80;
    splitTimeInMinutes = 60;

    std::ifstream t(getHomeDirectory() + "/RecorderConfig.json");
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    if (str.empty()) {
        logging("Config file is empty, use default config");
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
        if (j.contains("configPassword")) {
            configPassword = j["configPassword"];
        }
        if (j.contains("enableSaveToHomeDir")) {
            enableSaveToHomeDir = j["enableSaveToHomeDir"];
        }
        if (j.contains("fileWriteIntervalInMs")) {
            fileWriteIntervalInMs = j["fileWriteIntervalInMs"];
            if (fileWriteIntervalInMs < 1) {
                fileWriteIntervalInMs = 1;
            }
        }
        if (j.contains("httpServerPort")) {
            httpServerPort = j["httpServerPort"];
        }
        if (j.contains("logLevel")) {
            logLevel = j["logLevel"];
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
    if (!currentRecordPath.empty()) {
        j["defaultRecordPath"] = currentRecordPath;
    }
    j["configPassword"] = configPassword;
    j["enableSaveToHomeDir"] = enableSaveToHomeDir;
    j["fileWriteIntervalInMs"] = fileWriteIntervalInMs;
    j["httpServerPort"] = httpServerPort;
    j["logLevel"] = logLevel;
    set_log_level(logLevel);
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
