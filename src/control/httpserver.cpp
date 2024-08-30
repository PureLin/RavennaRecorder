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
    svr.Options("/port", [](const httplib::Request &request, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Authorization"); // Add 'Authorization' here
    });
    svr.Get("/logs", [](const httplib::Request &request, httplib::Response &res) {
        std::string directory_path = getLogDirectory();
        std::string html_content = "<html><body><ul>";
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto &entry: std::filesystem::directory_iterator(directory_path)) {
            entries.push_back(entry);
        }
        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.path().filename().string() < b.path().filename().string();
        });
        for (const auto &entry: entries) {
            std::string filename = entry.path().filename().string();
            html_content += "<li><a href=\"/file?name=" + filename + "\">" + filename + "</a></li>";
        }
        html_content += "</ul></body></html>";
        res.set_content(html_content, "text/html");
    });
    svr.Get("/file", [](const httplib::Request &request, httplib::Response &res) {
        std::string filename = request.get_param_value("name");
        std::string file_path = getLogDirectory() + "/" + filename;
        std::ifstream file(file_path);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            res.set_content(content, "text/plain");
        } else {
            res.status = 404;
            res.set_content("File not found", "text/plain");
        }
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