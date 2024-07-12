//
// Created by ME on 2024/1/10.
//
#include "StreamInfoListener.h"
#include "../common.h"

#define MAX_SDP_PACKET_SIZE 8192

const char *sdp_broadcast_group_address = "239.255.255.255";
const int group_port = 9875;
int sdp_socket;
char zeroconf_ip_address[NI_MAXHOST];
ssize_t ret;

map<string, StreamRecorder *> streamRecorderMap;

void setup_sdp_socket() {
    sdp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sdp_socket < 0) {
        logging("Error creating socket");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(sdp_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        logging("Error setting socket options");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in local_address{};
    memset(&local_address, 0, sizeof(local_address));
    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port = htons(group_port);
    if (bind(sdp_socket, (struct sockaddr *) &local_address, sizeof(local_address)) < 0) {
        logging("Error binding socket");
        exit(EXIT_FAILURE);
    }
    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(sdp_broadcast_group_address);
    mreq.imr_interface.s_addr = inet_addr(zeroconf_ip_address);
    if (setsockopt(sdp_socket, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_interface.s_addr,
                   sizeof(mreq.imr_interface.s_addr)) < 0) {
        logging("Error setting socket options");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sdp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        logging("Error setting socket options");
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(sdp_socket, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(sdp_socket, F_SETFL, flags);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(sdp_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        logging("Error setting socket options");
        exit(EXIT_FAILURE);
    }
}

[[noreturn]] void checkDisconnectedStreams() {
    while (true) {
        sleep(2);
        ConfigData::getInstance()->availablePaths = getAvailablePath();
    }
}

int sdp_listener_main() {
    setup_sdp_socket();
    if (sdp_socket < 0) {
        logging("Error creating socket");
        exit(EXIT_FAILURE);
    }
    char buffer[MAX_SDP_PACKET_SIZE];
    long long n;
    std::thread checkDisconnectedStreamsThread(checkDisconnectedStreams);
    while (true) {
        memset(buffer, 0, MAX_SDP_PACKET_SIZE);
        n = recvfrom(sdp_socket, buffer, MAX_SDP_PACKET_SIZE, 0, nullptr, nullptr);
        if (n <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            logging("Can't receive data from sdp socket");
            close(sdp_socket);
            setup_sdp_socket();
            continue;
        }
        bool is_sdp_removed = buffer[0] == 0x24;
        for (int i = 0; i <= n; ++i) {
            if (buffer[i] == '\0') {
                buffer[i] = ' ';
            }
        }
        stream_info streamInfo = SDPParser::parse(buffer);
        if (streamInfo.streamName.empty()) {
            //Some time buffer will be random empty string, so ignore it when print log
            if (n > 10) {
                logging("Can't parse SDP info %d, %s", (int) n, buffer);
            }
            continue;
        }
        if (streamInfo.channelCount > MAX_RECORD_CHANNELS) {
            logging("%s: Channel count is too high %d", streamInfo.streamName.c_str(), streamInfo.channelCount);
            continue;
        }
        if (is_sdp_removed) {
            logging("SDP removed %s", streamInfo.streamName.c_str());
            if (ConfigData::getInstance()->streamInfoMap.find(streamInfo.streamName) != ConfigData::getInstance()->streamInfoMap.end()) {
                ConfigData::getInstance()->streamInfoMap.erase(streamInfo.streamName);
            }
            if (streamRecorderMap.find(streamInfo.streamName) != streamRecorderMap.end()) {
                streamRecorderMap[streamInfo.streamName]->stop();
                delete streamRecorderMap[streamInfo.streamName];
                streamRecorderMap.erase(streamInfo.streamName);
            }
            continue;
        }
        ConfigData::getInstance()->streamInfoMap[streamInfo.streamName] = streamInfo;
        if (streamRecorderMap.find(streamInfo.streamName) == streamRecorderMap.end()) {
            logging("New SDP received %s", streamInfo.streamName.c_str());
            streamRecorderMap[streamInfo.streamName] = new StreamRecorder(streamInfo);
            if (ConfigData::getInstance()->startRecordImmediately) {
                streamRecorderMap[streamInfo.streamName]->start();
            }
        } else if (streamRecorderMap[streamInfo.streamName]->getStreamInfo().sourceIp != streamInfo.sourceIp ||
                   streamRecorderMap[streamInfo.streamName]->getStreamInfo().sourcePort != streamInfo.sourcePort ||
                   streamRecorderMap[streamInfo.streamName]->getStreamInfo().bitDepth != streamInfo.bitDepth ||
                   streamRecorderMap[streamInfo.streamName]->getStreamInfo().sampleRate != streamInfo.sampleRate ||
                   streamRecorderMap[streamInfo.streamName]->getStreamInfo().channelCount != streamInfo.channelCount ||
                   streamRecorderMap[streamInfo.streamName]->getStreamInfo().onePacketFrameLength != streamInfo.onePacketFrameLength) {
            logging("SDP changed %s", streamInfo.streamName.c_str());
            streamRecorderMap[streamInfo.streamName]->updateStreamInfo(streamInfo);
        }
        for (auto &stream: ConfigData::getInstance()->streamInfoMap) {
            if (stream.second.lastUpdateTimestamp < time(nullptr) - 180) {
                if (streamRecorderMap.find(stream.first) != streamRecorderMap.end()) {
                    logging("Stream %s disconnected", stream.first.c_str());
                    streamRecorderMap[stream.first]->stop();
                    delete streamRecorderMap[stream.first];
                    streamRecorderMap.erase(stream.first);
                }
            }
        }
    }
    return 0;
}