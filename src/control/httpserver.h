//
// Created by ME on 2023/12/25.
//

#ifndef RAVENNAPI_HTTPSERVER_H
#define RAVENNAPI_HTTPSERVER_H

#include "../pch.h"
#include "../httplib.h"


class httpserver {
public:
    static httpserver *getInstance();

    void start(std::string string);

    void stop();

    ~httpserver();

private:
    void run();
    std::string ip_address;
    httplib::Server svr;
    static httpserver instance;

    httpserver() = default;
};


#endif //RAVENNAPI_HTTPSERVER_H
