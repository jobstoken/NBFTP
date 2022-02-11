/*
 * communication.cpp
 *
 *  Created on: Jan 7, 2017
 *      Author: sunchao
 */

#include <time.h>
#include <string.h>
#include <unistd.h>
#include "communication.h"
#include "taskop.h"
#include "dataop.h"
#include "extension.h"

#include "utils/file.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/transfer.h"

/* 获取将要发送的任务。 */
static TaskInfo *get_pending_task() {
    TaskInfo *task = send_queue.next_task();
    if (task == NULL) {
        //如果任务发送完成，就添加一个标志没有需要发送的文件的标志Task
        add_reply_task(&send_queue, COMPLETE_SIG, 0);
        task = send_queue.next_task();
    }
    return task;
}

/* 处理回复任务。返回值：0为正常接收，1为接收完毕. */
static int process_task(TaskInfo *task) {
    if (task->type == SMALL_FILE || task->type == LARGE_FILE) {
        //要先正确接收->再进行检测->通过检测才能发ACK
        if (task->status == COMPLETED) {
            File fp(task->store, "r");
            if (fp.check_md5(task->md5) == 1) {
                log.info("[COMM] Check md5 success! Now send CHECKCOR_SIG");
                //加入返回队列里一个带有正确检测标志的task
                add_reply_task(&send_queue, CHECK_PASS, task->task_id);
                task->status = CHECKED;
                recv_queue.update_task(*task);
                call_extension("FILE", task->store, task->task_info);
            } else {
                log.error("[COMM] Check md5 failed! Now send CHECKERR_SIG");
                add_reply_task(&send_queue, CHECK_FAILED, task->task_id);
                // If error, delete the received data
                task->status = INCONSIST;
                task->current_position = 0;
                recv_queue.update_task(*task);
            }
        } else if (task->status == INCONSIST) {
            add_lastpos_task(&send_queue, task->task_id, task->current_position);
        } else {
            add_reply_task(&send_queue, REPLY, task->task_id);
        }
    } else if (task->type == MESSAGE) {
        add_reply_task(&send_queue, REPLY, task->task_id);
        if (task->status == COMPLETED) {
            call_extension("MESSAGE", task->store, task->task_info);
        }
    } else {
        recv_queue.del_task(task->task_id);
        if (task->type == LASTPOS) {
            TaskInfo *file_task = send_queue.find_task(task->ctrl_param);
            log.debug("[COMM] Find file task %ld.", file_task->task_id);
            file_task->current_position = task->current_position;
            file_task->error_count ++;
            file_task->status = PENDING;
            send_queue.update_task(*file_task);
            delete file_task;
        } else if (task->type == CHECK_PASS) {
            TaskInfo *file_task = send_queue.find_task(task->ctrl_param);
            log.debug("[COMM] Find file task %ld.", file_task->task_id);
            file_task->status = CHECKED;
            send_queue.update_task(*file_task);
            delete file_task;
        } else if (task->type == CHECK_FAILED) {
            TaskInfo *file_task = send_queue.find_task(task->ctrl_param);
            log.debug("[COMM] Find file task %ld.", file_task->task_id);
            file_task->current_position = 0;
            file_task->status = PENDING;
            send_queue.update_task(*file_task);
            delete file_task;
        } else if (task->type == REPLY) {
            TaskInfo *file_task = send_queue.find_task(task->ctrl_param);
            log.debug("[COMM] Find file task %ld.", file_task->task_id);
            task_success(file_task, &send_queue);
            delete file_task;
        } else if (task->type == COMPLETE_SIG) {
            log.debug("[COMM] Received COMPLETE_SIG.");
            return 1;
        }
    }
    return 0;
}

static int receive_task(Connection *conn);

/* 发送任务并处理。返回值：0为正常发送，1为发送空任务（队列无可发送任务），-1为发送出错。 */
static int send_task(Connection *conn) {
    TaskInfo *task = get_pending_task();
    log.debug("[COMM] Get task %s(%ld) to send.", task->task_info, task->task_id);
    if (task == NULL) return 0;

    // Pack task to RawData
    RawData *data = pack_data(task, &send_queue);
    if (data == NULL) {
        // 任务数据获取失败
        log.error("[COMM] CANNOT get data of task %s(%ld)", task->task_info, task->task_id);
        task->status = FAULT;
        task->error_count = -1;
        send_queue.update_task(*task);
        delete task;
        return -1;
    }
    // 发送任务数据
    int send_ret = conn->send_data(data);
    delete data;
    if (send_ret != 0) {
        task_failure(task, &send_queue);
        delete task;
        return -1;
    }
    // 处理返回消息
    if (task->need_reply) {//大小文件，MESSAGE在任务创建的时候默认需要回执
        if (receive_task(conn) != 0) {
            task_failure(task, &send_queue);
            delete task;
            return -1;
        }
    } else {
        task_success(task, &send_queue);
    }
    // 返回值处理
    if (task->type == COMPLETE_SIG) {
        delete task;
        return 1;
    } else {
        delete task;
        return 0;
    }
}

/* 接收任务并处理。返回值：0为正常接收，1为接收完毕，-1为接收出错。 */
static int receive_task(Connection *conn) {
    if (conn->has_data() != 1) return -1;

    // Receive RawData
    RawData *data = conn->recv_data();
    if (data == NULL) return -1;
    else if (data->data() == NULL) {
        delete data;
        return -1;
    }

    TaskInfo *task = unpack_data(data, &recv_queue);
    delete data;
    if (task == NULL) {
        log.error("[COMM] Unpacked data is NULL.");
        return -1;
    }
    log.debug("[COMM] Received task %s(%ld).", task->task_info, task->task_id);
    int ret = process_task(task);//处理接收到的任务
    if (task->type <= MESSAGE) {
        send_task(conn);
    }
    delete task;
    return ret;
}

int send_tasks(Connection *conn) {
    while (status == 1) {
        int ret = send_task(conn);
        if (ret != 0) return ret;
    }
    return 0;
}

int receive_tasks(Connection *conn) {
    while (status == 1) {
        int ret = receive_task(conn);
        if (ret != 0) return ret;
    }
    return 0;
}
