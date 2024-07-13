//
// Created by ME on 2023/12/25.
//

#include "httpserver.h"

#include <utility>
#include "../Log.h"
#include "websocket_server.h"
#include "../common.h"
#include "../ConfigData.h"

httpserver httpserver::instance;

httpserver *httpserver::getInstance() {
    return &instance;
}

httpserver::~httpserver() {
    stop();
}

void httpserver::start(std::string ip) {
    ip_address = std::move(ip);
    std::thread t(&httpserver::run, this);
    t.detach();
}

void httpserver::stop() {
    svr.stop();
}

void httpserver::run() {
    svr.Options("/*", [](const httplib::Request &request, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type");
    });
    svr.Get("/", [](const httplib::Request &request, httplib::Response &res) {
        logging("httpserver get request from %s", request.remote_addr.c_str());
        std::ifstream t(getHomeDirectory() + "/ConfigMain.html");
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        res.set_content(str, "text/html");
    });
    svr.Get("/port", [](const httplib::Request &request, httplib::Response &res) {
        logging("httpserver post request from %s", request.remote_addr.c_str());
        nlohmann::json j;
        auto it = request.headers.find("Authorization");
        if (it != request.headers.end()) {
            std::string password = it->second;
            if (password == ConfigData::getInstance()->configPassword) {
                j["port"] = websocket_server::port;
            } else {
                j["port"] = -1;
            }
        } else {
            j["port"] = -1;
        }
        //allow cross-origin
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(j.dump(), "application/javascript");
    });
    logging("httpserver start");
    svr.listen("0.0.0.0", ConfigData::getInstance()->httpServerPort);
}