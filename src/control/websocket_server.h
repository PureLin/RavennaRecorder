//
// Created by ME on 2023/12/10.
//

#ifndef WEBSOCKETTEST_WEBSOCKET_SERVER_H
#define WEBSOCKETTEST_WEBSOCKET_SERVER_H

#include "../pch.h"

typedef websocketpp::server<websocketpp::config::asio> server;

class websocket_server {
public:
    websocket_server();

    ~websocket_server();

    void run();

    void stop();

    static int port;

private:
    void on_open(const websocketpp::connection_hdl& hdl);

    void on_close(const websocketpp::connection_hdl& hdl);

    void on_message(const websocketpp::connection_hdl& hdl, const server::message_ptr& msg);

    void broadcast();

    void do_one_broadcast();

    typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> con_list;

    server m_server;
    con_list m_connections{};
    std::thread broadcast_thread;
    std::thread websocket_server_thread;
};


#endif //WEBSOCKETTEST_WEBSOCKET_SERVER_H
