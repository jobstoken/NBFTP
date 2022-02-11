/*
 * test_control.cpp
 *
 *  Created on: Mar 29, 2018
 *      Author: sunchao
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "taskop.h"
#include "control_part.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/transfer.h"

void pass() {}

static void start_server(int port) {
    Control control(port);
    control.server_routine(pass);
}

static void show_result(RawData *response) {
    if (response == NULL) {
        fprintf(stderr, "Options or parameters Error!\n");
        exit(1);
    }
    char *message = new char[response->length()];
    while (response->pos() < response->length() - 4) {
        response->unpack_string(message);
        printf("%s\n", message);
    }
    delete message;
}
static void start_client(int port) {
    Control control(port);
    RawData *response;

    response = control.add_send_message("Hello", 3);
    show_result(response);
    delete response;

    response = control.list_send_tasks();
    show_result(response);
    delete response;

    response = control.delete_send_task(-1);
    show_result(response);
    delete response;

    response = control.stop_server();
    show_result(response);
    delete response;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Need two parameter of port, config!");//参数依次为端口，日志目录
        exit(1);
    }

    int port = atoi(argv[1]);
    pid_t cid = fork();
    if (cid != 0) {         //server端
        usleep(1000000);
        start_client(port);
    } else {//client
        config.init(argv[2]);  // 初始化配置文件
        const char *log_dir = config.get_string("LOG", "Directory", "build/logfiles_test");
        log.init(log_dir);      // 初始化日志模块
        init_taskqueues();
        start_server(port);
    }

    printf("All tests passed!\n");
    return 0;
}

