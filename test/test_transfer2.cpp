/*
 * test_transfer2.cpp
 *
 *  Created on: Mar 3, 2018
 *      Author: sunchao
 */
/*  测试网络传输模块：启动两个进程，server端有两个线程，Client向两个server发送两个数据，然后输出。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "utils/log.h"
#include "utils/transfer.h"

char SERVER[] = "127.0.0.1";

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

    conn->set_timeout(5);
    RawData data(32);//要发送的数据
    data.pack(iamclient, strlen(iamclient) + 1);//打包
    conn->send_data(&data);//发送

    conn->close_conn();
    delete conn;
}


void *start_server(void *arg) {
    int *port = (int *)arg;
    Socket server(*port, 5);//建立一个socket
    Connection *conn = NULL;//建立连接
    for (int i = 0; i < 2; i++) {
        while (conn == NULL) {
            usleep(100000);
            conn = server.client_link();
        }

        assert(conn != NULL);//确保有连接
        conn->set_timeout(5);

        if (conn->has_data() > 0) {
            RawData *rdata = conn->recv_data();//进行接收
            printf("Server(%d) received: %s\n", *port, rdata->data());
            assert(strcmp(rdata->data(), iamclient) == 0);//将接收数据进行对比
            delete rdata;
        }

        conn->close_conn();
        delete conn;
        conn = NULL;
        usleep(100000);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Need four parameter of port1, port2, log_server, log_client!");//参数依次为端口（9999)，日志目录×2
        exit(1);
    }

    int port1 = atoi(argv[1]);
    int port2 = atoi(argv[2]);
    pthread_t pid1, pid2;
    pid_t cid = fork();
    if (cid != 0) {         //server端
        log.init(argv[3]);  //初始化日志
        pthread_create(&pid1, NULL, start_server, (void *)&port1);
        pthread_create(&pid2, NULL, start_server, (void *)&port2);
        pthread_join(pid1, NULL);
        pthread_join(pid2, NULL);
    } else {//client
        usleep(100000);
        log.init(argv[4]);//
        send_test(SERVER, port1);//发送数据
        send_test(SERVER, port2);//发送数据
        send_test(SERVER, port2);//发送数据
        send_test(SERVER, port1);//发送数据
    }
    printf("All tests passed!\n");

    return 0;
}

