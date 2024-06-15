//
// Created by ME on 2023/12/12.
//

#include "SDPParser.h"
#include "../Log.h"
#include <ctime>

using std::string;
using std::map;

void resolveOneLine(string &line, map<string, string> &fields) {
    try {
        if (line.size() > 2 && line[1] != '=') {
            return;
        }
        char c = line.substr(0, 1)[0];
        if (c != 'a') {
            fields[line.substr(0, 1)] = line.substr(2);
            return;
        }
        line = line.substr(2);
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            fields[line.substr(0, colonPos)] = line.substr(colonPos + 1);
        } else {
            fields[line.substr(0, colonPos)] = "";
        }
    } catch (std::exception &e) {
    }
}

stream_info SDPParser::parse(const string &sdpData) {
    size_t pos = 0;
    string sdpDataCopy = sdpData; // Create a copy of sdpData to avoid modifying the original string
    sdpDataCopy.erase(0, sdpDataCopy.find("application/sdp ") + 16);
    map<string, string> fields;
    while ((pos = sdpDataCopy.find_first_of("\r\n")) != string::npos) {
        string line = sdpDataCopy.substr(0, pos);
        if (!line.empty()) {
            resolveOneLine(line, fields);
        }
        sdpDataCopy.erase(0, pos + 1);
        if (sdpDataCopy[0] == '\n') { // Handle the case where the newline character is "\r\n"
            sdpDataCopy.erase(0, 1);
        }
    }
    if (sdpDataCopy.length() > 0) {
        resolveOneLine(sdpDataCopy, fields);
    }
    stream_info info;
    // Parse specific fields
    if (fields.count("s")) {
        info.streamName = fields["s"];
    }
    if (fields.count("c")) {
        info.sourceIp = fields["c"].substr(fields["c"].find_last_of(' ') + 1, fields["c"].find_last_of('/') - fields["c"].find_last_of(' ') - 1);
    }
    if (fields.count("m")) {
        info.sourcePort = atoi(fields["m"].substr(fields["m"].find_first_of(' ')).c_str());
    }
    if (fields.count("rtpmap")) {
        string audioFormat = fields["rtpmap"].substr(fields["rtpmap"].find_last_of('L') + 1);
        info.bitDepth = std::stoi(audioFormat.substr(0, audioFormat.find_first_of(('/'))));
        info.sampleRate = std::stoi(audioFormat.substr(audioFormat.find_first_of('/') + 1, audioFormat.find_last_of('/')));
        info.channelCount = std::stoi(audioFormat.substr(audioFormat.find_last_of('/') + 1));
    }
    if (fields.count("framecount")) {
        //when framecount have a '-' in it, it means one frame will split into multiple packets
        if (fields["framecount"].find('-') != -1) {
            info.onePacketFrameLength = std::stoi(fields["framecount"].substr(0, fields["framecount"].find_first_of('-')));
            info.totalFrameLength = std::stoi(fields["framecount"].substr(fields["framecount"].find_first_of('-') + 1));
        } else {
            info.onePacketFrameLength = std::stoi(fields["framecount"]);
        }
    } else if (fields.count("ptime")) {
        info.onePacketFrameLength = int(round(std::stod(fields["ptime"]) * info.sampleRate / 1000));
        info.totalFrameLength = info.onePacketFrameLength;
    }
    info.lastUpdateTimestamp = std::time(nullptr);
    return info;
}
