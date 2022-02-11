/*
 * monitorop.h
 *
 *  Created on: Jan 11, 2017
 *      Author: sunchao
 */

#ifndef MONITOROP_H_
#define MONITOROP_H_

/* 读取配置文件，创建监控器。 */
void init_monitors();

/* 检测所有监控器。 */
void check_monitors();

/* 停止所有监控器。 */
void stop_monitors();

#endif /* MONITOROP_H_ */
