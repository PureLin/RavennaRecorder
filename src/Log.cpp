//
// Created by ME on 2023/12/11.
//
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include "common.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "Log.h"

FILE *log_file;
std::shared_ptr<spdlog::logger> logger;


void logging(LogLevel level, const char *message, ...) {
    if (logger == nullptr) {
        open_log_file();
    }
    char buffer[1024];
    va_list args;
    va_start(args, message);
    std::vsnprintf(buffer, 1024, message, args);
    va_end(args);
    switch (level) {
        case DEBUG:
            logger->debug(buffer);
            break;
        case INFO:
            logger->info(buffer);
            break;
        case WARN:
            logger->warn(buffer);
            break;
        case ERROR:
            logger->error(buffer);
            break;
    }
}

void logging(const char *message, ...) {
    if (logger != nullptr) {
        char buffer[1024];
        va_list args;
        va_start(args, message);
        std::vsnprintf(buffer, 1024, message, args);
        va_end(args);
        logger->info(buffer);
        return;
    }
    //print datetime first
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    char time_string[80];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
    if (log_file != NULL) {
        fprintf(log_file, "%s ", time_string);
        va_list args;
        va_start(args, message);
        vfprintf(log_file, message, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
        return;
    }
    printf("%s ", time_string);
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
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
    logger = spdlog::daily_logger_mt("daily_logger", logPath + "/Recorder.log", 0, 0);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::debug);
    logger->log(spdlog::level::info, "Start RavennaRecorder");
//    log_file = fopen((logPath + "/Recorder" + time_string + ".log").c_str(), "a");
//    if (log_file == nullptr) {
//        printf("Can't open log file, will output to stdout\n");
//    } else {
//        time_t now = time(nullptr);
//        struct tm *local_time = localtime(&now);
//        char time_string[80];
//        strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
//        logging("Start RavennaRecorder");
//    }
}

void set_log_level(int level) {
    if (logger == nullptr) {
        return;
    }
    spdlog::flush_every(std::chrono::seconds(3));
    switch (level) {
        case LogLevel::DEBUG:
            logger->set_level(spdlog::level::debug);
            break;
        case LogLevel::INFO:
        default:
            logger->set_level(spdlog::level::info);
            break;
        case LogLevel::WARN:
            logger->set_level(spdlog::level::warn);
            break;
        case LogLevel::ERROR:
            logger->set_level(spdlog::level::err);
            break;
    }
}
