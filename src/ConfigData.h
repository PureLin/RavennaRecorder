//
// Created by ME on 2023/12/11.
//

#ifndef RAVENNAPI_CONFIGDATA_H
#define RAVENNAPI_CONFIGDATA_H

#include "pch.h"

using std::vector;
using std::string;
using std::map;

struct stream_info {
    string streamName;
    string sourceIp;
    int sourcePort{};
    int sampleRate{};
    int bitDepth{};
    int channelCount{};
    int onePacketFrameLength{};
    int totalFrameLength{};
    long lastUpdateTimestamp{};

};

static const int MAX_RECORD_CHANNELS = 256;

class ConfigData {
public:
    //only used in runtime
    map<string, stream_info> streamInfoMap;
    vector<string> availablePaths;
    string currentRecordPath = "";
    bool currentRecordPathAvailable = false;
    //need to be saved to Config.json
    bool startRecordImmediately;
    bool enableSaveToHomeDir;
    int fileWriteIntervalInMs;
    int httpServerPort;
    int splitTimeInMinutes;
    string configPassword = "0000";
    int logLevel = 2;

    static ConfigData *getInstance();

    void read_config();

    void save_config();

    ConfigData(const ConfigData &) = delete;

    ConfigData &operator=(const ConfigData &) = delete;

private:
    static ConfigData instance;

    ConfigData() = default;
};


#endif //RAVENNAPI_CONFIGDATA_H
