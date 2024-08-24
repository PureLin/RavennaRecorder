//
// Created by ME on 2023/12/11.
//

#ifndef RAVENNAPI_LOG_H
#define RAVENNAPI_LOG_H

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
};

void logging(LogLevel level, const char *message, ...);

void logging(const char *message, ...);

void open_log_file();

void set_log_level(int level);

#endif //RAVENNAPI_LOG_H
