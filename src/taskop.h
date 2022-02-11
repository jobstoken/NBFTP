/*
 * taskop.h
 *
 *  Created on: Oct 23, 2016
 *      Author: sunchao
 */

#ifndef TASKOP_H_
#define TASKOP_H_

#include "utils/taskqueue.h"
#include "utils/rawdata.h"

/* 待发送的任务队列。 */
extern TaskQueue send_queue;
/* 接收到的任务队列。 */
extern TaskQueue recv_queue;

/* 读取配置文件，获取任务队列和数据队列。 */
void init_taskqueues();

/* 添加文件任务。 */
void add_file_task(TaskQueue *queue, const char *filename, int priority);

/* 添加消息任务。 */
void add_message_task(TaskQueue *queue, const char *message, int priority);

/* 添加较验信息。 */
void add_md5_check_task(TaskQueue *queue, long task_id, const char *md5);

/* 添加回复信号。 */
void add_reply_task(TaskQueue *queue, TaskType control, long task_id);

/* 添加错误位置信号。 */
void add_lastpos_task(TaskQueue *queue, long task_id, long position);

/* 打包成待传输的数据（原始格式）。 */
RawData *pack_data(TaskInfo *task, TaskQueue *queue);

/* 解析接收到的数据（原始格式）。 */
TaskInfo *unpack_data(RawData *data, TaskQueue *queue);

/* 发送数据成功时的后续操作。 */
void task_success(TaskInfo *task, TaskQueue *queue);

/* 发送数据失败时的后续操作。 */
void task_failure(TaskInfo *task, TaskQueue *queue);

#endif /* TASKOP_H_ */
