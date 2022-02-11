/*
 * taskqueue.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: sunchao
 */

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "file.h"
#include "taskfile.h"
#include "taskqueue.h"

TaskQueue::TaskQueue() {
    _cache_dir = NULL;
    srand(time(NULL));
}

TaskQueue::~TaskQueue() {
    if (_cache_dir != NULL) {
        delete _cache_dir;
        _cache_dir = NULL;
    }
}

void TaskQueue::init(const char *taskfile) {
    _taskfile.init(taskfile);
}

//设置要传输文件的文件夹(目录)
void TaskQueue::set_cache_dir(const char *dir) {
    log.debug("[TASK] Set cache directory: %s.", dir);
    _cache_dir = strdup(dir);
    int len = strlen(_cache_dir);
    if (_cache_dir[len - 1] == '/') _cache_dir[len - 1] = 0;
    create_dir(_cache_dir);//task文件
}

void TaskQueue::inter_walk(TaskList *root, peek_task_t operation, void *params) {
    if (root == NULL) return;
    inter_walk(root->next, operation, params);
    (*operation)(root->info, params);
}

void TaskQueue::walk_tasks(peek_task_t operation, void *params) {
    inter_walk(_taskfile.get_rev_tasks(), operation, params);
}

void TaskQueue::update_task(const TaskInfo task) {
    TaskInfo *temp = _taskfile.get_task(task.task_id);
    if (temp == NULL) {
        log.error("[TASK] Can not update task: not find!");
    } else {
        log.debug("[TASK] Update task %s(%ld).", task.task_info, task.task_id);
        *temp = task;//更新节点
        if (task.type <= MESSAGE) {
            _taskfile.write_tasks();
        }
    }
}

TaskInfo *TaskQueue::find_task(const long task_id) {
    TaskInfo *task = _taskfile.get_task(task_id);
    if (task == NULL) return task;
    else return new TaskInfo(*task);
}

// Get next task in disk
TaskInfo *TaskQueue::next_task() {
    TaskInfo *task = NULL;
    TaskList *temp = _taskfile.get_rev_tasks();
    for (;temp != NULL; temp = temp->next) {
        if (temp->info.status == INITIAL) continue;
        if (temp->info.status == COMPLETED) continue;
        if (temp->info.status == CHECKED) continue;
        if (temp->info.status == PAUSED) continue;
        if (temp->info.status == FAULT) continue;
        time_t resume = temp->info.error_time + temp->info.resume_interval;
        if (temp->info.error_count > 0 && resume > time(NULL)) continue;
        if (task == NULL) {
            task = &temp->info;
        } else if (temp->info.priority <= task->priority) {
            task = &temp->info;
        }
    }
    if (task == NULL) return task;
    else return new TaskInfo(*task);
}

// Delete exact task in disk
void TaskQueue::del_task(const long task_id) {
    _taskfile.del_task(task_id);
}

// Clear diskFile in disk
void TaskQueue::del_all_tasks() {
    _taskfile.clear();
    _taskfile.write_tasks();
}

// For recv_queue, status = PENDING
TaskInfo *TaskQueue::new_task(const TaskType type, long task_id) {
    TaskInfo *task = new TaskInfo;
    task->task_id = task_id;
    task->type = type;
    task->status = PENDING;
    task->priority = 5;
    task->block_size = -1;
    task->current_position = -1;
    task->error_count = 0;
    task->need_reply = 0;
    sprintf(task->store, "%s/%010ld", _cache_dir, task->task_id);
    // 添加到任务列表中
    _taskfile.add_task(*task);
    return task;
}

// For Send_queue, status = INITIAL
TaskInfo *TaskQueue::new_task(const char *path_or_info, const int priority, const TaskType type) {
    TaskInfo *task = new TaskInfo;
    task->task_id = rand();
    task->type = type;
    task->status = INITIAL;
    task->priority = priority;
    task->block_size = -1;
    task->current_position = -1;
    task->error_count = 0;
    task->need_reply = 0;
    if (type == SMALL_FILE || type == LARGE_FILE) {
        sprintf(task->store, "%s/%010ld", _cache_dir, task->task_id);
        char *filename = strext(path_or_info, '/');
        strcpy(task->task_info, filename);
        delete filename;
        copy_file(path_or_info, task->store);//将文件从test文件夹下拷贝到task文件夹下
        File fp(task->store, "rb");
        if (fp.is_null()) task->file_size = 0;
        else task->file_size = fp.size();
        fp.get_md5(task->md5);
        fp.close();
    } else if (type == MESSAGE) {
        strcpy(task->task_info, "MESSAGE");
        strcpy(task->store, path_or_info);
    } else {
        strcpy(task->task_info, path_or_info);
    }
    // 添加到任务列表中
    _taskfile.add_task(*task);
    return task;
}

TaskInfo *TaskQueue::new_task(const char *full_path) {
    return new_task(full_path, 5, SMALL_FILE);
}

TaskInfo *TaskQueue::new_task(const char *full_path, const int priority) {
    return new_task(full_path, priority, SMALL_FILE);
}
