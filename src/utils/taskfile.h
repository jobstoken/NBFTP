/*
 * taskfile.h
 *
 *  Created on: Nov 19, 2016
 *      Author: sunchao
 */

#ifndef TASKFILE_H_
#define TASKFILE_H_

#include <time.h>
#include "file.h"

enum TaskType:int {
    SMALL_FILE,     // 小文件
    LARGE_FILE,     // 大文件
    MESSAGE,        // 消息
    MD5_CHECK,      // 发送较验码
    LASTPOS,        // 接收数据错误，发送已接收的数据位置
    COMPLETE_SIG,   // 本次传送完成，即传输队列为空
    CHECK_PASS,     // 较验正确回执
    CHECK_FAILED,   // 较验错误回执
    REPLY           // 一般回执
};

enum TaskStatus:int {
    INITIAL,
    PENDING,
    STARTED,
    TRANSING,
    PAUSED,
    ERROR,
    FAULT,
    CHECKED,
    COMPLETED,
    INCONSIST
};

enum CtrlType:int {
    CHECKCOR_SIG,   // 较验正确回执
    CHECKERR_SIG,   // 较验错误回执
    NORMAL_SIG      // 一般回执
};

/* 任务信息 */
struct TaskInfo {
    long task_id;               // 任务ID
    TaskType type;              // 任务类型
    TaskStatus status;          // 任务状态
    int priority;               // 任务优先级
    int error_count;            // 错误次数
    long file_size;             // 文件大小
    long block_size;            // 分块大小
    long current_position;      // 当前位置
    long error_time;            // 出错时间
    long resume_interval;       // 恢复时间
    long ctrl_param;            // 控制信息的参数
    char md5[16];               // 文件的md5值（接收文件时使用）
    int need_reply;             // 是否需要接收回执
    char ignore[36];            // 调整大小到128
    char task_info[128];        // 任务信息（原文件名/消息标识)
    char store[256];            // 保存任务信息（文件缓存全路径/消息）
};

struct TaskList {
    TaskInfo info;
    TaskList *next;
};

class TaskFile {
private:
    char *_filename;            // 保存任务的文件(路径) .task
    TaskList *_rev_tasks;           // 保存任务的链表，与任务文件同步

public:
    TaskFile();
    ~TaskFile();

    /* 设置保存任务的文件。先创建目录，再创建该目录下的文件。*/
    void init(const char *filename);
    /* 读取任务文件，保存到_tasks中。 */
    /* read tasks to memory. */
    void read_tasks();
    /* 保存任务到文件中。 */
    /* write tasks to file. */
    void write_tasks();

    /* 获取特定任务 */
    TaskInfo *get_task(const long task_id);
    /* 向链表中添加任务 */
    int add_task(const TaskInfo task);
    /* 从链表中删除任务 */
    void del_task(const long task_id);
    /* 清理链表 */
    void clear();

    /* 获取任务列表。 */
    /* check whether the task exists. */
    TaskList *get_rev_tasks();
};

#endif /* TASKFILE_H_ */
