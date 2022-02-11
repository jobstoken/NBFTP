/*
 * control_part.cpp
 *
 *  Created on: Mar 28, 2018
 *      Author: sunchao
 */

/*
 * nbftp_server.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "control_part.h"
#include "protect.h"
#include "taskop.h"
#include "utils/log.h"
#include "utils/transfer.h"

#define CTRL_LEN        32

static char SERVER[] = "127.0.0.1";

Control::Control(int port) {
    _port = port;
}

static void print_task(const TaskInfo task, void *arg) {
    RawData *response = (RawData *)arg;
    char buffer[512];
    int writen;
    if (task.type <= MESSAGE) {
        writen = sprintf(buffer, "%10ld %-32s", task.task_id, task.task_info);
    } else {
        return;
    }
    if (task.status == INITIAL || task.status == PENDING) {
        sprintf(buffer + writen, "Pending...");
    } else if (task.status == ERROR || task.status == FAULT) {
        sprintf(buffer + writen, "Error: %d", task.error_count);
    } else if (task.status == COMPLETED || task.status == CHECKED) {
        sprintf(buffer + writen, "Completed.");
    } else if (task.status == STARTED || task.status == TRANSING) {
        sprintf(buffer + writen, "Transferring...");
    } else if (task.status == INCONSIST) {
        sprintf(buffer + writen, "Inconsistent!\n");
    } else if (task.status == PAUSED) {
        sprintf(buffer + writen, "Paused!\n");
    } else {
        sprintf(buffer + writen, "Unknown status!\n");
    }
    response->pack_string(buffer);
}

RawData *Control::process(RawData *request) {
    RawData *response = new RawData(CTRL_LEN);
    //从首位先读出一个int大小的数据作为数据类型，不同类型的数据区别处理
    ControlType type = (ControlType)request->unpack_int();
    int ret = 0;
    if (type == GET_STATUS) {
        log.info("[CONTROL] Get Status.");
        ret = status;//默认正常状态
    } else if (type == STOP_SERVER) {
        log.info("[CONTROL] Stop Server.");
        status = 0;
        for (int i = 0; i < 10; i++) {
            usleep(500000);
            if (status == -1) {
                response->pack_string("Server stopped!");
                break;
            }
        }
        if (status != -1) {
            response->pack_string("Server cannot stop!");
            ret = -1;
        }
    } else if (type == ADD_MESSAGE) {
        log.info("[CONTROL] Add message.");
        int priority = request->unpack_int();
        char *message = request->unpack_string(NULL);
        log.debug("[CONTROL] Message is %s.", message);
        add_message_task(&send_queue, message, priority);
        delete message;
        response->pack_string("Add message task successful!");
    } else if (type == ADD_FILE) {
        log.debug("[CONTROL] Add file.");
        int priority = request->unpack_int();
        char *full_path = request->unpack_string(NULL);
        add_file_task(&send_queue, full_path, priority);
        char *message = stradd("Add file task ", full_path, " successful!");
        response->pack_string(message);
        delete full_path;
        delete message;
    } else if (type == GET_RECEIVED) {
        log.info("[CONTROL] Get received.");
        long taskid = request->unpack_long();
        TaskInfo *task = recv_queue.find_task(taskid);
        if (task == NULL) {
            response->pack_string("Task NOT found!");
            ret = -1;
        } else {
            if (task->type == MESSAGE) {
                response->pack_string(task->store);
            } else {
                char *dest_dir = request->unpack_string(NULL);
                char *target = stradd(dest_dir, "/", task->task_info);
                copy_file(task->store, target);
                char *message = stradd("Get file ", task->task_info, " successful!");
                response->pack_string(message);
                delete message;
                delete dest_dir;
                delete target;
            }
            delete task;
        }
    } else if (type == DELETE_SEND) {
        log.info("[CONTROL] Delete send task.");
        long taskid = request->unpack_long();
        if (taskid < 0) {
            send_queue.del_all_tasks();
            response->pack_string("All pending tasks deleted!");
        } else {
            TaskInfo *task = send_queue.find_task(taskid);
            if (task == NULL) {
                response->pack_string("Task NOT found!");
                ret = -1;
            } else {
                delete task;
                send_queue.del_task(taskid);
                response->pack_string("Delete task successful!");
            }
        }
    } else if (type == DELETE_RECEIVE) {
        log.info("[CONTROL] Delete received task.");
        long taskid = request->unpack_long();
        if (taskid < 0) {
            recv_queue.del_all_tasks();
            response->pack_string("All received tasks deleted!");
        } else {
            TaskInfo *task = recv_queue.find_task(taskid);
            if (task == NULL) {
                response->pack_string("Task NOT found!");
                ret = -1;
            } else {
                delete task;
                recv_queue.del_task(taskid);
                response->pack_string("Delete task successful!");
            }
        }
    } else if (type == LIST_SEND) {
        log.info("[CONTROL] List send tasks.");
        send_queue.walk_tasks(print_task, (void *)response);
    } else if (type == LIST_RECEIVE) {
        log.info("[CONTROL] List received tasks.");
        recv_queue.walk_tasks(print_task, (void *)response);
    } else {
        response->pack_string("No this operation!");
        ret = -1;
    }
    response->pack_int(ret);//加入返回值
    return response;
}

void Control::server_routine(function protect) {
    Socket socket(_port, 5, 1);    // 启动监听服务
    while (status == 1) {
        (*protect)();
        Connection *conn = socket.client_link();    // 客户端连接
        if (conn == NULL) {
            usleep(500000);
            continue;
        }
        if (conn->connected()) {
            conn->set_timeout(10);         // 设置超时时间
            if (conn->has_data() > 0) {//获取数据长度
                RawData *request = conn->recv_data();//根据获取的长度读数据
                RawData *response = process(request);
                conn->send_data(response, 1);
                delete request;
                delete response;
            }
        }
        delete conn;
    }
}

RawData *Control::remote(RawData *request, int sync) {
    Connection *conn = new Connection(SERVER, _port);  // 连接服务器
    if (conn == NULL) return NULL;
    if (!conn->connected()) {
        delete conn;
        return NULL;
    }
    conn->set_timeout(10);         // 设置超时时间
    conn->send_data(request);
    int ret = conn->has_data();
    RawData *response = NULL;
    if (sync == 1) {
        while (ret == -2) ret = conn->has_data();
    }
    if (ret > 0) {
        response = conn->recv_data();
    }
    delete conn;
    return response;
}

int Control::get_status() {
    RawData data(CTRL_LEN);
    data.pack_int(GET_STATUS);
    RawData *response = remote(&data, 0);
    if (response == NULL) {
        return 0;
    } else {
        int status = response->unpack_int();
        delete response;
        return status;
    }
}

RawData *Control::stop_server() {
    RawData data(CTRL_LEN);
    data.pack_int(STOP_SERVER);
    return remote(&data, 1);
}

RawData *Control::add_send_message(const char *message, int priority) {
    RawData data(CTRL_LEN);
    data.pack_int(ADD_MESSAGE);
    data.pack_int(priority);
    data.pack_string(message);
    return remote(&data, 1);
}

RawData *Control::add_send_file(const char *filename, int priority) {
    char *full_path = NULL;
    if (filename[0] == '/') {
        full_path = strdup(filename);
    } else {
        char *dir = getcwd(NULL, 0);
        full_path = stradd(dir, "/", filename);
        delete dir;
    }
    RawData data(CTRL_LEN);
    data.pack_int(ADD_FILE);
    data.pack_int(priority);
    data.pack_string(full_path);
    delete full_path;
    return remote(&data, 1);
}

RawData *Control::get_received_task(long taskid) {
    char *dir = getcwd(NULL, 0);
    RawData data(CTRL_LEN);
    data.pack_int(GET_RECEIVED);
    data.pack_long(taskid);
    data.pack_string(dir);
    delete dir;
    return remote(&data, 1);
}

RawData *Control::delete_send_task(long taskid) {
    RawData data(CTRL_LEN);
    data.pack_int(DELETE_SEND);
    data.pack_long(taskid);
    return remote(&data, 1);
}

RawData *Control::delete_received_task(long taskid) {
    RawData data(CTRL_LEN);
    data.pack_int(DELETE_RECEIVE);
    data.pack_long(taskid);
    return remote(&data, 1);
}

RawData *Control::list_send_tasks() {
    RawData data(CTRL_LEN);
    data.pack_int(LIST_SEND);
    return remote(&data, 1);
}

RawData *Control::list_received_tasks() {
    RawData data(CTRL_LEN);
    data.pack_int(LIST_RECEIVE);
    return remote(&data, 1);
}
