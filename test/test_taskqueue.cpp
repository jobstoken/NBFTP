/*
 * test_taskfile.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: sunchao
 */
/*  测试task模块：向队列中添加任务、更新任务、删除任务、删除所有任务。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "utils/taskqueue.h"
#include "utils/log.h"

static void print_task(const TaskInfo task, void *_arg) {
    printf("Id: %ld, Priority: %d, %s\n",
            task.task_id, task.priority, task.task_info);
}

int test_add_task(TaskQueue *queue) {
    printf("Add task\n");
    TaskInfo *task = queue->new_task("Makefile", 3, SMALL_FILE);
    task->status = PENDING;
    queue->update_task(*task);
    task = queue->new_task("server.conf");
    task->status = PENDING;
    queue->update_task(*task);
    queue->walk_tasks(print_task, NULL);
    return 0;
}

int test_update_task(TaskQueue *queue) {
    printf("Update task\n");
    TaskInfo *task = queue->next_task();
    task->priority = 1;
    queue->update_task(*task);
    queue->walk_tasks(print_task, NULL);
    return 0;
}

int test_delete(TaskQueue *queue) {
    printf("Delete task\n");
    TaskInfo *task = queue->next_task();
    queue->del_task(task->task_id);
    queue->walk_tasks(print_task, NULL);
    return 0;
}

int test_delete_all(TaskQueue *queue) {
    printf("Delete all task\n");
    queue->del_all_tasks();
    queue->walk_tasks(print_task, NULL);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Need parameters of filename and cache dir!");
        exit(1);
    }
    char *filename = argv[1];
    char *cache = argv[2];

    log.init("build");
    TaskQueue *queue = new TaskQueue();//初始化测试
    queue->init(filename);//绑定队列文件filename
    queue->set_cache_dir(cache);
    assert(!test_add_task(queue));
    assert(!test_update_task(queue));
    assert(!test_delete(queue));
    assert(!test_delete_all(queue));
    printf("All tests passed!\n");

    return 0;
}
