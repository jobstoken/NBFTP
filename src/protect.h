/*
 * protect.h
 *
 *  Created on: Jan 10, 2017
 *      Author: sunchao
 */

#ifndef PROTECT_H_
#define PROTECT_H_

extern const char *main_pidfile;
extern const char *protect_pidfile;

/* 读取配置文件，获取记录状态的文件名。 */
void init_protect(char **argv);

/* 创建PID file。 */
void save_pidfile(const char *pidfile);

/* 记录主进程的状态，并检测交叉保护进程的运行状态。 */
void check_protect();

/* 清理交叉保护模块。 */
void stop_protect();

#endif /* PROTECT_H_ */
