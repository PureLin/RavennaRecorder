//
// Created by ME on 2023/12/11.
//

#ifndef RAVENNAPI_LOG_H
#define RAVENNAPI_LOG_H

extern FILE *log_file;

void logging(const char *message, ...); // Function declaration
void open_log_file(); // Function declaration

#endif //RAVENNAPI_LOG_H
