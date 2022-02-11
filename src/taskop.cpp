/*
 * taskop.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>

#include "taskop.h"
#include "dataop.h"
#include "utils/basic.h"
#include "utils/log.h"
#include "utils/config.h"
#include "utils/taskqueue.h"
#include "utils/rawdata.h"

#define EXTLEN  32

/* 待发送的任务队列。 */
TaskQueue send_queue;
/* 接收到的任务队列。 */
TaskQueue recv_queue;

void init_taskqueues() {
    /*
     *从配置文件中获取任务队列存放的文件，然后作为参数传递
    */
    send_queue.init(config.get_string("TASK", "File", ".tasks"));
    send_queue.set_cache_dir(config.get_string("TASK", "Directory", "tasks"));
    recv_queue.init(config.get_string("DATA", "File", ".data"));
    recv_queue.set_cache_dir(config.get_string("DATA", "Directory", "data"));
}

void add_file_task(TaskQueue *queue, const char *full_path, int priority) {
    log.debug("[TASK] Add file task with priority %d: %s.", priority, full_path);
    //这里的full_path是test文件夹下的文件路径，要复制到task下面，new_task完成该操作
    //就是构造一个TaskInfo，将文件的属性进行设置，然后头插入到TaskList队列中
    //一个文件的传输就是一个传输任务，在TaskList中，要传输的任务保存在task文件中，文件名记录在_cache_dir
    //任务持久化在.task文件中，文件名记录在_filename中
    TaskInfo *task = queue->new_task(full_path, priority);//加入传输队列并持久化
    if (task->file_size < config.get_int("TASK", "BigFileSize", 40960)) {
        //根据实际大小设置大文件小文件标识
        task->type = SMALL_FILE;
    } else {
        task->type = LARGE_FILE;
    }
    task->need_reply = 1;
    task->current_position = -1;
    task->status = PENDING;
    queue->update_task(*task);
    delete task;
}

/* 小文件数据格式：标识，任务ID，文件MD5，文件名，文件内容。 */
static RawData *pack_small_file(TaskInfo *task, TaskQueue *queue) {
    //缓冲区大小是文件大小+32,同时为_data申请这么大的内存空间
    RawData *data = new RawData(task->file_size + EXTLEN);

    log.debug("[TASK] Pack small file %s.", task->task_info);
    data->pack_char(SMALL_FILE_DATA);
    data->pack_long(task->task_id);
    data->pack(task->md5, 16);
    data->pack_string(task->task_info);
    File fp(task->store, "rb");
    if (fp.is_null()) { delete data; return NULL; }
    data->pack_file(&fp, task->file_size);
    fp.close();
    return data;
}

/* 小文件数据格式：标识，任务ID，文件MD5，文件名，文件内容。 */
static TaskInfo *unpack_small_file(RawData *data, TaskQueue *queue) {
    // Create task Using task_id
    TaskInfo *task = queue->new_task(SMALL_FILE, data->unpack_long());
    //之后这些改动改动的不是队列里的task，而是它的拷贝，再进行改动，所以改动之后要对队列里的task进行更新
    task->need_reply = 1;//默认需要回复确认
    data->unpack(task->md5, 16);
    data->unpack_string(task->task_info);
    File fp(task->store, "wb+");
    if (fp.is_null()) { delete task; return NULL; }
    task->file_size = data->unpack_file(&fp);// 写入/data/下
    fp.close();
    task->current_position = task->file_size;
    task->status = COMPLETED;//完成读入标志
    log.debug("[TASK] Unpacked small file %s.", task->task_info);
    return task;
}

/* 大文件开始数据格式：标识，任务ID，文件MD5，文件名，文件大小。 */
static RawData *pack_large_file_begin(TaskInfo *task, TaskQueue *queue) {
    RawData *data = new RawData(EXTLEN);

    log.debug("[TASK] Pack large file %s(begin).", task->task_info);
    data->pack_char(LARGE_FILE_BEGIN);
    data->pack_long(task->task_id);
    data->pack(task->md5, 16);
    data->pack_string(task->task_info);
    data->pack_long(task->file_size);
    return data;
}

/* 大文件开始数据格式：标识，任务ID，文件MD5，文件名，文件大小。 */
static TaskInfo *unpack_large_file_begin(RawData *data, TaskQueue *queue) {
    // Create task using task_id
    TaskInfo *task = queue->new_task(LARGE_FILE, data->unpack_long());
    task->need_reply = 1;
    data->unpack(task->md5, 16);
    data->unpack_string(task->task_info);
    task->file_size = data->unpack_long();
    task->current_position = 0;
    task->block_size = 0;
    task->status = STARTED;
    log.debug("[TASK] Unpacked large file %s(begin).", task->task_info);
    return task;
}

static void update_block_size(long &block_size) {
    if (config.get_int("TASK", "AutoBlockSize", 0) == 0) {
        block_size = config.get_int("TASK", "FixedBlockSize", 8192);
    } else if (block_size <= 0) {
        block_size = config.get_int("TASK", "MinBlockSize", 4096);
    } else {
        block_size *= 2;
        long max_block = config.get_int("TASK", "MaxBlockSize", 4194304);
        if (block_size > max_block) {
            block_size = max_block;
        }
    }
}

/* 大文件分块数据格式：标识，任务ID，当前位置，文件内容。 */
static RawData *pack_large_file_part(TaskInfo *task, TaskQueue *queue) {
    update_block_size(task->block_size);
    RawData *data = new RawData(task->block_size + EXTLEN);
    long end = task->current_position + task->block_size;
    if (end > task->file_size) end = task->file_size;

    log.debug("[TASK] Pack large file %s(%ld-%ld).",
            task->task_info, task->current_position, end);
    data->pack_char(LARGE_FILE_PART);
    data->pack_long(task->task_id);
    data->pack_long(task->current_position);
    File fp(task->store, "rb");
    if (fp.is_null()) { delete data; return NULL; }
    fp.seek(task->current_position);//获取文件上次读到的位置
    data->pack_file(&fp, end - task->current_position);//继续读相应长度的数据
    fp.close();
    return data;
}

/* 大文件分块数据格式：标识，任务ID，当前位置，文件内容。 */
static TaskInfo *unpack_large_file_part(RawData *data, TaskQueue *queue) {
    TaskInfo *task = queue->find_task(data->unpack_long());
    if (task == NULL) {
        log.error("[TASK] Task not found when unpack_large_file_part()");
        return NULL;
    }
    task->need_reply = 1;
    log.debug("[TASK] Old task info: %s, current_position: %ld.", task->task_info, task->current_position);
    long current = data->unpack_long();
    log.debug("[TASK] New task info: %s, current_position: %ld.", task->task_info, current);
    if (current != task->current_position) {
        log.error("[TASK] Large file part disordered!");
        task->status = INCONSIST;
        return task;
    }
    const char *mode = "wb";
    if (current > 0) mode = "ab";
    File fp(task->store, mode);
    if (fp.is_null()) { delete task; return NULL; }
    long length = data->unpack_file(&fp);
    fp.close();
    task->current_position = current + length;
    task->status = (task->current_position >= task->file_size) ? COMPLETED : TRANSING;
    log.debug("[TASK] Unpacked large file %s(%ld-%ld).", task->task_info, current, current + length);
    return task;
}

void add_message_task(TaskQueue *queue, const char *message, int priority) {
    if (strlen(message) < 255) {
        log.debug("[TASK] Add message task with priority %d: %s.", priority, message);
        TaskInfo *task = queue->new_task(message, priority, MESSAGE);
        task->status = PENDING;
        task->need_reply = 1;
        queue->update_task(*task);
        delete task;
    } else {
        log.error("[TASK] CANNOT add message (too long): %s", message);
    }
}

/* 消息数据格式：标识，任务ID，消息(存在task_info中)。 */
static RawData *pack_message(TaskInfo *task, TaskQueue *queue) {
    RawData *data = new RawData(EXTLEN);

    log.debug("[TASK] Pack message %ld.", task->task_id);
    data->pack_char(MESSAGE_DATA);
    data->pack_long(task->task_id);
    data->pack_string(task->store);
    return data;
}

/* 消息数据格式：标识，任务ID，消息(存在task_info中)。 */
static TaskInfo *unpack_message(RawData *data, TaskQueue *queue) {
    // Create task Using task_id
    TaskInfo *task = queue->new_task(MESSAGE, data->unpack_long());
    task->need_reply = 1;
    strcpy(task->task_info, "MESSAGE");
    data->unpack_string(task->store);
    task->current_position = strlen(task->store);
    task->status = COMPLETED;
    log.debug("[TASK] Unpacked message %ld.", task->task_id);
    return task;
}

void add_reply_task(TaskQueue *queue, TaskType control, long task_id) {
    log.debug("[TASK] Add reply task with parameters: %d.", control);
    TaskInfo *task = queue->new_task("REPLY", 1, control);
    task->need_reply = 0;      //一般回复不需要server再进行回复
    task->ctrl_param = task_id;
    task->status = PENDING;
    queue->update_task(*task);
    delete task;
}

void add_lastpos_task(TaskQueue *queue, long task_id, long position) {
    log.debug("[TASK] Add lastPos task with parameters: %d.", position);
    TaskInfo *task = queue->new_task("LASTPOS", 1, LASTPOS);
    task->need_reply = 0;
    task->ctrl_param = task_id;
    task->current_position = position;
    task->status = PENDING;
    queue->update_task(*task);
    delete task;
}

/* 控制命令数据格式：标识，任务ID，控制类型，参数ID。 */
static RawData *pack_reply(TaskInfo *task, TaskQueue *queue) {
    RawData *data = new RawData(EXTLEN);

    log.debug("[TASK] Pack reply task %ld.", task->task_id);
    data->pack_char((DataType)task->type);//int 强转成 char类型
    data->pack_long(task->task_id);
    data->pack_long(task->ctrl_param);
    if (task->type == LASTPOS) {
        data->pack_long(task->current_position);
    }
    return data;
}

/* 控制命令数据格式：标识，任务ID，控制类型，参数ID。 */
static TaskInfo *unpack_reply(DataType type, RawData *data, TaskQueue *queue) {
    // Create task Using task_id
    TaskInfo *task = queue->new_task((TaskType)type, data->unpack_long());
    task->need_reply = 0;
    task->ctrl_param = data->unpack_long();
    if (task->type == LASTPOS) {
        task->current_position = data->unpack_long();
    }
    task->status = COMPLETED;
    log.debug("[TASK] Unpacked reply task %ld.", task->task_id);
    return task;
}

// task当前的任务，queue发送队列
RawData *pack_data(TaskInfo *task, TaskQueue *queue) {
    if (task == NULL) return NULL;
    RawData *data = NULL;
    switch (task->type) {
        case SMALL_FILE:
            //小文件就一个RawData带走
            data = pack_small_file(task, queue);
            break;
        case LARGE_FILE:
            //大文件就多来几个RawData带走
            if (task->current_position < 0) {
                //大数据先发送元数据信息
                data = pack_large_file_begin(task, queue);
            } else {
                //再分块发送数据
                data = pack_large_file_part(task, queue);
            }
            break;
        case MESSAGE:
            data = pack_message(task, queue);
            break;
        default:
            data = pack_reply(task, queue);
            break;
    }
    //加密或者压缩,暂无功能
    data = presend_data(data);
    task->status = TRANSING;
    
    // 改变任务的状态
    queue->update_task(*task);
    return data;
}

TaskInfo *unpack_data(RawData *data, TaskQueue *queue) {
    data = postrecv_data(data);
    data->reset();//接收到数据后，指针归零
    DataType type = (DataType)data->unpack_char();
    TaskInfo *task = NULL;
    switch (type) {
        //将读到的RawData拆包，在包装成TaskInfo，存到TaskList上
        case SMALL_FILE_DATA:
            task = unpack_small_file(data, queue);
            break;
        case LARGE_FILE_BEGIN:
            task = unpack_large_file_begin(data, queue);
            break;
        case LARGE_FILE_PART:
            task = unpack_large_file_part(data, queue);
            break;
        case MESSAGE_DATA:
            task = unpack_message(data, queue);
            break;
        default:
            task = unpack_reply(type, data, queue);
            break;
    }
    queue->update_task(*task);
    return task;
}

// If COMPLETED, DONOT delete the task from disk, but kept in memory
void task_success(TaskInfo *task, TaskQueue *queue) {
    int consider = 0;
    if (task->type == LARGE_FILE) {
        // Update current_position
        if (task->current_position < 0) {
            task->current_position = 0;
            task->status = STARTED;
            consider = 1;
            log.debug("[TASK] Task success: %s(%ld)-BEGIN.", task->task_info, task->task_id);
        } else if (task->current_position < task->file_size) {
            task->current_position += task->block_size;
            if (task->current_position < task->file_size) {
                //Still need sending
                task->status = STARTED;
                consider = 1;
                log.debug("[TASK] Task success: %s(%ld)-%d.", task->task_info, task->task_id, task->current_position);
            }
        }
    }
    if (consider == 0) {
        // SMALL_FILE, MESSAGE, CONTROL
        task->status = COMPLETED;
        log.info("[TASK] Task finished successful: %s(%ld).", task->task_info, task->task_id);
    }
    queue->update_task(*task);
}

void task_failure(TaskInfo *task, TaskQueue *queue) {
    task->error_count++;
    task->block_size = -1;
    task->status = ERROR;
    task->resume_interval = config.get_int("TASK", "ResumeInterval", 10);
    if (task->error_count > config.get_int("TASK", "MaxErrorCount", 3)) {
        if (task->priority > 3) {
            task->priority ++;
            task->error_count = 0;
        }
        log.error("[TASK] Task degraded for ERROR: %s(%ld).", task->task_info, task->task_id);
    } else {
        log.error("[TASK] Task paused with ERROR: %s(%ld).", task->task_info, task->task_id);
    }
    queue->update_task(*task);
}

