/*
 * test_taskop.cpp
 *
 *  Created on: Nov 19, 2016
 *      Author: sunchao
 */
/*  taskop模块和数据通信模块的联合测试：
 *  发送一个Message，看服务器是否能接收到。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "taskop.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/transfer.h"

static int send_test()
{
    const char *host = config.get_string("MAIN", "Host", "127.0.0.1");
    int port = config.get_int("MAIN", "Port", 9999);
    Connection conn(host, port);

    add_message_task(&send_queue, "I'm client.", 3);
    TaskInfo *task = send_queue.next_task();
    RawData *data = pack_data(task, &send_queue);
    conn.send_data(data);
    send_queue.del_task(task->task_id);
    delete data;
    delete task;

    conn.close_conn();
    return 0;
}

static int start_server() {
    int port = config.get_int("MAIN", "Port", 9999);
    int sock_num = config.get_int("MAIN", "MaxLink", 5);
    int reuse_on = config.get_int("MAIN", "ReuseOn", 1);
    Socket socket(port, sock_num, reuse_on);    // 启动监听服务
    Connection *conn = NULL;
    while (conn == NULL) {
        sleep(1);
        conn = socket.client_link();
    }

    assert(conn != NULL);

    if (conn->has_data() > 0) {
        RawData *data = conn->recv_data();
        if (data == NULL) {
            printf("Received Data is NULL.\n");
        } else if (data->data() == NULL) {
            printf("Received Data with length 0.\n");
            delete data;
        } else {
            TaskInfo *task = unpack_data(data, &recv_queue);
            printf("Server received: %s\n", task->task_info);
            recv_queue.del_task(task->task_id);
            delete task;
            delete data;
        }
    }

    conn->close_conn();
    delete conn;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of config filename!");
        exit(1);
    }
    char *filename = argv[1];
    config.init(filename);  // 初始化配置文件
    const char *log_dir = config.get_string("LOG", "Directory", "build/logfiles_test");
    log.init(log_dir);      // 初始化日志模块
    init_taskqueues();

    send_queue.del_all_tasks();
    recv_queue.del_all_tasks();
    if (config.get_int("MAIN", "Server", 1) == 1) {
        assert(start_server() == 0);
    } else {
        assert(send_test() == 0);
    }
    printf("All tests passed!\n");

    return 0;
}
