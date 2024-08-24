//
// Created by ME on 2023/12/10.
//

#include "websocket_server.h"
#include <functional>
#include "../Log.h"
#include "../ConfigData.h"
#include "../stream/StreamRecorder.h"

using json = nlohmann::json;
using namespace std;

int websocket_server::port = 9000;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"

extern map<string, StreamRecorder *> streamRecorderMap;


websocket_server::websocket_server() {
    int retry = 0;
    while (retry < 10) {
        try {
            m_server.init_asio();
            m_server.set_open_handler([this](auto &&PH1) { on_open(forward<decltype(PH1)>(PH1)); });
            m_server.set_close_handler([this](auto &&PH1) { on_close(forward<decltype(PH1)>(PH1)); });
            m_server.set_message_handler([this](auto &&PH1, auto &&PH2) { on_message(forward<decltype(PH1)>(PH1), forward<decltype(PH2)>(PH2)); });
            return;
        } catch (websocketpp::exception const &e) {
            logging("websocket_server init failed: %s, retry count %d", e.what(), retry);
            this_thread::sleep_for(chrono::seconds(1));
            ++retry;
        }
    }
}

#pragma clang diagnostic pop

void websocket_server::on_open(const websocketpp::connection_hdl &hdl) {
    m_connections.insert(hdl);
}

void websocket_server::on_close(const websocketpp::connection_hdl &hdl) {
    m_connections.erase(hdl);
}

void websocket_server::on_message(const websocketpp::connection_hdl &hdl, const server::message_ptr &msg) {
    auto str = msg->get_payload();
    if (json::accept(str) == 0) {
        logging("Received unknown message: %s", str.c_str());
        return;
    }
    json j = json::parse(str);
    if (j["command"] == "refresh_stream_list") {
        do_one_broadcast();
        return;
    }
    if (j["command"] == "update_config") {
        if (j["folder"] != nullptr) {
            ConfigData::getInstance()->currentRecordPath = j["folder"];
        }
        if (j["split_time_minute"] != nullptr) {
            string s = j["split_time_minute"];
            int i = stoi(s);
            if (i > 0 && i < 60) {
                ConfigData::getInstance()->splitTimeInMinutes = i;
            }
        }
        if (j["write_interval_in_ms"] != nullptr) {
            string s = j["write_interval_in_ms"];
            ConfigData::getInstance()->fileWriteIntervalInMs = stoi(s);
        }
        if (j["start_immediately"] != nullptr) {
            ConfigData::getInstance()->startRecordImmediately = j["start_immediately"];
        }
        ConfigData::getInstance()->save_config();
        do_one_broadcast();
    }
    if (j["command"] == "set_stream") {
        string stream = j["stream_name"];
        string command = j["action"];
        if (command == "start") {
            if (streamRecorderMap.find(stream) != streamRecorderMap.end()) {
                streamRecorderMap[stream]->start();
            }
        } else if (command == "stop") {
            if (streamRecorderMap.find(stream) != streamRecorderMap.end()) {
                streamRecorderMap[stream]->stop();
            }
        } else if (command == "slice") {
            if (streamRecorderMap.find(stream) != streamRecorderMap.end()) {
                streamRecorderMap[stream]->slice();
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1));
        do_one_broadcast();
        return;
    }
    if (j["command"] == "set_channel_selected") {
        string stream = j["stream_name"];
        int channel = j["channel"];
        bool selected = j["selected"];
        if (streamRecorderMap.find(stream) != streamRecorderMap.end()) {
            streamRecorderMap[stream]->setChannelSelected(channel, selected);
        }
        this_thread::sleep_for(chrono::milliseconds(1));
        do_one_broadcast();
        return;
    }
    if (j["command"] == "stop_all_stream") {
        for (auto &s: streamRecorderMap) {
            s.second->stop();
        }
        this_thread::sleep_for(chrono::milliseconds(1));
        do_one_broadcast();
        return;
    }
    if (j["command"] == "update_password") {
        string password = j["password"];
        ConfigData::getInstance()->configPassword = password;
        ConfigData::getInstance()->save_config();
        do_one_broadcast();
        return;
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc50-cpp"
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"

void websocket_server::run() {
    int retry = 0;
    port = 8000 + rand() % 10;
    while (retry < 120) {
        try {
            m_server.listen(port);
            m_server.start_accept();
            break;
        } catch (websocketpp::exception const &e) {
            logging("websocket_server %d listen failed: %s, retry count %d", port, e.what(), retry);
            this_thread::sleep_for(chrono::seconds(1));
            retry++;
            port += rand() % 10;
        }
    }
    websocket_server_thread = thread(&server::run, &m_server);
    broadcast_thread = thread(&websocket_server::broadcast, this);
}

#pragma clang diagnostic pop


void websocket_server::broadcast() {
    while (m_server.is_listening()) {
        this_thread::sleep_for(chrono::seconds(3));
        do_one_broadcast();
    }
}

void websocket_server::do_one_broadcast() {
    json j;
    j["type"] = "current_status";
    vector<map<string, string>> streamList;
    for (auto &s: streamRecorderMap) {
        map<string, string> m;
        m["stream_name"] = s.first;
        const stream_info &info = s.second->getStreamInfo();
        m["source_ip"] = info.sourceIp;
        m["sample_rate"] = to_string(info.sampleRate);
        m["bit_depth"] = to_string(info.bitDepth);
        m["channel_count"] = to_string(info.channelCount);
        m["status"] = s.second->getState();
        m["file_name"] = s.second->getCurrentFileName();
        m["channel_selected"] = s.second->getChannelSelected();
        m["error_msg"] = s.second->getErrorMessage();
        streamList.push_back(m);
    }
    j["stream_list"] = streamList;
    j["available_path"] = ConfigData::getInstance()->availablePaths;
    j["folder"] = ConfigData::getInstance()->currentRecordPath;
    j["folder_available"] = ConfigData::getInstance()->currentRecordPathAvailable;
    j["split_time_minute"] = ConfigData::getInstance()->splitTimeInMinutes;
    j["write_interval_in_ms"] = ConfigData::getInstance()->fileWriteIntervalInMs;
    j["start_immediately"] = ConfigData::getInstance()->startRecordImmediately;
    j["time"] = time(nullptr);
    string s = j.dump();
    for (const auto &it: m_connections) {
        m_server.send(it, s, websocketpp::frame::opcode::text);
    }
}

void websocket_server::stop() {
    m_server.stop_listening();
    m_server.stop();
    broadcast_thread.join();
    websocket_server_thread.join();
}

websocket_server::~websocket_server() {
    stop();
}

