/*
 * taskfile.cpp
 *
 *  Created on: Nov 19, 2016
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>

#include "taskfile.h"
#include "file.h"
#include "log.h"

TaskFile::TaskFile() {
    _filename = NULL;
    _rev_tasks = NULL;
}

TaskFile::~TaskFile() {
    clear();
    if (_filename != NULL) {
        delete _filename;
    }
}

void TaskFile::init(const char *filename) {
    log.debug("[TASK] Initialize task module with file: %s.", filename);
    if (strlen(filename) > 0) {
        create_file(filename);
        file_mode(filename, 0644);
        if (_filename != NULL) {
            delete _filename;
        }
        _filename = strdup(filename);
        read_tasks();
    }
}

// Read info from disk to memory
void TaskFile::read_tasks() {
    File fp(_filename, "rb");//打开一个文件
    if (fp.is_null()) return;
    log.info("[TASK] Read tasks from file: %s!", _filename);
    clear();//清楚当前队列里的任务
    TaskInfo temp;
    TaskList **head = &_rev_tasks;//头指针，指向当前的任务链表，尾插法
    while (fp.read(&temp, sizeof(TaskInfo), 1) > 0) {
        *head = new TaskList;
        (*head)->info = temp;
        head = &(*head)->next;
    }
    *head = NULL;
    fp.close();
}

// Write info from memory to disk, just disk file operation
void TaskFile::write_tasks() {
    File fp(_filename, "wb+");
    if (fp.is_null()) return;
    log.info("[TASK] Write tasks to file: %s!", _filename);
    TaskList *temp = _rev_tasks;
    while (temp != NULL) {
        if (temp->info.type <= MESSAGE) {
            fp.write(&temp->info, sizeof(TaskInfo), 1);
        }
        temp = temp->next;
    }
    fp.close();
}

//查看这个task是否存在
TaskInfo *TaskFile::get_task(const long task_id) {
    TaskList *temp = _rev_tasks;
    while (temp != NULL) {
        if (temp->info.task_id == task_id) {
            return &temp->info;
        }
        temp = temp->next;
    }
    return NULL;
}

int TaskFile::add_task(const TaskInfo task) {
    if (get_task(task.task_id) != NULL) {
        log.error("[TASK] Can not add task: already exists!");
        return -1;
    } else {//不存在就加入到队列中
        log.info("[TASK] Add new task %s(%ld).", task.task_info, task.task_id);
        TaskList *new_task = new TaskList;
        new_task->info = task;
        new_task->next = _rev_tasks;//头插法
        _rev_tasks = new_task;
        if (task.type <= MESSAGE) {
            write_tasks();//整个任务队列的持久化
        }
        return 0;
    }
}

void TaskFile::del_task(const long task_id) {
    if (_rev_tasks == NULL) return;

    TaskList *current = _rev_tasks;
    if (_rev_tasks->info.task_id == task_id) {
        _rev_tasks = _rev_tasks->next;
    } else {
        TaskList *parent = current;
        current = parent->next;
        while (current != NULL) {
            if (current->info.task_id == task_id) {
                parent->next = current->next;
            }
            parent = current;
            current = parent->next;
        }
    }
    if (current != NULL) {
        log.info("[TASK] Delete task %s(%ld).", current->info.task_info, task_id);
        if (current->info.type <= MESSAGE) {
            write_tasks();
        }
        delete current;
    }
}

void TaskFile::clear() {
    log.info("[TASK] Clear tasks.");
    TaskList *current;
    while (_rev_tasks != NULL) {
        current = _rev_tasks;
        _rev_tasks = _rev_tasks->next;
        delete current;
    }
}

TaskList *TaskFile::get_rev_tasks() {
    return _rev_tasks;
}

