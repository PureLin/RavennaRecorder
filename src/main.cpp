#include "pch.h"
#include "control/websocket_server.h"
#include "control/httpserver.h"
#include "Log.h"
#include "stream/StreamInfoListener.h"

void setup() {
    open_log_file();
    ConfigData::getInstance()->read_config();
}

void wait_for_ip_address() {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    bool found = false;
    while (!found) {
        if (getifaddrs(&ifaddr) == -1) {
            logging("getifaddrs() failed");
            continue;
        }
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (NULL == ifa->ifa_addr) {
                continue;
            }
            if (ifa->ifa_addr->sa_family == AF_INET) {
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), zeroconf_ip_address, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    logging("getnameinfo() failed: %s", gai_strerror(s));
                    continue;
                }
                logging("Interface: %s\tAddress: %s", ifa->ifa_name, zeroconf_ip_address);
                //check if ip address is starting with 169.254
                if (strncmp(zeroconf_ip_address, "169.254", 7) == 0) {
                    found = true;
                    break;
                }
            }
        }

        freeifaddrs(ifaddr);
        sleep(1);
    }
    logging("IP address: %s", zeroconf_ip_address);
}


int main() {
    setup();
    wait_for_ip_address();
    websocket_server server_instance;
    server_instance.run();
    httpserver::getInstance()->start(std::string(zeroconf_ip_address));
    sdp_listener_main();
}
