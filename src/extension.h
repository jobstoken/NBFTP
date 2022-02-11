/*
 * extension.h
 *
 *  Created on: Jan 14, 2017
 *      Author: sunchao
 */

#ifndef EXTENSION_H_
#define EXTENSION_H_

/* 读取配置文件，初始化扩展模块。 */
void init_extension();

/* 调用扩展功能。 */
void call_extension(const char *dir, const char *path_or_info, const char *truename);

/* 清理扩展模块。 */
void stop_extension();

#endif /* EXTENSION_H_ */
