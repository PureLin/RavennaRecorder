//
// Created by ME on 2023/12/11.
//
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include "common.h"

FILE *log_file;

void logging(const char *message, ...) {
    //print datetime first
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_string[80];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
    if (log_file == NULL) {
        printf("%s ", time_string);
        va_list args;
        va_start(args, message);
        vprintf(message, args);
        va_end(args);
        printf("\n");
    } else {
        fprintf(log_file, "%s ", time_string);
        va_list args;
        va_start(args, message);
        vfprintf(log_file, message, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

void open_log_file() {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_string[80];
    strftime(time_string, sizeof(time_string), "%Y_%m_%d_%H_%M_%S", local_time);
    std::string logPath = getLogDirectory();
    if (!directoryExists(logPath)) {
        mkdir((logPath).c_str(), 0777);
    }
    log_file = fopen((logPath + "/Recorder" + time_string + ".log").c_str(), "a");
    if (log_file == nullptr) {
        printf("Can't open log file, will output to stdout\n");
    } else {
        time_t now = time(nullptr);
        struct tm *local_time = localtime(&now);
        char time_string[80];
        strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
        logging("Start RavennaRecorder");
    }
}
