/*
 * taskqueue.h
 *
 *  Created on: Dec 22, 2016
 *      Author: sunchao
 */

#ifndef TASKQUEUE_H_
#define TASKQUEUE_H_

#include "file.h"
#include "taskfile.h"

typedef void (*peek_task_t)(const TaskInfo, void *);

class TaskQueue {
private:
    TaskFile _taskfile;
    char *_cache_dir;//要传输的文件的文件夹,task文件

    void inter_walk(TaskList *root, peek_task_t operation, void *params);

public:
    TaskQueue();
    ~TaskQueue();

    /* 初始化任务队列。绑定任务文件，从任务文件中读取任务，并保持同步。 */
    void init(const char *taskfile);
    /* 设置保存任务文件的目录。 */
    void set_cache_dir(const char *dir);
    /* 对所有任务操作。 */
    void walk_tasks(peek_task_t operation, void *params);
    /* 获取当前需要处理的任务。选取优先级最高的任务，如果上次传递出错并且没到恢复时间则选取下一个。 */
    TaskInfo *next_task();
    /* 获取任务。参数为任务ID。 */
    TaskInfo *find_task(const long task_id);
    /* 更新任务。如果任务不存在，则忽略。 */
    void update_task(const TaskInfo task);
    /* 删除任务。参数为任务ID。 */
    void del_task(const long task_id);
    /* 清空所有任务。 */
    void del_all_tasks();

    /* 创建新任务。参数为：待传送的文件名，优先级（默认为5），类型（默认为1）。 */
    TaskInfo *new_task(const TaskType type, long task_id);
    TaskInfo *new_task(const char *full_path);
    TaskInfo *new_task(const char *full_path, const int priority);
    TaskInfo *new_task(const char *full_path, const int priority, const TaskType type);
};

#endif /* TASKQUEUE_H_ */
