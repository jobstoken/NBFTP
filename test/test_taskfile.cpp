/*
 * test_taskfile.cpp
 *
 *  Created on: Mar 10, 2018
 *      Author: sunchao
 */
/* 测试任务模块：
 * 任务基本功能。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "utils/taskfile.h"
#include "utils/log.h"
#include "utils/config.h"

static int test_taskinfo() {
    TaskInfo a;
    a.task_id = 9;
    TaskInfo *b = new TaskInfo(a);
    assert(a.task_id == b->task_id);
    b->task_id = 8;
    assert(a.task_id != b->task_id);
    return 0;
}

static int test_taskfile(char *cache) {
    TaskFile tf;
    tf.init(cache);
    tf.clear();
    tf.write_tasks();
    TaskInfo a;
    a.task_id = 9;
    strcpy(a.task_info, "test");
    a.type = MESSAGE;
    tf.add_task(a);
    assert(strcmp(tf.get_task(9)->task_info, a.task_info) == 0);
    a.task_id = 8;
    strcpy(a.task_info, "test2");
    tf.add_task(a);
    return 0;
}

static int test_taskfile2(char *cache) {
    TaskFile tf;
    tf.init(cache);
    assert(strcmp(tf.get_task(9)->task_info, "test") == 0);
    assert(strcmp(tf.get_task(8)->task_info, "test") != 0);
    tf.del_task(9);
    assert(tf.get_task(9) == NULL);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Need two parameter of config filename and taskfile!");
        exit(1);
    }
    char *filename = argv[1];
    config.init(filename);  // 初始化配置文件
    const char *log_dir = config.get_string("LOG", "Directory", "build/logfiles_test");
    log.init(log_dir);      // 初始化日志模块

    test_taskinfo();
    test_taskfile(argv[2]);
    test_taskfile2(argv[2]);
    printf("All tests passed!\n");

    return 0;
}
