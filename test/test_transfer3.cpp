/*
 * test_transfer3.cpp
 *
 *  Created on: Mar 3, 2018
 *      Author: sunchao
 */
/*  测试网络传输模块：启动两个进程，发送一个数据，等待使Client超时，然后输出。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "utils/log.h"
#include "utils/transfer.h"

const char *iamclient = "Hello, I'm client!";
const char *iamserver = "Hello, I'm server!";

void send_test(char *host, int port)
{
    Connection *conn = new Connection(host, port);//启动连接
    while (!conn->connected()) {
        delete conn;
        usleep(1000);
        conn = new Connection(host, port);
    }

    assert(conn != NULL);//确保有连接
    conn->set_timeout(2);

    RawData data(32);//要发送的数据
    data.pack(iamclient, strlen(iamclient) + 1);//打包
    conn->send_data(&data);//发送

    assert(conn->has_data() < 0);

    conn->close_conn();
    delete conn;
}


static void start_server(int port) {
    Socket server(port, 5);//建立一个socket
    Connection *conn = NULL;//建立连接
    while (conn == NULL) {
        usleep(100000);
        conn = server.client_link();
    }

    assert(conn != NULL);//确保有连接

    usleep(3000000);

    if (conn->has_data() > 0) {
        RawData *rdata = conn->recv_data();//进行接收
        printf("Server received: %s\n", rdata->data());
        assert(strcmp(rdata->data(), iamclient) == 0);//将接收数据进行对比
        delete rdata;
    }

    conn->close_conn();
    delete conn;
}

char SERVER[] = "127.0.0.1";

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "Need three parameter of port, log_server, log_client!");//参数依次为端口（9999)，日志目录×2
        exit(1);
    }

    int port = atoi(argv[1]);
    pid_t cid = fork();
    if (cid != 0) {         //server端
        log.init(argv[2]);  //初始化日志
        start_server(port); //流程测试
    } else {//client
        usleep(100000);
        log.init(argv[3]);//
        send_test(SERVER, port);//发送数据
    }

    printf("All tests passed!\n");

    return 0;
}

