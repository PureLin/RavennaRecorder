//
// Created by ME on 2024/5/6.
//

#ifndef RAVENNARECORDER_STREAMRECORDER_H
#define RAVENNARECORDER_STREAMRECORDER_H

#include "../pch.h"
#include "../ConfigData.h"

using namespace std;
using namespace moodycamel;

enum ErrorState {
    NO_ERROR,
    SOCKET_ERROR,
    STREAM_CHANGE_ERROR,
    STREAM_FORMAT_ERROR,
    FILE_ERROR,
    PATH_ERROR,
    STORAGE_DISCONNECTION_ERROR,
};
using namespace moodycamel;

class StreamRecorder {
public:
    StreamRecorder(const stream_info &streamInfo);

    ~StreamRecorder();

    bool openNewFile();

    void start();

    void slice();

    void stop();

    [[nodiscard]] const stream_info &getStreamInfo() const;

    string getState() const;

    string getCurrentFileName();

    void updateStreamInfo(const stream_info &info);

    void setChannelSelected(int channel, bool selected);

    string getChannelSelected();

    string getErrorMessage();

private:
    bool channelSelected[MAX_RECORD_CHANNELS];
    int selectedChannelCount = 0;
    std::atomic<bool> needSlice = false;
    std::atomic<int32_t> last_max_value = 0;
    BlockingConcurrentQueue<int32_t> audioQueue{};

    stream_info currentStreamInfo;
    bool isRecording = false;
    ErrorState inErrorState = NO_ERROR;
    string recordPath;
    thread recordThread;
    thread writeThread;
    int audio_receive_socket;
    int packet_size;
    int file_write_batch;
    string currentFileName;
    string currentFileFullName;
    SF_INFO recordInfo{};
    SNDFILE *recordFile{};
    int32_t *fileBuffer;
    int lastWriteDurationInMs = 0;

    bool networkFrameSkipping = false;
    bool audioBufferTooMuch = false;
    bool writingIsSlow = false;

    void calculateRecordConfigs();

    void setup_stream_socket();

    void doRecord();

    void doWrite();

    void closeRecordFile();
};


#endif //RAVENNARECORDER_STREAMRECORDER_H
