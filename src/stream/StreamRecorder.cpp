//
// Created by ME on 2024/5/6.
//

#include "StreamRecorder.h"
#include "../Log.h"
#include "../common.h"

#define MAX_SDP_PACKET_SIZE 4096
#define RTP_HEADER_SIZE 12
#define TEN_MB 2621440

extern char zeroconf_ip_address[NI_MAXHOST];

StreamRecorder::StreamRecorder(const stream_info &streamInfo) : audio_receive_socket(-1) {
    currentStreamInfo = streamInfo;
    calculateRecordConfigs();
    selectedChannelCount = currentStreamInfo.channelCount;
    for (bool &i: channelSelected) {
        i = true;
    }
}

void StreamRecorder::updateStreamInfo(const stream_info &streamInfo) {
    bool needRestart = isRecording;
    if (isRecording) {
        stop();
    }
    if (recordThread.joinable()) {
        recordThread.join();
    }
    currentStreamInfo = streamInfo;
    calculateRecordConfigs();
    if (needRestart) {
        start();
    }
}


void StreamRecorder::calculateRecordConfigs() {
    int samples_per_packet = currentStreamInfo.onePacketFrameLength * currentStreamInfo.channelCount;
    packet_size = samples_per_packet * currentStreamInfo.bitDepth / 8 + RTP_HEADER_SIZE;
    recordInfo.channels = currentStreamInfo.channelCount;
    recordInfo.samplerate = currentStreamInfo.sampleRate;
    if (currentStreamInfo.bitDepth == 16) {
        recordInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    } else if (currentStreamInfo.bitDepth == 24) {
        recordInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
    } else if (currentStreamInfo.bitDepth == 32) {
        recordInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
    } else {
        inErrorState = STREAM_FORMAT_ERROR;
        logging("%s Unsupported bit depth %d", currentStreamInfo.streamName.c_str(), currentStreamInfo.bitDepth);
    }
    logging("%s packet size %d", currentStreamInfo.streamName.c_str(), packet_size);
    fileBuffer = new int32_t[TEN_MB];
}

void StreamRecorder::start() {
    if (isRecording) {
        return;
    }
    if (recordThread.joinable()) {
        recordThread.join();
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }
    isRecording = true;
    inErrorState = NO_ERROR;
    writingIsSlow = false;
    networkFrameSkipping = false;
    recordThread = thread(&StreamRecorder::doRecord, this);
    writeThread = thread(&StreamRecorder::doWrite, this);
}

void StreamRecorder::stop() {
    if (!isRecording) {
        return;
    }
    logging("%s: Stopping recording", currentStreamInfo.streamName.c_str());
    isRecording = false;
    if (recordThread.joinable()) {
        recordThread.join();
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }
    if (audio_receive_socket != -1) {
        close(audio_receive_socket);
        audio_receive_socket = -1;
    }
    closeRecordFile();
    logging("%s: Recording stopped", currentStreamInfo.streamName.c_str());
}

inline int convert_data(const unsigned char *webBuffer, int32_t *fileBuffer, int samples_per_packet, int channelCount, int bit_depth, const bool channelSelected[MAX_RECORD_CHANNELS]) {
    memset(fileBuffer, 0, samples_per_packet * sizeof(int));
    int bufferIndex = 0;
    if (bit_depth == 16) {
        for (int i = 0; i != samples_per_packet; i++) {
            if (channelSelected[i % channelCount]) {
                fileBuffer[bufferIndex++] = (webBuffer[i * 2] << 24) | (webBuffer[i * 2 + 1] << 16);
            }
        }
    } else if (bit_depth == 24) {
        for (int i = 0; i != samples_per_packet; i++) {
            if (channelSelected[i % channelCount]) {
                fileBuffer[bufferIndex++] = (webBuffer[i * 3] << 24) | (webBuffer[i * 3 + 1] << 16) | (webBuffer[i * 3 + 2] << 8);
            }
        }
    } else if (bit_depth == 32) {
        for (int i = 0; i != samples_per_packet; i++) {
            if (channelSelected[i % channelCount]) {
                fileBuffer[bufferIndex++] = (webBuffer[i * 4] << 24) | (webBuffer[i * 4 + 1] << 16) | (webBuffer[i * 4 + 2] << 8) | webBuffer[i * 4 + 3];
            }
        }
    }
    return bufferIndex;
}

void StreamRecorder::doRecord() {
    unsigned char webBuffer[MAX_SDP_PACKET_SIZE];
    int32_t netBuffer[4096];
    ssize_t n;
    uint32_t seq = UINT32_MAX;
    int samples_per_packet = currentStreamInfo.onePacketFrameLength * currentStreamInfo.channelCount;
    while (isRecording && inErrorState == NO_ERROR) {
        if (audio_receive_socket == -1) {
            setup_stream_socket();
            continue;
        }
        n = recvfrom(audio_receive_socket, webBuffer, MAX_SDP_PACKET_SIZE, 0, nullptr, nullptr);
        if (n <= 0) {
            inErrorState = SOCKET_ERROR;
            logging("%s: Can't receive data from audio socket %s", currentStreamInfo.streamName.c_str(), strerror(errno));
            isRecording = false;
            break;
        }
        if (n != packet_size) {
            logging("%s Input data size changed %d, should be %d, indicate stream config may changed", currentStreamInfo.streamName.c_str(), n, packet_size);
            inErrorState = STREAM_CHANGE_ERROR;
            isRecording = false;
            break;
        }
        uint32_t currentSeq = (webBuffer[2] << 8) | webBuffer[3];
        if (uint16_t(seq + 1) != currentSeq && seq != UINT32_MAX) {
            logging("%s: Warning Packet sequence changed, should be %d, but got %d, audio may dropout", currentStreamInfo.streamName.c_str(), seq, currentSeq);
            networkFrameSkipping = true;
        }
        seq = currentSeq;
        int size = convert_data(webBuffer + RTP_HEADER_SIZE, netBuffer, samples_per_packet, currentStreamInfo.channelCount, currentStreamInfo.bitDepth, channelSelected);
        int last_max = 0;
        for (int i = 0; i != size; ++i) {
            if (abs(netBuffer[i]) > last_max) {
                last_max_value = abs(netBuffer[i]);
            }
        }
        audioQueue.enqueue_bulk(netBuffer, size);
    }
    if (audio_receive_socket != -1) {
        close(audio_receive_socket);
        audio_receive_socket = -1;
    }
}


void StreamRecorder::doWrite() {
    int remainSample = 0;
    int writeFrameCount = 0;
    int actualReadSampleCount;
    std::chrono::time_point<std::chrono::system_clock> warningDismissTime = std::chrono::system_clock::now();
    int queueThreshold = currentStreamInfo.sampleRate * ConfigData::getInstance()->fileWriteIntervalInMs * 4 / 1000;
    logging("%s: queueThreshold %d", currentStreamInfo.streamName.c_str(), queueThreshold);
    while (isRecording && inErrorState == NO_ERROR) {
        if (recordFile == nullptr) {
            if (!openNewFile()) {
                logging("%s: Can't open file for recording", currentStreamInfo.streamName.c_str());
                isRecording = false;
            }
            lastWriteDurationInMs = 0;
        }
        actualReadSampleCount = (int) audioQueue.wait_dequeue_bulk_timed(fileBuffer + remainSample, TEN_MB - remainSample, std::chrono::milliseconds(ConfigData::getInstance()->fileWriteIntervalInMs));
        if (audioQueue.size_approx() > queueThreshold) {
            logging("%s: Audio queue is too much %d", currentStreamInfo.streamName.c_str(), audioQueue.size_approx());
            audioBufferTooMuch = true;
        }
        int writeFrame = (remainSample + actualReadSampleCount) / selectedChannelCount;
        remainSample = (remainSample + actualReadSampleCount) % selectedChannelCount;
        if (writeFrame != 0) {
            std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
            sf_writef_int(recordFile, fileBuffer, writeFrame);
            std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (elapsed > ConfigData::getInstance()->fileWriteIntervalInMs) {
                lastWriteDurationInMs = elapsed;
                writingIsSlow = true;
                warningDismissTime = end + std::chrono::seconds(30);
            } else if (warningDismissTime < end) {
                writingIsSlow = false;
            }
        }
        if (writeFrame != 0 && remainSample > 0) {
            memcpy(fileBuffer, fileBuffer + writeFrame * selectedChannelCount, remainSample * sizeof(int32_t));
        }
        writeFrameCount += writeFrame;
        if (writeFrameCount >= file_write_batch) {
            logging("%s: Record time limit reached, will open new file.", currentStreamInfo.streamName.c_str());
            closeRecordFile();
            writeFrameCount = 0;
        }
        if (needSlice) {
            logging("%s: Slicing file", currentStreamInfo.streamName.c_str());
            closeRecordFile();
            needSlice = false;
        }
    }
    closeRecordFile();
}


void StreamRecorder::setup_stream_socket() {
    if (audio_receive_socket != -1) {
        close(audio_receive_socket);
    }
    // create a UDP socket
    audio_receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (audio_receive_socket < 0) {
        logging("Error creating socket");
        isRecording = false;
        return;
    }
    int reuse = 1;
    if (setsockopt(audio_receive_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        logging("Error setting socket options");
        isRecording = false;
        return;
    }
    struct sockaddr_in local_address{};
    memset(&local_address, 0, sizeof(local_address));
    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = inet_addr(currentStreamInfo.sourceIp.c_str());
    local_address.sin_port = htons(currentStreamInfo.sourcePort);
    if (bind(audio_receive_socket, (struct sockaddr *) &local_address, sizeof(local_address)) < 0) {
        logging("Error binding socket");
        isRecording = false;
        return;
    }
    struct ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr(currentStreamInfo.sourceIp.c_str());
    mreq.imr_interface.s_addr = inet_addr(zeroconf_ip_address);
    if (setsockopt(audio_receive_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        logging("Error setting socket options");
        isRecording = false;
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    if (setsockopt(audio_receive_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        logging("Error setting socket options");
        isRecording = false;
        return;
    }
}

const stream_info &StreamRecorder::getStreamInfo() const {
    return currentStreamInfo;
}

string StreamRecorder::getState() const {
    if (inErrorState != NO_ERROR) {
        return "Error";
    } else if (isRecording) {
        return "Recording";
    } else {
        return "Stopped";
    }
}

bool StreamRecorder::openNewFile() {
    string streamNameSanitized = sanitizeFilePath(currentStreamInfo.streamName);
    recordPath = ConfigData::getInstance()->currentRecordPath + "/" + streamNameSanitized;
    file_write_batch = ConfigData::getInstance()->splitTimeInMinutes * 60 * currentStreamInfo.sampleRate;
    if (ConfigData::getInstance()->currentRecordPath.empty()) {
        logging("Error: Record path is empty");
        inErrorState = PATH_ERROR;
        return false;
    }
    if (mkdir(recordPath.c_str(), 0777) == -1) {
        if (errno != EEXIST) {
            logging("%s mkdir %s failed: %s", streamNameSanitized.c_str(), recordPath.c_str(), strerror(errno));
            inErrorState = PATH_ERROR;
            return false;
        }
    }
    selectedChannelCount = 0;
    for (int i = 0; i < currentStreamInfo.channelCount; i++) {
        if (channelSelected[i]) {
            selectedChannelCount++;
        }
    }
    recordInfo.channels = selectedChannelCount;
    currentFileName = getCurrentTime() + ".wav";
    currentFileFullName = recordPath + "/" + getCurrentTime() + ".wav";
    recordFile = sf_open(currentFileFullName.c_str(), SFM_WRITE, &recordInfo);
    if (recordFile == nullptr) {
        logging("%s: Error opening file: %s", currentFileFullName.c_str(), sf_strerror(recordFile));
        inErrorState = FILE_ERROR;
        return false;
    } else {
        logging("%s: File opened: %s", streamNameSanitized.c_str(), currentFileFullName.c_str());
        return true;
    }
}

StreamRecorder::~StreamRecorder() {
    stop();
    if (recordThread.joinable()) {
        recordThread.join();
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }
    if (audio_receive_socket != -1) {
        close(audio_receive_socket);
        audio_receive_socket = -1;
    }
    closeRecordFile();
    if (fileBuffer != nullptr) {
        delete[] fileBuffer;
    }
}

void StreamRecorder::slice() {
    needSlice = true;
}

string StreamRecorder::getCurrentFileName() {
    if (currentFileName.empty()) {
        return "";
    }
    int last_max = int(20.0 * std::log10(last_max_value / (double) INT32_MAX));
    auto fileSize = std::filesystem::file_size(currentFileFullName) / 1024 / 1024;
    last_max_value = 0;
    if (last_max < -120) {
        return currentFileName + "@ -inf dBFs #" + std::to_string(fileSize) + "MB";
    } else {
        return currentFileName + "@ " + std::to_string(last_max) + " dBFs #" + std::to_string(fileSize) + "MB";
    }
}

void StreamRecorder::closeRecordFile() {
    if (recordFile != nullptr) {
        sf_close(recordFile);
        recordFile = nullptr;
        currentFileName = "";
    }
    audioBufferTooMuch = false;
    writingIsSlow = false;
}

void StreamRecorder::setChannelSelected(int channel, bool selected) {
    if (channel < 0 || channel >= MAX_RECORD_CHANNELS) {
        return;
    }
    channelSelected[channel] = selected;
}

string StreamRecorder::getChannelSelected() {
    stringbuf result;
    for (int i = 0; i < currentStreamInfo.channelCount; ++i) {
        result.sputn(channelSelected[i] ? "1" : "0", 1);
    }
    return result.str();
}

string StreamRecorder::getErrorMessage() {
    if (ErrorState::NO_ERROR != inErrorState) {
        switch (inErrorState) {
            case STREAM_FORMAT_ERROR:
                return "Error:Unsupported bit depth";
            case SOCKET_ERROR:
                return "Error:Receive data failed, check your network connection";
            case STREAM_CHANGE_ERROR:
                return "Error:Input data format mismatched, does your stream config changed?";
            case PATH_ERROR:
            case FILE_ERROR:
                return "Error:Can't create recording file, check your storage configuration";
            default:
                break;
        }
    }
    if (audioBufferTooMuch) {
        return "Error: Files are written too slow causing data backlog, please stop recording and swap a faster storage device.";
    }
    if (networkFrameSkipping || writingIsSlow) {
        string result = "Warning:";
        if (networkFrameSkipping) {
            result += "Some audio packet dropped, will have pop sound in record file.  ";
        }
        if (writingIsSlow) {
            result += "File writing is slow(" + to_string(lastWriteDurationInMs) + "ms), try to increase 'File Write Interval' or use a faster storage device.";
        }
        return result;
    }
    return std::string();
}
