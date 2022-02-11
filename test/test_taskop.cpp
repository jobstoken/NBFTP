/*
 * test_taskop.cpp
 *
 *  Created on: Apr 9, 2017
 *      Author: sunchao
 */
/* 测试任务模块：
 * 打包，解包任务
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "taskop.h"
#include "monitorop.h"
#include "utils/log.h"
#include "utils/config.h"

static int test_message_task() {
    send_queue.del_all_tasks();
    add_message_task(&send_queue, "Hello", 3);
    TaskInfo *task = send_queue.next_task();
    RawData *data = pack_data(task, &send_queue);
    recv_queue.del_all_tasks();
    TaskInfo *task2 = unpack_data(data, &recv_queue);
    assert(strcmp(task->task_info, task2->task_info) == 0);
    assert(task->task_id == task2->task_id);
    assert(task->need_reply == task2->need_reply);
    delete task;
    delete task2;
    delete data;
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
    init_monitors();

    test_message_task();
    stop_monitors();
    printf("All tests passed!\n");

    return 0;
}
