/*
 * control_part.h
 *
 *  Created on: Mar 28, 2018
 *      Author: sunchao
 */

#ifndef CONTROL_PART_H_
#define CONTROL_PART_H_

#include <time.h>
#include "utils/rawdata.h"

enum ControlType {
    GET_STATUS,
    STOP_SERVER,
    ADD_MESSAGE,
    ADD_FILE,
    GET_RECEIVED,
    DELETE_SEND,
    DELETE_RECEIVE,
    LIST_SEND,
    LIST_RECEIVE
};

typedef void (*function)(void);

class Control {
private:
    int _port;          // 控制端服务器的端口

    /* 处理控制消息。 */
    RawData *process(RawData *request);
    /* 与服务器通信命令。 */
    RawData *remote(RawData *data, int sync);
public:
    Control(int port);
    /* 启动控制的服务端，接收控制消息。 */
    void server_routine(function protect);
    /* 获取运行状态。 */
    int get_status();
    /* 停止传输服务。 */
    RawData *stop_server();
    /* 添加发送文件。 */
    RawData *add_send_file(const char *filename, int priority);
    /* 添加发送消息。 */
    RawData *add_send_message(const char *message, int priority);
    /* 添加发送消息。 */
    RawData *get_received_task(long taskid);
    /* 清理发送任务(-1为清理所有)。 */
    RawData *delete_send_task(long taskid);
    /* 清理接收任务(-1为清理所有)。 */
    RawData *delete_received_task(long taskid);
    /* 列出发送任务。 */
    RawData *list_send_tasks();
    /* 列出接收任务。 */
    RawData *list_received_tasks();
};

#endif /* SRC_CONTROL_PART_H_ */
