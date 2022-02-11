/*
 * test_communication.cpp
 *
 *  Created on: Mar 30, 2018
 *      Author: sunchao
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "taskop.h"
#include "communication.h"
#include "utils/log.h"
#include "utils/transfer.h"

void start_server(int port) {
    Socket socket(port, 2, 1);    // 启动监听服务

    int comm_ret = 0;       // 发送接收任务的返回值
    while (comm_ret != 1) {
        usleep(100000);
        Connection *conn = socket.client_link();    // 客户端连接
        if (conn != NULL && conn->connected()) {
            printf("Received connection.\n");
            conn->set_timeout(1);         // 设置超时时间
            comm_ret = send_tasks(conn);     // 发送数据
        }
        if (conn != NULL) delete conn;
    }
    printf("Server Complete.\n");
}

void start_client(char *host, int port) {
    int comm_ret = 0;           // 发送接收任务的返回值
    while (comm_ret != 1) {
        Connection *conn = new Connection(host, port);  // 连接服务器
        if (conn != NULL && conn->connected()) {
            printf("Connection linked.\n");
            conn->set_timeout(1);         // 设置超时时间
            comm_ret = receive_tasks(conn);  // 接收数据
        }
        if (conn != NULL) delete conn;
    }
    printf("Client Complete.\n");
}

static char SERVER[] = "127.0.0.1";

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Need three parameter of port, log_server, log_client!");//参数依次为端口（9999)，日志目录×2
        exit(1);
    }

    int port = atoi(argv[1]);
    pid_t cid = fork();
    if (cid != 0) {         //server端
        log.init(argv[2]);  //初始化日志
        start_server(port);
    } else {//client
        usleep(1000000);
        log.init(argv[3]);  //初始化日志
        start_client(SERVER, port);
    }

    printf("All tests passed!\n");
    return 0;
}
