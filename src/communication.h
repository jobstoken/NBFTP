/*
 * communication.h
 *
 *  Created on: Jan 7, 2017
 *      Author: sunchao
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include "utils/transfer.h"

/* 发送任务并处理。返回值：0为正常发送，1为发送空任务（队列无可发送任务），-1为发送出错。 */
int send_tasks(Connection *conn);

/* 接收任务并处理。返回值：0为正常接收，1为接收完毕，-1为接收出错，-2为暂时接收不到数据。 */
int receive_tasks(Connection *conn);

#endif /* COMMUNICATION_H_ */
